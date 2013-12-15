// Leslie Lamport, The Mutual Exclusion Problem: Part II - Statement and Solutions, Journal of the ACM, 33(2), 1986,
// Fig. 1, p. 337

enum Intent { DontWantIn, WantIn };
volatile TYPE *intents;									// shared

static void *Worker( void *arg ) {
	unsigned int id = (size_t)arg;
	int j;
#ifdef FAST
	unsigned int cnt = 0;
#endif // FAST
	size_t entries[RUNS];

	for ( int r = 0; r < RUNS; r += 1 ) {
		entries[r] = 0;
		while ( stop == 0 ) {
#ifdef FAST
			id = startpoint( cnt );						// different starting point each experiment
			cnt = cycleUp( cnt, NoStartPoints );
#endif // FAST
		  L: intents[id] = WantIn;
			Fence();									// force store before more loads
			for ( j = 0; j < id; j += 1 ) {				// check if thread with higher id wants in
//			for ( j = id - 1; j >= 0; j -= 1 ) {
				if ( intents[j] == WantIn ) {
					intents[id] = DontWantIn;
					Fence();							// force store before more loads
					while ( intents[j] == WantIn ) Pause();
					goto L;
				} // if
			} // for
			for ( j = id + 1; j < N; j += 1 )
				while ( intents[j] == WantIn ) Pause();
			CriticalSection( id );						// critical section
			intents[id] = DontWantIn;					// exit protocol
			entries[r] += 1;
		} // while
		__sync_fetch_and_add( &Arrived, 1 );
		while ( stop != 0 ) Pause();
		__sync_fetch_and_add( &Arrived, -1 );
	} // for
	qsort( entries, RUNS, sizeof(size_t), compare );
	return (void *)median(entries);
} // Worker

void ctor() {
	intents = Allocator( sizeof(volatile TYPE) * N );
	for ( int i = 0; i < N; i += 1 ) {					// initialize shared data
		intents[i] = DontWantIn;
	} // for
} // ctor

void dtor() {
	free( (void *)intents );
} // dtor

// Local Variables: //
// tab-width: 4 //
// compile-command: "gcc -Wall -std=gnu99 -O3 -DAlgorithm=LamportRetractIntent Harness.c -lpthread -lm" //
// End: //

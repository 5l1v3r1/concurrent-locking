volatile TYPE **intents;								// triangular matrix of intents
volatile TYPE **turns;									// triangular matrix of turns
unsigned int depth;

static void *Worker( void *arg ) {
	unsigned int id = (size_t)arg;
	unsigned int ridt, ridi;
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
			ridi = id;
			for ( int lv = 0; lv < depth; lv += 1 ) {	// entry protocol
//				ridi = id >> lv;						// round id for intent
				ridt = ridi >> 1;						// round id for turn
				intents[lv][ridi] = 1;					// declare intent
				turns[lv][ridt] = ridi;					// RACE
				Fence();								// force store before more loads
				while ( intents[lv][ridi ^ 1] == 1 && turns[lv][ridt] == ridi ) Pause();
				ridi = ridi >> 1;
			} // for
			CriticalSection( id );
			for ( int lv = depth - 1; lv >= 0; lv -= 1 ) { // exit protocol
				intents[lv][id >> lv] = 0;				// retract all intents in reverse order
			} // for
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
	depth = Clog2( N );									// maximal depth of binary tree
	int width = 1 << depth;								// maximal width of binary tree
	intents = Allocator( sizeof(volatile TYPE *) * depth );	// allocate matrix columns
	turns = Allocator( sizeof(volatile TYPE *) * depth );
	for ( int r = 0; r < depth; r += 1 ) {				// allocate matrix rows
		int size = width >> r;							// maximal row size
		intents[r] = Allocator( sizeof(TYPE) * size );
		for ( int c = 0; c < size; c += 1 ) {			// initial all intents to dont-want-in
			intents[r][c] = 0;
		} // for
		turns[r] = Allocator( sizeof(TYPE) * (size >> 1) );	// half maximal row size
	} // for
} // ctor

void dtor() {
	for ( int r = 0; r < depth; r += 1 ) {				// deallocate matrix rows
		free( (void *)turns[r] );
		free( (void *)intents[r] );
	} // for
	free( (void *)turns );								// deallocate matrix columns
	free( (void *)intents );
} // dtor

// Local Variables: //
// tab-width: 4 //
// compile-command: "gcc -Wall -std=gnu99 -O3 -DAlgorithm=TaubenfeldBuhr Harness.c -lpthread -lm" //
// End: //

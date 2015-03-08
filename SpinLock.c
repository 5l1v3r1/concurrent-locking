static volatile TYPE lock
#if defined( __i386 ) || defined( __x86_64 )
	__attribute__(( aligned (128) ));					// Intel recommendation
#elif defined( __sparc )
	CALIGN;
#else
    #error unsupported architecture
#endif
static TYPE PAD CALIGN __attribute__(( unused ));		// protect further false sharing

void spin_lock( volatile TYPE *lock ) {
#ifndef NOEXPBACK
	enum { SPIN_START = 4, SPIN_END = 64 * 1024, };
	unsigned int spin = SPIN_START;
#endif // ! NOEXPBACK

	for ( unsigned int i = 1;; i += 1 ) {
	  if ( *lock == 0 && __sync_lock_test_and_set( lock, 1 ) == 0 ) break;
#ifndef NOEXPBACK
		for ( volatile unsigned int s = 0; s < spin; s += 1 ) Pause(); // exponential spin
		//spin += spin;									// powers of 2
		if ( i % 64 == 0 ) spin += spin;				// slowly increase by powers of 2
		if ( spin > SPIN_END ) spin = SPIN_START;		// prevent overflow
#else
	    Pause();
#endif // ! NOEXPBACK
	} // for
} // spin_lock

void spin_unlock( volatile TYPE *lock ) {
	__sync_lock_release( lock );
} // spin_unlock

static void *Worker( void *arg ) {
	TYPE id = (size_t)arg;
	uint64_t entry;
#ifdef FAST
	unsigned int cnt = 0, oid = id;
#endif // FAST

	for ( int r = 0; r < RUNS; r += 1 ) {
		entry = 0;
		while ( stop == 0 ) {
			spin_lock( &lock );
			CriticalSection( id );
			spin_unlock( &lock );
#ifdef FAST
			id = startpoint( cnt );						// different starting point each experiment
			cnt = cycleUp( cnt, NoStartPoints );
#endif // FAST
			entry += 1;
		} // while
#ifdef FAST
		id = oid;
#endif // FAST
		entries[r][id] = entry;
		__sync_fetch_and_add( &Arrived, 1 );
		while ( stop != 0 ) Pause();
		__sync_fetch_and_add( &Arrived, -1 );
	} // for
	return NULL;
} // Worker

void ctor() {
	lock = 0;
} // ctor

void dtor() {
} // dtor

// Local Variables: //
// tab-width: 4 //
// compile-command: "gcc -Wall -std=gnu99 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DAlgorithm=SpinLock Harness.c -lpthread -lm" //
// End: //

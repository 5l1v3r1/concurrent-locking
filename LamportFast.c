// Leslie Lamport, A Fast Mutual Exclusion Algorithm, ACM Transactions on Computer Systems, 5(1), 1987, Fig. 2, p. 5
// N => do not want in, versus 0 in original paper, so "b" is dimensioned 0..N-1 rather than 1..N.

#include <stdbool.h>

volatile TYPE *b;
volatile TYPE x, y;

#define await( E ) while ( ! (E) ) Pause()

static void *Worker( void *arg ) {
	unsigned int id = (size_t)arg;
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
		  start: b[id] = true;							// entry protocol
			x = id;
			Fence();									// force store before more loads
			if ( y != N ) {
				b[id] = false;
				Fence();								// force store before more loads
				await( y == N );
				goto start;
			}
			y = id;
			Fence();									// force store before more loads
			if ( x != id ) {
				b[id] = false;
				Fence();								// force store before more loads
				for ( int j = 0; j < N; j += 1 )
					await( ! b[j] );
				if ( y != id ) {
//					await( y == N );
					goto start;
				}
			}
			CriticalSection( id );
			y = N;										// exit protocol
			b[id] = false;
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
	b = Allocator( sizeof(volatile TYPE) * N );
	for ( int i = 0; i < N; i += 1 ) {					// initialize shared data
		b[i] = false;
	} // for
	y = N;
} // ctor

void dtor() {
	free( (void *)b );
} // dtor

// Local Variables: //
// tab-width: 4 //
// compile-command: "gcc -Wall -std=gnu99 -O3 -DAlgorithm=LamportFast Harness.c -lpthread -lm" //
// End: //

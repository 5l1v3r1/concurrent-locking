// Static and dynamic allocation of tables available.

typedef struct CALIGN {
	TYPE Q[2], R;
} Token;

typedef struct CALIGN {
	TYPE es;											// left/right opponent
	Token *ns;											// pointer to path node from leaf to root
} Tuple;

static Tuple **states CALIGN;							// handle N threads
static int *levels CALIGN;								// minimal level for binary tree
//static Tuple states[64][6] CALIGN;						// handle 64 threads with maximal tree depth of 6 nodes (lg 64)
//static int levels[64] = { -1 } CALIGN;					// minimal level for binary tree
static Token *t CALIGN;
static TYPE PAD CALIGN __attribute__(( unused ));		// protect further false sharing

#define inv( c ) (1 - c)
#define await( E ) while ( ! (E) ) Pause()

static inline void binary_prologue( TYPE c, volatile Token *t ) {
	TYPE other = inv( c );
#if defined( DEKKER )
	t->Q[c] = 1;
	Fence();											// force store before more loads
	while ( t->Q[other] ) {
		if ( t->R == c ) {
			t->Q[c] = 0;
			Fence();									// force store before more loads
			while ( t->R == c ) Pause();				// low priority busy wait
			t->Q[c] = 1;
			Fence();									// force store before more loads
		} else {
			Pause();
		} // if
	} // while
#elif defined( DEKKERWHH )
	for ( ;; ) {
		t->Q[c] = 1;
		Fence();										// force store before more loads
	  if ( ! t->Q[other] ) break;
	  if ( t->R == c ) {
			await( ! t->Q[other] );
			break;
		} // if
		t->Q[c] = 0;
		Fence();
		await( t->R == c || ! t->Q[other] );
	} // for
#elif defined( TSAY )
	t->Q[c] = 1;
	t->R = c;
	Fence();											// force store before more loads
	if ( t->Q[other] ) while ( t->R == c ) Pause();		// busy wait
#else	// Peterson (default)
	t->Q[c] = 1;
	t->R = c;											// RACE
	Fence();											// force store before more loads
	while ( t->Q[other] && t->R == c ) Pause();			// busy wait
#endif
} // binary_prologue

static inline void binary_epilogue( TYPE c, volatile Token *t ) {
#if defined( DEKKER )
	t->R = c;
	t->Q[c] = 0;
#elif defined( DEKKERWHH )
	t->R = c;
	if ( t->R == c ) {
		t->R = inv( c );
	} // if
	t->Q[c] = 0;
#elif defined( TSAY )
	t->Q[c] = 0;
	t->R = c;
#else	// Peterson (default)
	t->Q[c] = 0;
#endif
} // binary_epilogue

static void *Worker( void *arg ) {
	TYPE id = (size_t)arg;
	uint64_t entry;
#ifdef FAST
	unsigned int cnt = 0, oid = id;
#endif // FAST

	int s, level = levels[id];
	Tuple *state = states[id];

	for ( int r = 0; r < RUNS; r += 1 ) {
		entry = 0;
		while ( stop == 0 ) {
			for ( s = 0; s <= level; s += 1 ) {			// entry protocol
				binary_prologue( state[s].es, state[s].ns );
			} // for
			CriticalSection( id );
			for ( s = level; s >= 0; s -= 1 ) {			// exit protocol, reverse order
				binary_epilogue( state[s].es, state[s].ns );
			} // for
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

void __attribute__((noinline)) ctor() {
	// element 0 not used
	t = Allocator( sizeof(typeof(t[0])) * N );

	// states[id][s].es indicates the left or right contender at a match.
	// states[id][s].ns is the address of the structure that contains the match data.
	// s ranges from 0 to the tree level of a start point (leaf) in a minimal binary tree.
	// levels[id] is level of start point minus 1 so bi-directional tree traversal is uniform.

	states = Allocator( sizeof(typeof(states[0])) * N );
	levels = Allocator( sizeof(typeof(levels[0])) * N );
	levels[0] = -1;										// default for N=1
	for ( int id = 0; id < N; id += 1 ) {
		t[id].Q[0] = t[id].Q[1] = 0;
		unsigned int start = N + id, level = Log2( start );
		states[id] = Allocator( sizeof(typeof(states[0][0])) * level );
		levels[id] = level - 1;
		for ( unsigned int s = 0; start > 1; start >>= 1, s += 1 ) {
			states[id][s].es = start & 1;
			states[id][s].ns = &t[start >> 1];
		} // for
	} // for
} // ctor

void __attribute__((noinline)) dtor() {
	free( (void *)levels );
	free( (void *)states );
	free( (void *)t );
} // dtor

// Local Variables: //
// tab-width: 4 //
// compile-command: "gcc -Wall -std=gnu99 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DAlgorithm=PetersonBuhr Harness.c -lpthread -lm" //
// End: //

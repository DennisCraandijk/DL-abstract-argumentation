//
//     MultiSAT
//
//     Siert Wieringa 
//     siert.wieringa@aalto.fi
// (c) Aalto University 2013
//
//
//
#include"simp/SimpSolver.h"
#include"utils/Options.h"
#include"utils/ParseUtils.h"
#include"core/Dimacs.h"
#include<zlib.h>
#include<sys/types.h>
#include<sys/wait.h>

#if (defined GLUCORED) || (defined MINIRED)
#include"reducer/Version.h"
#define SOLVER_NAME VERSION_STRING
#else
#define SOLVER_NAME "SAT solver"
#endif

using namespace Minisat;

#define MULTISAT "MultiSAT"
static IntOption opt_nc (MULTISAT, "nc" , "Number of solvers to run without simplifier\n",       0, IntRange(0, INT32_MAX));
static IntOption opt_ns (MULTISAT, "ns" , "Number of solvers to run after running simplifier\n", 1, IntRange(0, INT32_MAX));

#ifdef NDEBUG
#define quit(n) exit(n)
#else
#define quit(n) return n
#endif


// Signal handler will interrupt the solver
SimpSolver *solver;
static void SIGINT_interrupt(int signum) { solver->interrupt(); }

// Solve will run solver 'S' after setting its random seed.
lbool solve(SimpSolver& S) {
    vec<Lit> dummy;
    return S.solveLimited(dummy);
}

// childSolve implements the actions of a child process
void childSolve(pid_t parent, int pipe, SimpSolver& S) {
    // Give child solver 'S' a random seed different from the other threads and use 2% random decisions
    srand(getpid());
    S.random_seed     = rand();
    S.random_var_freq = 0.02;

    // Run solver
    lbool result = solve(S);
    if ( result != l_Undef ) {
	// If result SAT or UNSAT interrupt parent solver
	kill(parent, SIGINT);
	
	// Ignore future signals to avoid interrupt of the pipe writes
	signal(SIGINT, SIG_IGN);

	char r = (char) toInt(result);
	if ( write(pipe, &r, sizeof(char)) == sizeof(char) && result == l_True ) {
	    // If result SAT write model into pipe
	    char* str = new char[S.nVars()];

	    for( int i = 0; i < S.nVars(); i++ )
		str[i]=(char) toInt(S.model[i]);

	    const int cnt = S.nVars()*sizeof(char);
	    const int w = write(pipe, str, cnt);
	
	    delete [] str;
	}
    }
}

int main(int argc, char** argv) {
    printf("c This is MultiSAT\n");
    printf("c A simple program for running multiple copies of a MiniSAT-like SAT-solver\nc\n");
    printf("c Written by Siert Wieringa, (c) Aalto University 2013\n\n");

    parseOptions(argc, argv, true);

    if ( opt_nc + opt_ns == 0 ) { // If user doesn't want us to run any solvers then there's nothing to do
	printf("s UNKNOWN\n"); 
	quit(0);
    }

    try {
	SimpSolver S; // Create parent's 'master' solver instance
	solver = &S;

	// Disable simplifying features if user did not request any simplifying solvers
        if (opt_ns == 0) S.eliminate(true);	
        
        if (argc == 1) 
	    printf("c Reading from standard input... Use '--help' for help.\n");

	// Open input file
        gzFile in = (argc == 1) ? gzdopen(0, "rb") : gzopen(argv[1], "rb");
        if (in == NULL) {
            fprintf(stderr, "c ERROR! Could not open file: %s\n", argc == 1 ? "<stdin>" : argv[1]);
	    exit(1);
	}

	// Parse input
	parse_DIMACS(in, S);
        gzclose(in);

	// Perform unit propagation
	if ( !S.simplify() ) {
	    printf("c Solved by unit propagation\n");
	    printf("s UNSATISFIABLE\n");
	    quit(20);
	}

	// Setup signal handling
	signal(SIGINT , SIGINT_interrupt);
	signal(SIGPIPE, SIG_IGN);
	
	const pid_t ppid     = getpid();
	const int   childCnt = opt_nc + opt_ns - 1;
	vec<pid_t>  childPids;
	vec<int>    childPipes;
	childPids.capacity(childCnt);
	childPipes.capacity(childCnt);

	lbool ret = l_Undef;
	// Fork 'c' copies, then run simplifier on parent, then fork remaining 's-1' copies
	for(int i = 0; i <= childCnt; i++ ) {
	    // Run simplifier
	    if ( i == opt_nc ) {
		printf("c Running simplifier in parent process\n");

		if ( !S.eliminate(true) ) {
		    printf("c Solved by simplification\n");
		    ret = l_False;
		    break;
		}
	    }

	    if ( i == childCnt ) break;

	    // Create new child
	    int pf[2];
	    if ( pipe(pf) == 0 ) {
		fflush(stdout); // Must flush output to avoid duplicates after fork
		pid_t p = fork();
		if ( p == 0 ) {
		    close(pf[0]); // Close read end of pipe

		    // Disable simplifier if this is one of the first 'c' children
		    if ( i < opt_nc ) {
			S.use_elim  = false; 
			S.use_asymm = false; 
			S.eliminate(true);
		    }

		    // Solve
		    childSolve(ppid, pf[1], S);
		    close(pf[1]);
		    exit(0);
		}
		else if ( p > 0 ) {
		    printf("c Started %s in child process %d\n", SOLVER_NAME, p);

		    close(pf[1]); // Close write end of pipe
		    childPipes.push_(pf[0]);
		    childPids.push_(p);
		}
		else printf("c ERROR Failed to create new child process\n");
	    }
	    else printf("c ERROR Failed to create pipe for new child process\n");
	}

	// Run solver on parent process
	if ( ret == l_Undef ) {
	    printf("c Started %s in parent process\n", SOLVER_NAME);
	    fflush(stdout);
	    ret = solve(S);
	}
	
	// Print results if found by parent process
	if ( ret == l_True ) {
	    printf("s SATISFIABLE\nv ");
	    for( int v = 0; v < S.nVars(); v++ )
		if ( S.model[v] != l_Undef ) 
		    printf("%s%d ", (S.model[v]==l_True)?"":"-", v+1);
	    printf("0\n");
	    printf("c SATISFIABLE found by parent process\n");
	    fflush(stdout);
	}
	else if ( ret == l_False ) {
	    printf("s UNSATISFIABLE\n");
	    printf("c UNSATISFIABLE found by parent process\n");
	    fflush(stdout);
	}
	
	// Interrupt all children
	for( int i = 0; i < childPids.size(); i++ )
	    kill(childPids[i], SIGINT);

	// Read results and close pipes
	for( int i = 0; i < childPids.size(); i++ ) {	      
	    char r;
	    if ( ret == l_Undef && read(childPipes[i], &r, sizeof(char)) == sizeof(char) ) {
		ret = toLbool(r);
		
		if ( ret == l_True ) {
		    char* str = new char[S.nVars()];
		    const int cnt = S.nVars()*sizeof(char);
		    const int rd  = read(childPipes[i], str, cnt); // must always read even if result is known, to avoid blocking on write
		    
		    if ( rd < cnt )
			printf("c ERROR child process %d reported SATISFIABLE but failed to provide model\n", childPids[i]);
		    else {
			printf("s SATISFIABLE\nv ");
			for( int v = 0; v < S.nVars(); v++ ) {
			    lbool r = toLbool(str[v]);
			    if ( r != l_Undef ) printf("%s%d ", (r==l_True)?"":"-", v+1);
			}
			printf("0\n");
			printf("c SATISFIABLE found by child process %d\n", childPids[i]);
			fflush(stdout);
		    }

		    delete str;
		}
		else if ( ret == l_False ) {		   
		    printf("s UNSATISFIABLE\n");
		    printf("c UNSATISFIABLE found by child process %d\n", childPids[i]);
		    fflush(stdout);
		}
		else printf("c ERROR received unexpected result 'UNKNOWN' from child process %d\n", childPids[i]);			       
	    }

	    // Close pipe from child to parent
	    close(childPipes[i]);
	}
	
	// Wait for all children to terminate
	for( int i = 0; i < childPids.size(); i++ ) 
	    waitpid(childPids[i], NULL, 0);	
	
	if ( ret == l_True ) 
	  quit(10);
	else if ( ret == l_False ) 
	  quit(20);

    } catch (OutOfMemoryException&){
	printf("c Out of memory\n");
    }

    printf("s UNKNOWN\n");
    quit(0);
}

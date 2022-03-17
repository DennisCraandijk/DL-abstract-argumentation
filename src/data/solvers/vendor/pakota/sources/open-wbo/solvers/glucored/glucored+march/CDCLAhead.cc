//
//     CDCL Ahead
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

#define LOOKAHEAD_SOLVER_PATH "march_rw"

using namespace Minisat;

#ifdef NDEBUG
#define quit(n) exit(n)
#else
#define quit(n) return n
#endif

// Overloading MiniSAT's 'SimpSolver' class allows capturing input from file parser
class SelectSolver : public SimpSolver {
public:
    SelectSolver () : uniqLengths(0) {}
    ~SelectSolver () { clean(); }

    bool addClause_(vec<Lit>& ps) {
        if ( !okay() ) return false;

        // Record number of clauses of a certain length, count number of unique lengths
        clauseLengths.growTo(ps.size(), 0);
        if ( clauseLengths[ps.size()-1]++ == 0 ) uniqLengths++;

        // Make a copy of the clause if we still want to calculate the diameter
        if ( uniqLengths <= 2 ) {
            vec<Lit> *copy = new vec<Lit>();
            ps.copyTo(*copy);

            for( int i = 0; i < ps.size(); i++ )
                occ[var(ps[i])].push(copy);

            clauses.push(copy);
        }

        return SimpSolver::addClause_(ps);
    }

    Var newVar() { occ.push(); distance.push(0); return SimpSolver::newVar(); }

    // Delete memory used for diameter checking procedure
    void clean() {
        for( int i = 0; i < clauses.size(); i++ )
            delete clauses[i];
        clauses.clear();
        occ.clear();

    }

    // Run CDCL solver if:
    //   - Input formula unsat by IUP (no need to do any more work...)
    //   - Unique number of different clause lengths > 2
    //   - Unique number of clause lengths = 2 and diameter of formula > 4
    bool runCDCL() {
        return !okay() || uniqLengths > 2 || (uniqLengths == 2 && !checkDiameterMax(4));
    }
private:
    int checkDistanceMax (Var v, int max) {
        int maxD  = 0;
        int qhead = 0;

        bfsQueue.clear();
        bfsQueue.push_(v);
        while( qhead < bfsQueue.size() ) {
            Var p= bfsQueue[qhead++];
            maxD = distance[p];
            for ( int i = 0; i < occ[p].size(); i++ ) {
                vec<Lit>& c = *(occ[p][i]);

                for( int j = 0; j < c.size(); j++ ) {
                    Var q = var(c[j]);
                    if ( q != v && distance[q] == 0 ) {
                        distance[q] = maxD + 1;
                        if ( distance[q] > max ) return false; // Stop if maximum exceeded

                        bfsQueue.push_(q);
                    }
                }
            }
        }

        for( int i = 0; i < distance.size(); i++ )
            distance[i] = 0;

        return true;
    }

    bool checkDiameterMax(int max) {
        bfsQueue.capacity(occ.size());

        // Check for each variable wether the longest path from that variable to any other is shorter than 'max'
        for( Var v = 0; v < occ.size(); v++ )
            if ( occ[v].size() && !checkDistanceMax(v, max) ) return false;

        return true;
    }

    // Used by 'checkDistanceMax' only
    vec<int>  distance;
    vec<int>  bfsQueue;
    vec<bool> inQueue;

    // Other
    int                   uniqLengths;
    vec<int>              clauseLengths;
    vec<vec<Lit>* >       clauses;
    vec<vec<vec<Lit>* > > occ;
};

int main(int argc, char** argv) {

    try {
        printf("c Simple heuristics for selecting GlucoRed or March SAT-solver\n");
        printf("c Written by Siert Wieringa, (c) Aalto University 2013\n\n");
        parseOptions(argc, argv, true);

        SelectSolver S; // Create solver instance

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

        // Run Simplifier + CDCL Solver
        if ( S.runCDCL() ) {
            S.clean();
            S.eliminate(true);
            if ( !S.okay() ) {
                printf("c Solved by simplification\n");
                printf("s UNSATISFIABLE\n");
                quit(20);
            }

            vec<Lit> dummy;

            lbool ret = S.solveLimited(dummy);
            if ( ret == l_True ) {
                printf("s SATISFIABLE\nv ");
                for( int v = 0; v < S.nVars(); v++ )
                    if ( S.model[v] != l_Undef )
                        printf("%s%d ", (S.model[v]==l_True)?"":"-", v+1);
                printf("0\n");
                fflush(stdout);
                quit(10);
            }
            else if ( ret == l_False ) {
                printf("s UNSATISFIABLE\n");
                fflush(stdout);
                quit(20);
            }
        }
        // Run lookahead solver
        else {
            fflush(stdout);
            execl(LOOKAHEAD_SOLVER_PATH, LOOKAHEAD_SOLVER_PATH, argv[1], NULL);
        }
    }
    catch (OutOfMemoryException&){
        printf("c Out of memory\n");
    }

    printf("s UNKNOWN\n");
    quit(0);
}

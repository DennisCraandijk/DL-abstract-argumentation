//
//     MiniRed/GlucoRed
//
//     Siert Wieringa 
//     siert.wieringa@aalto.fi
// (c) Aalto University 2012/2013
//
//
#ifndef ext_solver_h
#define ext_solver_h
#include"core/Solver.h"
#include"reducer/Version.h"

namespace Minisat {

// An extension of the 'Solver' class
class ExtSolver : public Solver {
protected:
    // Copy the problem clauses of this solver instance to 'other', 
    // starting from 'offset' (for incremental SAT)
    //
    // The variables that are created by copyProblem in 'other' are NOT
    // set as decision (=branching) variables
    // 
    bool copyProblem (ExtSolver& other, int offset);

    // Add clause 'lits' regardless of the current solver state. Performs a
    // minimal amount of backtracking required to allow this. If the clause
    // is asserting will propagate the asserting literal. Returns the reference
    // to a conflict if one occurs.
    //
    // This function may set 'ok' to 'false' !!!
    //
    CRef addClauseOnFly (vec<Lit>& lits);

private:
    // Helper function for 'addClauseOnFly'
    bool litComp (const Lit& p, const Lit& q);   

#ifdef GLUCORED
    vec<int> tmp_lbd;
#endif
};

}

#endif

/*****************************************************************************************[Alg_MSU3.cc]
Open-WBO -- Copyright (c) 2013-2015, Ruben Martins, Vasco Manquinho, Ines Lynce

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
************************************************************************************************/

#ifndef Alg_WMSU3_h
#define Alg_WMSU3_h

#ifdef SIMP
#include "simp/SimpSolver.h"
#else
#include "core/Solver.h"
#endif

#include "../MaxSAT.h"
#include "../Encoder.h"
#include <vector>
#include <map>
#include <set>

namespace NSPACE
{

//=================================================================================================
class WMSU3 : public MaxSAT
{

public:
  WMSU3(int verb = _VERBOSITY_SOME_, int incremental = _INCREMENTAL_ITERATIVE_,
        int enc = _CARD_TOTALIZER_, int pb = _PB_SWC_, bool bmo = true)
  {
    solver = NULL;
    verbosity = verb;
    incremental_strategy = incremental;
    encoding = enc;
    encoder.setCardEncoding(enc);
    encoder.setPBEncoding(pb);
    bmo_strategy = bmo;
    is_bmo = false;
  }
  ~WMSU3()
  {
    if (solver != NULL) delete solver;
  }

  // Print solver configuration.
  void printConfiguration()
  {
    printf("c ==========================================[ Solver Settings "
           "]============================================\n");
    printf("c |                                                                "
           "                                       |\n");
    printf("c |  Algorithm: %23s                                             "
           "                      |\n",
           "WMSU3");
    print_Incremental_configuration(incremental_strategy);
    print_WMSU3_configuration();
    printf("c |                                                                "
           "                                       |\n");
  }

  void search(); // MSU3 search.

protected:
  // Rebuild MaxSAT solver
  //
  Solver *rebuildSolver(); // Rebuild MaxSAT solver.

  // Search methods
  //
  void WMSU3_none_bmo();
  void WMSU3_none();
  void WMSU3_iterative_bmo();
  void WMSU3_iterative();

  // Other
  //
  void initRelaxation(); // Relaxes soft clauses.
  // subsetsum with dynamic programming.
  bool subsetSum(vec<int> &set, int64_t num);
  void print_WMSU3_configuration(); // Print WSMU3 configuration.

  Solver *solver;  // SAT Solver used as a black box.
  Encoder encoder; // Interface for the encoder of constraints to CNF.

  // Controls the incremental strategy used by MSU3 algorithms.
  int incremental_strategy;
  int encoding; // Controls the cardinality encoding used by MSU3 algorithms.

  int weightStrategy;   // Weight strategy to be used in 'weightSearch'.
  vec<Lit> assumptions; // Assumptions used in the SAT solver.

  // Literals to be used in the constraint that excludes models.
  vec<Lit> objFunction;
  vec<int> coeffs; // Coefficients of the literals that are used in the
                   // constraint that excludes models.

  std::map<Lit, int> coreMapping; // Mapping between the assumption literal and
                                  // the respective soft clause.

  // Soft clauses that are currently in the MaxSAT formula.
  vec<bool> activeSoft;

  bool bmo_strategy; // BMO strategy option.
  bool is_bmo;       // Tests if MaxSAT formula has the BMO property.

  // Used for disabling soft clauses that did not become active during previous
  // iterations of the bmo algorithm (non-incremental algorithm).
  vec<Lit> unit_bmo;
};
}

#endif

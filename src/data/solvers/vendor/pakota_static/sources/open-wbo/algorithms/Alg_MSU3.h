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

#ifndef Alg_MSU3_h
#define Alg_MSU3_h

#ifdef SIMP
#include "simp/SimpSolver.h"
#else
#include "core/Solver.h"
#endif

#include "../MaxSAT.h"
#include "../Encoder.h"
#include <map>
#include <set>

namespace NSPACE
{

//=================================================================================================
class MSU3 : public MaxSAT
{

public:
  MSU3(int verb = _VERBOSITY_MINIMAL_, int incremental = _INCREMENTAL_NONE_,
       int enc = _CARD_TOTALIZER_)
  {
    solver = NULL;
    verbosity = verb;
    incremental_strategy = incremental;
    encoding = enc;
    encoder.setCardEncoding(enc);
  }
  ~MSU3()
  {
    if (solver != NULL) delete solver;
  }

  void search(); // MSU3 search.

  // Print solver configuration.
  void printConfiguration()
  {
    printf("c ==========================================[ Solver Settings "
           "]============================================\n");
    printf("c |                                                                "
           "                                       |\n");
    printf("c |  Algorithm: %23s                                             "
           "                      |\n",
           "MSU3");
    print_Incremental_configuration(incremental_strategy);
    print_Card_configuration(encoding);
    printf("c |                                                                "
           "                                       |\n");
  }

protected:
  // Rebuild MaxSAT solver
  //
  Solver *rebuildSolver(); // Rebuild MaxSAT solver.

  void MSU3_none();      // Non-incremental MSU3.
  void MSU3_blocking();  // Incremental Blocking MSU3.
  void MSU3_weakening(); // Incremental Weakening MSU3.
  void MSU3_iterative(); // Incremental Iterative Encoding MSU3.

  // Other
  void initRelaxation(); // Relaxes soft clauses.

  Solver *solver;  // SAT Solver used as a black box.
  Encoder encoder; // Interface for the encoder of constraints to CNF.

  // Controls the incremental strategy used by MSU3 algorithms.
  int incremental_strategy;
  // Controls the cardinality encoding used by MSU3 algorithms.
  int encoding;

  // Literals to be used in the constraint that excludes models.
  vec<Lit> objFunction;
  vec<int> coeffs; // Coefficients of the literals that are used in the
                   // constraint that excludes models.

  std::map<Lit, int> coreMapping; // Mapping between the assumption literal and
                                  // the respective soft clause.

  // Soft clauses that are currently in the MaxSAT formula.
  vec<bool> activeSoft;
};
}

#endif

/*****************************************************************************************[Alg_LinearUS.h]
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

#ifndef Alg_LinearUS_h
#define Alg_LinearUS_h

#ifdef SIMP
#include "simp/SimpSolver.h"
#else
#include "core/Solver.h"
#endif

#include "../MaxSAT.h"
#include "../Encoder.h"
#include <map>
#include <set>
#include <algorithm>

namespace NSPACE
{

//=================================================================================================
class LinearUS : public MaxSAT
{

public:
  LinearUS(int verb = _VERBOSITY_MINIMAL_, int incremental = _INCREMENTAL_NONE_,
           int enc = _CARD_TOTALIZER_)
  {
    solver = NULL;
    verbosity = verb;
    incremental_strategy = incremental;
    encoding = enc;
    encoder.setCardEncoding(enc);
  }
  ~LinearUS()
  {
    if (solver != NULL) delete solver;
  }

  void search(); // LinearUS search.

  // Print solver configuration.
  void printConfiguration()
  {
    printf("c ==========================================[ Solver Settings "
           "]============================================\n");
    printf("c |                                                                "
           "                                       |\n");
    printf("c |  Algorithm: %23s                                             "
           "                      |\n",
           "LinearUS");
    print_Card_configuration(encoder.getCardEncoding());
    print_Incremental_configuration(incremental_strategy);
    printf("c |                                                                "
           "                                       |\n");
  }

protected:
  // Rebuild MaxSAT solver
  //
  Solver *rebuildSolver(); // Rebuild MaxSAT solver.

  // LinearUS search algorithms.
  //
  void lbSearch_none();
  void lbSearch_blocking();
  void lbSearch_weakening();
  void lbSearch_iterative();

  // Other
  void initRelaxation(); // Relaxes soft clauses.

  Solver *solver;  // SAT Solver used as a black box.
  Encoder encoder; // Interface for the encoder of constraints to CNF.

  int incremental_strategy;
  int encoding;

  // Literals to be used in the constraint that excludes models.
  vec<Lit> objFunction;
  vec<int> coeffs; // Coefficients of the literals that are used in the
                   // constraint that excludes models.
};
}

#endif

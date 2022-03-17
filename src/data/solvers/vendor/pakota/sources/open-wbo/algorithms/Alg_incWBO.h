/*****************************************************************************************[Alg_incWBO.h]
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

#ifndef Alg_incWBO_h
#define Alg_incWBO_h

#ifdef SIMP
#include "simp/SimpSolver.h"
#else
#include "core/Solver.h"
#endif

#include "Alg_WBO.h"
#include "../Encoder.h"
#include "../MaxTypes.h"
#include "utils/System.h"
#include <utility>
#include <map>
#include <set>

namespace NSPACE
{

class incWBO : public WBO
{

public:
  // Different AMO encodings can be easily plugged in.
  incWBO(int verb = _VERBOSITY_MINIMAL_, int weight = _WEIGHT_NONE_,
         bool symmetry = true, int limit = INT32_MAX, int enc = _AMO_LADDER_)
  {

    solver = NULL;
    verbosity = verb;

    nbCurrentSoft = 0;
    weightStrategy = weight;

    symmetryStrategy = symmetry;
    symmetryBreakingLimit = limit;

    firstBuild = true;
  }

  ~incWBO()
  {
    if (solver != NULL) delete solver;
  }

  void search(); // incWBO search.

  // Print solver configuration.
  void printConfiguration()
  {
    printf("c ==========================================[ Solver Settings "
           "]============================================\n");
    printf("c |                                                                "
           "                                       |\n");
    print_WBO_configuration(weightStrategy, symmetryStrategy,
                            symmetryBreakingLimit);
    print_Incremental_configuration(_INCREMENTAL_BLOCKING_);
    print_AMO_configuration(_AMO_LADDER_);
    printf("c |                                                                "
           "                                       |\n");
  }

protected:
  // Rebuild MaxSAT solver
  //
  void incrementalBuildWeightSolver(int strategy); // Incrementally build the
                                                   // MaxSAT solver with
                                                   // weight-based strategy.

  // Utils for core management
  //
  void relaxCore(vec<Lit> &conflict, int weightCore,
                 vec<Lit> &assumps); // Relaxes a core.

  // Symmetry breaking methods
  //
  void symmetryBreaking(); // Adds symmetry breaking clauses.

  // incWBO search
  //
  void weightSearch(); // Search using weight-based methods.
  void normalSearch(); // Original incWBO search.

  uint64_t incComputeCostModel(vec<lbool> &currentModel,
                               int weight = INT32_MAX);

  // // SAT solver
  Encoder encoder; // Interface for the encoder of constraints to CNF.

  // Incremental
  //
  vec<bool> incSoft; // Controls the polarity of the assumptions.
  // Controls the build of the solver.
  // The solver is only build once.
  bool firstBuild;
};
}

#endif

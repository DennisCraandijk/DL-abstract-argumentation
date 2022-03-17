/*****************************************************************************************[MaxSAT.h]
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

#ifndef MaxSAT_h
#define MaxSAT_h

#ifdef SIMP
#include "simp/SimpSolver.h"
#else
#include "core/Solver.h"
#endif

#include "Soft.h"
#include "Hard.h"
#include "MaxTypes.h"
#include "utils/System.h"
#include <utility>
#include <map>
#include <set>
#include <vector>
#include <algorithm>

namespace NSPACE
{

class MaxSAT
{

public:
  MaxSAT()
  {
    hardWeight = INT32_MAX;
    problemType = _UNWEIGHTED_;
    nbVars = 0;
    nbSoft = 0;
    nbHard = 0;
    nbInitialVariables = 0;

    currentWeight = 1;

    // 'ubCost' will be set to the sum of the weights of soft clauses
    //  during the parsing of the MaxSAT formula.
    ubCost = 0;
    lbCost = 0;

    // Statistics
    nbSymmetryClauses = 0;
    nbCores = 0;
    nbSatisfiable = 0;
    sumSizeCores = 0;
  }

  virtual ~MaxSAT()
  {
    for (int i = 0; i < softClauses.size(); i++)
    {
      softClauses[i].clause.clear();
      softClauses[i].relaxationVars.clear();
    }

    for (int i = 0; i < hardClauses.size(); i++)
      hardClauses[i].clause.clear();
  }

  int nVars();   // Number of variables.
  int nSoft();   // Number of soft clauses.
  int nHard();   // Number of hard clauses.
  void newVar(); // New variable.

  void addHardClause(vec<Lit> &lits);             // Add a new hard clause.
  void addSoftClause(int weight, vec<Lit> &lits); // Add a new soft clause.
  // Add a new soft clause with predefined relaxation variables.
  void addSoftClause(int weight, vec<Lit> &lits, vec<Lit> &vars);

  Lit newLiteral(bool sign = false); // Make a new literal.

  void setProblemType(int type); // Set problem type.
  int getProblemType();          // Get problem type.

  void updateSumWeights(int weight);   // Update initial 'ubCost'.
  void setCurrentWeight(int weight);   // Set initial 'currentWeight'.
  int getCurrentWeight();              // Get 'currentWeight'.
  void setHardWeight(int weight);      // Set initial 'hardWeight'.
  void setInitialTime(double initial); // Set initial time.

  // Print configuration of the MaxSAT solver.
  // virtual void printConfiguration();
  void printConfiguration();

  // Encoding information.
  void print_AMO_configuration(int encoding);
  void print_PB_configuration(int encoding);
  void print_Card_configuration(int encoding);

  // Incremental information.
  void print_Incremental_configuration(int incremental);

  virtual void search();      // MaxSAT search.
  void printAnswer(int type); // Print the answer.

  // Copy the soft and hard clauses to a new MaxSAT solver.
  void copySolver(MaxSAT *solver);

  // Tests if a MaxSAT formula has a lexicographical optimization criterion.
  bool isBMO(bool cache = true);

protected:
  // MaxSAT database
  //
  vec<Soft> softClauses; // Stores the soft clauses of the MaxSAT formula.
  vec<Hard> hardClauses; // Stores the hard clauses of the MaxSAT formula.

  // Interface with the SAT solver
  //
  Solver *newSATSolver(); // Creates a SAT solver.
  // Solves the formula that is currently loaded in the SAT solver.
  lbool searchSATSolver(Solver *S, vec<Lit> &assumptions, bool pre = false);
  lbool searchSATSolver(Solver *S, bool pre = false);

  void newSATVariable(Solver *S); // Creates a new variable in the SAT solver.

  // Properties of the MaxSAT formula
  //
  int hardWeight;         // Weight of the hard clauses.
  int problemType;        // Stores the type of the MaxSAT problem.
  int nbVars;             // Number of variables used in the SAT solver.
  int nbSoft;             // Number of soft clauses.
  int nbHard;             // Number of hard clauses.
  int nbInitialVariables; // Number of variables of the initial MaxSAT formula.
  vec<lbool> model;       // Stores the best satisfying model.

  // Statistics
  //
  int nbCores;           // Number of cores.
  int nbSymmetryClauses; // Number of symmetry clauses.
  uint64_t sumSizeCores; // Sum of the sizes of cores.
  int nbSatisfiable;     // Number of satisfiable calls.

  // Bound values
  //
  uint64_t ubCost; // Upper bound value.
  uint64_t lbCost; // Lower bound value.

  // Others
  int currentWeight;  // Initialized to the maximum weight of soft clauses.
  double initialTime; // Initial time.
  int verbosity;      // Controls the verbosity of the solver.

  // Different weights that corresponds to each function in the BMO algorithm.
  std::vector<uint64_t> orderWeights;

  // Utils for model management
  //
  void saveModel(vec<lbool> &currentModel); // Saves a Model.
  // Compute the cost of a model.
  uint64_t computeCostModel(vec<lbool> &currentModel, int weight = INT32_MAX);

  // Utils for printing
  //
  void printModel(); // Print the best satisfying model.
  void printStats(); // Print search statistics.

  // Greater than comparator.
  bool static greaterThan(int i, int j) { return (i > j); }
};
}

#endif

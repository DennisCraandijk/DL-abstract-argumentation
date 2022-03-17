/*****************************************************************************************[Alg_incWBO.cc]
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

#include "Alg_incWBO.h"

using namespace NSPACE;

/************************************************************************************************
 //
 // Rebuild MaxSAT solver
 //
 ************************************************************************************************/

/*_________________________________________________________________________________________________
  |
  |  incrementalBuildWeightSolver : (solver: Solver *) (strategy : int)
  |                                 -> [Solver *]
  |
  |   Description:
  |
  |    Rebuilds a SAT solver with the current MaxSAT formula using a
  |    weight-based strategy.
  |    Only soft clauses with weight greater or equal to 'currentWeight' are
  |    considered in the working MaxSAT formula.
  |
  |   For further details see:
  |     * Ruben Martins, Vasco Manquinho, Ines Lynce: On Partitioning for
  |       Maximum Satisfiability. ECAI 2012: 913-914
  |	    * Carlos Ansótegui, Maria Luisa Bonet, Joel Gabàs, Jordi Levy:
  |       Improving SAT-Based Weighted MaxSAT Solvers. CP 2012: 86-101
  |
  |   Pre-conditions:
  |     * Assumes that 'currentWeight' has been previously updated.
  |     * Assumes that the weight strategy is either '_WEIGHT_NORMAL_' or
  |       '_WEIGHT_DIVERSIFY_'.
  |
  |   Post-conditions:
  |     * 'nbCurrentSoft' is updated to the number of soft clauses in the
  |        working MaxSAT formula.
  |
  |________________________________________________________________________________________________@*/
void incWBO::incrementalBuildWeightSolver(int strategy)
{

  assert(strategy == _WEIGHT_NORMAL_ || strategy == _WEIGHT_DIVERSIFY_);

  // Only build the solver in the first iteration.
  if (firstBuild)
  {
    if (solver != NULL)
    {
      delete solver;
    }
    solver = newSATSolver();

    for (int i = 0; i < nVars(); i++)
      newSATVariable(solver);

    for (int i = 0; i < nHard(); i++)
      solver->addClause(hardClauses[i].clause);

    if (symmetryStrategy) symmetryBreaking();

    firstBuild = false;
  }

  // For the remaining iterations clauses are added to the solver incrementally.
  vec<Lit> clause;
  nbCurrentSoft = 0;
  for (int i = 0; i < nSoft(); i++)
  {
    if (softClauses[i].weight >= currentWeight && softClauses[i].weight)
    {
      nbCurrentSoft++;
      clause.clear();
      softClauses[i].clause.copyTo(clause);
      for (int j = 0; j < softClauses[i].relaxationVars.size(); j++)
        clause.push(softClauses[i].relaxationVars[j]);
      clause.push(softClauses[i].assumptionVar);

      solver->addClause(clause);
    }
  }
}

/************************************************************************************************
 //
 // Utils for core management
 //
 ************************************************************************************************/

/*_________________________________________________________________________________________________
  |
  |  relaxCore : (conflict : vec<Lit>&) (weightCore : int) (assumps : vec<Lit>&)
  |              ->  [void]
  |
  |  Description:
  |
  |    Relaxes the core as described in the original incWBO paper.
  |
  |  For further details see:
  |    * Vasco Manquinho, Joao Marques-Silva, Jordi Planes: Algorithms for
  |      Weighted Boolean Optimization. SAT 2009: 495-508
  |
  |  Pre-conditions:
  |    * Assumes that the core ('conflict') is not empty.
  |    * Assumes that the weight of the core is not 0 (should always be greater
  |      than or equal to 1).
  |
  |  Post-conditions:
  |    * If the weight of the soft clause is the same as the weight of the core:
  |      - 'softClauses[indexSoft].relaxationVars' is updated with a new
  |        relaxation variable.
  |    * If the weight of the soft clause is not the same as the weight of the
  |      core:
  |      - 'softClauses[indexSoft].weight' is decreased by the weight of the
  |        core.
  |      - A new soft clause is created. This soft clause has the weight of the
  |        core.
  |      - A new assumption literal is created and attached to the new soft
  |        clause.
  |      - 'coreMapping' is updated to map the new soft clause to its assumption
  |        literal.
  |    * 'sumSizeCores' is updated.
  |
  |________________________________________________________________________________________________@*/
void incWBO::relaxCore(vec<Lit> &conflict, int weightCore, vec<Lit> &assumps)
{

  assert(conflict.size() > 0);
  assert(weightCore > 0);

  vec<Lit> lits;

  for (int i = 0; i < conflict.size(); i++)
  {
    int indexSoft = coreMapping[conflict[i]];

    if (softClauses[indexSoft].weight == weightCore)
    {
      // If the weight of the soft clause is the same as the weight of the core
      // then relax it.

      vec<Lit> clause;
      softClauses[indexSoft].clause.copyTo(clause);
      vec<Lit> vars;
      softClauses[indexSoft].relaxationVars.copyTo(vars);

      Lit p = newLiteral();
      newSATVariable(solver);
      vars.push(p);
      lits.push(p);

      // Add a new soft clause with the weight of the core.
      addSoftClause(weightCore, clause, vars);

      Lit l = newLiteral();
      newSATVariable(solver);
      // Create a new assumption literal.
      softClauses[nSoft() - 1].assumptionVar = l;
      // Map the new soft clause to its assumption literal.
      coreMapping[l] = nSoft() - 1;
      incSoft[indexSoft] = true;
      incSoft.push(false);

      // Relaxed soft clause.
      for (int i = 0; i < vars.size(); i++)
        clause.push(vars[i]);
      clause.push(l);
      solver->addClause(clause);

      // Unit clause that disables the previous copy.
      clause.clear();
      clause.push(softClauses[indexSoft].assumptionVar);
      solver->addClause(clause);

      if (symmetryStrategy)
      {
        softMapping.push();
        relaxationMapping.push();
        new (&relaxationMapping[relaxationMapping.size() - 1]) vec<Lit>();
        new (&softMapping[softMapping.size() - 1]) vec<Lit>();
        softMapping[indexSoft].copyTo(softMapping[nSoft() - 1]);
        softMapping[indexSoft].clear();
        relaxationMapping[indexSoft].copyTo(relaxationMapping[nSoft() - 1]);
        relaxationMapping[indexSoft].clear();
        symmetryLog(nSoft() - 1);
      }
    }
    else
    {
      // If the weight of the soft clause is different from the weight of the
      // core then duplicate the soft clause.
      assert(softClauses[indexSoft].weight - weightCore > 0);
      softClauses[indexSoft].weight -=
          weightCore; // Update the weight of the soft clause.

      vec<Lit> clause;
      softClauses[indexSoft].clause.copyTo(clause);
      vec<Lit> vars;
      softClauses[indexSoft].relaxationVars.copyTo(vars);

      addSoftClause(softClauses[indexSoft].weight, clause,
                    vars); // Add old clause

      if (symmetryStrategy)
      {
        softMapping.push();
        relaxationMapping.push();
        new (&relaxationMapping[relaxationMapping.size() - 1]) vec<Lit>();
        new (&softMapping[softMapping.size() - 1]) vec<Lit>();
        softMapping[indexSoft].copyTo(softMapping[nSoft() - 1]);
        softMapping[indexSoft].clear();
        relaxationMapping[indexSoft].copyTo(relaxationMapping[nSoft() - 1]);
        relaxationMapping[indexSoft].clear();
      }

      incSoft[indexSoft] = true;
      Lit l = newLiteral();
      newSATVariable(solver);
      // Create a new assumption literal.
      softClauses[nSoft() - 1].assumptionVar = l;
      // Map the new soft clause to its assumption literal.
      coreMapping[l] = nSoft() - 1;
      incSoft.push(false);

      // Relaxed soft clause
      for (int i = 0; i < vars.size(); i++)
        clause.push(vars[i]);
      clause.push(l);
      solver->addClause(clause);

      clause.clear();
      vars.clear();
      softClauses[indexSoft].clause.copyTo(clause);
      softClauses[indexSoft].relaxationVars.copyTo(vars);

      l = newLiteral();
      newSATVariable(solver);
      vars.push(l);
      lits.push(l);

      // Add a new soft clause with the weight of the core.
      addSoftClause(weightCore, clause, vars);

      l = newLiteral();
      newSATVariable(solver);
      // Create a new assumption literal.
      softClauses[nSoft() - 1].assumptionVar = l;
      // Map the new soft clause to its assumption literal.
      coreMapping[l] = nSoft() - 1;
      incSoft.push(false);

      // Relaxed soft clause.
      for (int i = 0; i < vars.size(); i++)
        clause.push(vars[i]);
      clause.push(l);
      solver->addClause(clause);

      // Unit clause that disables the previous copy.
      clause.clear();
      clause.push(softClauses[indexSoft].assumptionVar);
      solver->addClause(clause);

      if (symmetryStrategy)
      {
        softMapping.push();
        relaxationMapping.push();
        new (&relaxationMapping[relaxationMapping.size() - 1]) vec<Lit>();
        new (&softMapping[softMapping.size() - 1]) vec<Lit>();
        symmetryLog(nSoft() - 1);
      }
    }
  }
  encoder.encodeAMO(solver, lits);
  nbVars = solver->nVars();
  if (symmetryStrategy) symmetryBreaking();

  sumSizeCores += conflict.size();
}

/************************************************************************************************
 //
 // Symmetry breaking methods
 //
 ************************************************************************************************/

/*_________________________________________________________________________________________________
  |
  |  symmetryBreaking : [void] ->  [void]
  |
  |  Description:
  |
  |    Adds binary clauses to the hard clauses to break symmetries between
  |    relaxation variables of soft clauses that have been relaxed multiple
  times.
  |    NOTE: This method may produce a large number of binary clauses.
  |    'symmetryBreakingLimit' can be used to control the maximum number of
  |     clauses added by this method.
  |
  |  For further details see:
  |    * Carlos Ansótegui, Maria Luisa Bonet, Joel Gabàs, Jordi Levy: Improving
  |      SAT-Based Weighted MaxSAT Solvers. CP 2012: 86-101
  |
  |  Post-conditions:
  |    * 'duplicatedSymmetry' is updated.
  |    * 'hardClauses' are updated with the binary clauses that break symmetries
  |       between relaxation variables.
  |    * 'nbSymmetryClauses' is updated.
  |
  |________________________________________________________________________________________________@*/
void incWBO::symmetryBreaking()
{

  if (indexSoftCore.size() != 0 && nbSymmetryClauses < symmetryBreakingLimit)
  {
    vec<Lit> *coreIntersection =
        new vec<Lit>[nbCores]; // Relaxation variables of soft clauses that
                               // appear in the current core stored by core
                               // index
    vec<Lit> *coreIntersectionCurrent =
        new vec<Lit>[nbCores]; // Relaxation variables of soft clauses that
                               // appear in previous cores that intersect with
                               // the current core stored by core index
    vec<int> coreList; // Indexes of cores that have soft clauses in common with
                       // the current core

    for (int i = 0; i < indexSoftCore.size(); i++)
    {
      int p = indexSoftCore[i];

      vec<int> addCores; // Cores where the soft clause with index 'p' appeared
      for (int j = 0; j < softMapping[p].size() - 1; j++)
      {
        int core = softMapping[p][j];
        addCores.push(core);
        if (coreIntersection[core].size() == 0) coreList.push(core);
        assert(j < relaxationMapping[p].size());
        assert(var(relaxationMapping[p][j]) > nbInitialVariables);
        coreIntersection[core].push(relaxationMapping[p][j]);
      }

      for (int j = 0; j < addCores.size(); j++)
      {
        int core = addCores[j];
        int b = softMapping[p].size() - 1;
        assert(b < relaxationMapping[p].size());
        assert(var(relaxationMapping[p][b]) > nbInitialVariables);
        coreIntersectionCurrent[core].push(relaxationMapping[p][b]);
      }

      for (int k = 0; k < coreList.size(); k++)
      {
        for (int i = 0; i < coreIntersection[coreList[k]].size(); i++)
        {
          for (int j = i + 1; j < coreIntersectionCurrent[coreList[k]].size();
               j++)
          {
            vec<Lit> clause;
            clause.push(~coreIntersection[coreList[k]][i]);
            clause.push(~coreIntersectionCurrent[coreList[k]][j]);

            // Symmetry clauses are binary clauses. This method may introduce
            // duplicated clauses.
            // The set 'duplicatedSymmetryClauses' is used to prevent adding
            // duplicated clauses.
            symmetryClause symClause;
            symClause.first = var(coreIntersection[coreList[k]][i]);
            symClause.second = var(coreIntersectionCurrent[coreList[k]][j]);
            if (var(coreIntersection[coreList[k]][i]) >
                var(coreIntersectionCurrent[coreList[k]][j]))
            {
              symClause.first = var(coreIntersectionCurrent[coreList[k]][j]);
              symClause.second = var(coreIntersection[coreList[k]][i]);
            }

            if (duplicatedSymmetryClauses.find(symClause) ==
                duplicatedSymmetryClauses.end())
            {
              duplicatedSymmetryClauses.insert(symClause);
              // addHardClause(clause);
              solver->addClause(clause);
              nbSymmetryClauses++;
              // If the number of symmetry clauses reached the limit then we
              // stop adding them.
              // NOTE:	This limit has not been proposed in the original paper.
              // It may be used here to prevent possible memory problems.
              if (symmetryBreakingLimit == nbSymmetryClauses) break;
            }
          }
          if (symmetryBreakingLimit == nbSymmetryClauses) break;
        }
        if (symmetryBreakingLimit == nbSymmetryClauses) break;
      }
      if (symmetryBreakingLimit == nbSymmetryClauses) break;
    }
    delete[] coreIntersection;
    delete[] coreIntersectionCurrent;
  }

  indexSoftCore.clear();
}

/************************************************************************************************
 //
 // incWBO search
 //
 ************************************************************************************************/

/*_________________________________________________________________________________________________
  |
  |  weightSearch : [void] ->  [void]
  |
  |  Description:
  |
  |    MaxSAT weight-based search. Considers the weights of soft clauses to find
  |    cores with larger weights first.
  |
  |  For further details see:
  |    * Ruben Martins, Vasco Manquinho, Inês Lynce: On Partitioning for Maximum
  |      Satisfiability. ECAI 2012: 913-914
  |    * Carlos Ansótegui, Maria Luisa Bonet, Joel Gabàs, Jordi Levy: Improving
  |      SAT-Based Weighted MaxSAT Solvers. CP 2012: 86-101
  |
  |  Pre-conditions:
  |    * Assumes 'weightStrategy' to be '_WEIGHT_NORMAL_' or
  |      '_WEIGHT_DIVERSIFY_'.
  |
  |  Post-conditions:
  |    * 'lbCost' is updated.
  |    * 'ubCost' is updated.
  |    * 'nbSatisfiable' is updated.
  |    * 'nbCores' is updated.
  |
  |________________________________________________________________________________________________@*/
void incWBO::weightSearch()
{

  assert(weightStrategy == _WEIGHT_NORMAL_ ||
         weightStrategy == _WEIGHT_DIVERSIFY_);

  unsatSearch();

  initAssumptions(assumptions);
  updateCurrentWeight(weightStrategy);
  incrementalBuildWeightSolver(weightStrategy);
  incSoft.growTo(nSoft(), false); // Controls the polarity of the assumptions.

  for (;;)
  {
    assumptions.clear();
    for (int i = 0; i < incSoft.size(); i++)
    {
      if (!incSoft[i]) assumptions.push(~softClauses[i].assumptionVar);
    }

    lbool res = searchSATSolver(solver, assumptions);

    if (res == l_False)
    {
      nbCores++;
      assert(solver->conflict.size() > 0);
      int coreCost = computeCostCore(solver->conflict);
      lbCost += coreCost;
      if (verbosity > 0)
        printf("c LB : %-12" PRIu64 " CS : %-12d W  : %-12d\n", lbCost,
               solver->conflict.size(), coreCost);
      relaxCore(solver->conflict, coreCost, assumptions);
      // delete solver.
      incrementalBuildWeightSolver(weightStrategy);
    }

    if (res == l_True)
    {
      nbSatisfiable++;
      if (nbCurrentSoft == nSoft())
      {
        assert(incComputeCostModel(solver->model) == lbCost);
        if (lbCost == ubCost && verbosity > 0) printf("c LB = UB\n");
        if (lbCost < ubCost)
        {
          ubCost = lbCost;
          saveModel(solver->model);
          printf("o %" PRIu64 "\n", lbCost);
        }
        printAnswer(_OPTIMUM_);
        exit(_OPTIMUM_);
      }
      else
      {
        updateCurrentWeight(weightStrategy);
        uint64_t cost = incComputeCostModel(solver->model);
        if (cost < ubCost)
        {
          ubCost = cost;
          saveModel(solver->model);
          printf("o %" PRIu64 "\n", ubCost);
        }

        if (lbCost == ubCost)
        {
          if (verbosity > 0) printf("c LB = UB\n");
          printAnswer(_OPTIMUM_);
          exit(_OPTIMUM_);
        }

        // delete solver.
        incrementalBuildWeightSolver(weightStrategy);
      }
    }
  }
}

/*_________________________________________________________________________________________________
  |
  |  computeCostModel : (incComputeCostModel : vec<lbool>&) (weight : int)
  |                     -> [uint64_t]
  |
  |  Description:
  |
  |    Computes the cost of 'currentModel'. The cost of a model is the sum of
  |    the weights of the unsatisfied soft clauses.
  |    If a weight is specified, then it only considers the sum of the weights
  |    of the unsatisfied soft clauses with the specified weight.
  |
  |  Pre-conditions:
  |    * Assumes that 'currentModel' is not empty.
  |
  |________________________________________________________________________________________________@*/
uint64_t incWBO::incComputeCostModel(vec<lbool> &currentModel, int weight)
{
  assert(currentModel.size() != 0);
  uint64_t currentCost = 0;

  for (int i = 0; i < nSoft(); i++)
  {
    bool unsatisfied = true;
    for (int j = 0; j < softClauses[i].clause.size(); j++)
    {

      if (incSoft[i])
      {
        unsatisfied = false;
        continue;
      }

      assert(var(softClauses[i].clause[j]) < currentModel.size());
      if ((sign(softClauses[i].clause[j]) &&
           currentModel[var(softClauses[i].clause[j])] == l_False) ||
          (!sign(softClauses[i].clause[j]) &&
           currentModel[var(softClauses[i].clause[j])] == l_True))
      {
        unsatisfied = false;
        break;
      }
    }

    if (unsatisfied) currentCost += softClauses[i].weight;
  }

  return currentCost;
}

/*_________________________________________________________________________________________________
  |
  |  normalSearch : [void] ->  [void]
  |
  |  Description:
  |
  |    Original incWBO algorithm.
  |
  |  For further details see:
  |    * Vasco Manquinho, Joao Marques-Silva, Jordi Planes: Algorithms for
  |      Weighted Boolean Optimization. SAT 2009: 495-508
  |
  |  Post-conditions:
  |    * 'lbCost' is updated.
  |    * 'ubCost' is updated.
  |    * 'nbSatisfiable' is updated.
  |    * 'nbCores' is updated.
  |
  |________________________________________________________________________________________________@*/
void incWBO::normalSearch()
{

  unsatSearch();

  initAssumptions(assumptions);
  solver = rebuildSolver();
  incSoft.growTo(nSoft(), false); // Controls the polarity of the assumptions.

  for (;;)
  {

    assumptions.clear();
    for (int i = 0; i < incSoft.size(); i++)
    {
      if (!incSoft[i]) assumptions.push(~softClauses[i].assumptionVar);
    }

    lbool res = searchSATSolver(solver, assumptions);

    if (res == l_False)
    {
      nbCores++;
      assert(solver->conflict.size() > 0);
      int coreCost = computeCostCore(solver->conflict);
      lbCost += coreCost;
      if (verbosity > 0)
        printf("c LB : %-12" PRIu64 " CS : %-12d W  : %-12d\n", lbCost,
               solver->conflict.size(), coreCost);

      if (lbCost == ubCost)
      {
        if (verbosity > 0) printf("c LB = UB\n");
        printAnswer(_OPTIMUM_);
        exit(_OPTIMUM_);
      }

      relaxCore(solver->conflict, coreCost, assumptions);
    }

    if (res == l_True)
    {
      nbSatisfiable++;
      ubCost = incComputeCostModel(solver->model);
      assert(lbCost == ubCost);
      printf("o %" PRIu64 "\n", lbCost);
      saveModel(solver->model);
      printAnswer(_OPTIMUM_);
      exit(_OPTIMUM_);
    }
  }
}

// Public search method
void incWBO::search()
{
  printConfiguration();

  nbInitialVariables = nVars();

  // If the maximum weight of the soft clauses is 1 then the problem is
  // considered unweighted.
  if (currentWeight == 1)
  {
    problemType = _UNWEIGHTED_;
    weightStrategy = _WEIGHT_NONE_;
  }

  if (symmetryStrategy) initSymmetry();

  if (problemType == _UNWEIGHTED_ || weightStrategy == _WEIGHT_NONE_)
    normalSearch();
  else if (weightStrategy == _WEIGHT_NORMAL_ ||
           weightStrategy == _WEIGHT_DIVERSIFY_)
    weightSearch();
}
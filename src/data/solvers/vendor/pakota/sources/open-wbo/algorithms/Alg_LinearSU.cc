/*****************************************************************************************[Linear.cc]
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

#include "Alg_LinearSU.h"

using namespace NSPACE;

/************************************************************************************************
 //
 // Linear Search Algorithm with Boolean Multilevel Optimization (BMO)
 //
 ************************************************************************************************/

/*_________________________________________________________________________________________________
  |
  |  bmoSearch : [void] ->  [void]
  |
  |  Description:
  |
  |    Linear search algorithm with lexicographical optimization.
  |
  |  For further details see:
  |    * Joao Marques-Silva, Josep Argelich, Ana Graça, Ines Lynce: Boolean
  |      lexicographic optimization: algorithms & applications. Ann. Math.
  |      Artif. Intell. 62(3-4): 317-343 (2011)
  |
  |  Post-conditions:
  |    * 'lbCost' is updated.
  |    * 'ubCost' is updated.
  |    * 'nbSatisfiable' is updated.
  |    * 'nbCores' is updated.
  |
  |________________________________________________________________________________________________@*/
void LinearSU::bmoSearch()
{
  assert(orderWeights.size() > 0);
  lbool res = l_True;

  initRelaxation();

  uint64_t currentWeight = orderWeights[0];
  uint64_t minWeight = orderWeights[orderWeights.size() - 1];
  int posWeight = 0;

  vec<vec<Lit> > functions;
  vec<int> weights;

  solver = rebuildBMO(functions, weights, currentWeight);

  uint64_t localCost = 0;
  ubCost = 0;

  for (;;)
  {

    vec<Lit> dummy;
    // Do not use preprocessing for linear search algorithm.
    // NOTE: When preprocessing is enabled the SAT solver simplifies the
    // relaxation variables which leads to incorrect results.
    res = searchSATSolver(solver, dummy);

    if (res == l_True)
    {
      nbSatisfiable++;

      uint64_t newCost = computeCostModel(solver->model, currentWeight);
      if (currentWeight == minWeight)
      {
        // If current weight is the same as the minimum weight, then we are in
        // the last lexicographical function.
        saveModel(solver->model);
        printf("o %" PRIu64 "\n", newCost + lbCost);
        ubCost = newCost + lbCost;
      }
      else
      {
        if (verbosity > 0)
          printf("c BMO-UB : %-12" PRIu64 "\t (Function %d/%d)\n", newCost,
                 posWeight + 1, (int)orderWeights.size());
      }

      if (newCost == 0 && currentWeight == minWeight)
      {
        // Optimum value has been found.
        printAnswer(_OPTIMUM_);
        exit(_OPTIMUM_);
      }
      else
      {

        if (newCost == 0)
        {

          functions.push();
          new (&functions[functions.size() - 1]) vec<Lit>();
          objFunction.copyTo(functions[functions.size() - 1]);

          localCost = newCost;
          weights.push(localCost / currentWeight);

          posWeight++;
          currentWeight = orderWeights[posWeight];
          localCost = 0;

          delete solver;
          solver = rebuildBMO(functions, weights, currentWeight);

          if (verbosity > 0) printf("c LB : %-12" PRIu64 "\n", lbCost);
        }
        else
        {

          // Optimization of the current lexicographical function.
          if (localCost == 0)
            encoder.encodeCardinality(solver, objFunction,
                                      newCost / currentWeight - 1);
          else
            encoder.updateCardinality(solver, newCost / currentWeight - 1);

          localCost = newCost;
        }
      }
    }
    else
    {
      nbCores++;

      if (currentWeight == minWeight)
      {
        // There are no more functions to be optimized.

        if (model.size() == 0)
        {
          assert(nbSatisfiable == 0);
          // If no model was found then the MaxSAT formula is unsatisfiable
          printAnswer(_UNSATISFIABLE_);
          exit(_UNSATISFIABLE_);
        }
        else
        {
          printAnswer(_OPTIMUM_);
          exit(_OPTIMUM_);
        }
      }
      else
      {

        // The current lexicographical function has been optimize. Go to the
        // next lexicographical function.
        functions.push();
        new (&functions[functions.size() - 1]) vec<Lit>();
        objFunction.copyTo(functions[functions.size() - 1]);

        weights.push(localCost / currentWeight);
        lbCost += localCost;

        posWeight++;
        currentWeight = orderWeights[posWeight];
        localCost = 0;

        delete solver;
        solver = rebuildBMO(functions, weights, currentWeight);

        if (verbosity > 0) printf("c LB : %-12" PRIu64 "\n", lbCost);
      }
    }
  }
}

/*_________________________________________________________________________________________________
  |
  |  normalSearch : [void] ->  [void]
  |
  |  Description:
  |
  |    Linear search algorithm.
  |
  |  For further details see:
  |    *  Daniel Le Berre, Anne Parrain: The Sat4j library, release 2.2. JSAT
  |       7(2-3): 59-6 (2010)
  |    *  Miyuki Koshimura, Tong Zhang, Hiroshi Fujita, Ryuzo Hasegawa: QMaxSAT:
  |       A Partial Max-SAT Solver. JSAT 8(1/2): 95-100 (2012)
  |
  |  Post-conditions:
  |    * 'ubCost' is updated.
  |    * 'nbSatisfiable' is updated.
  |    * 'nbCores' is updated.
  |
  |________________________________________________________________________________________________@*/
void LinearSU::normalSearch()
{

  lbool res = l_True;

  initRelaxation();
  solver = rebuildSolver();

  while (res == l_True)
  {

    vec<Lit> dummy;
    // Do not use preprocessing for linear search algorithm.
    // NOTE: When preprocessing is enabled the SAT solver simplifies the
    // relaxation variables which leads to incorrect results.
    res = searchSATSolver(solver, dummy);

    if (res == l_True)
    {
      nbSatisfiable++;
      uint64_t newCost = computeCostModel(solver->model);
      saveModel(solver->model);
      printf("o %" PRIu64 "\n", newCost);

      if (newCost == 0)
      {
        // If there is a model with value 0 then it is an optimal model
        ubCost = newCost;
        printAnswer(_OPTIMUM_);
        exit(_OPTIMUM_);
      }
      else
      {
        if (problemType == _WEIGHTED_)
        {
          if (!encoder.hasPBEncoding())
            encoder.encodePB(solver, objFunction, coeffs, newCost - 1);
          else
            encoder.updatePB(solver, newCost - 1);
        }
        else
        {
          // Unweighted.
          if (!encoder.hasCardEncoding())
            encoder.encodeCardinality(solver, objFunction, newCost - 1);
          else
            encoder.updateCardinality(solver, newCost - 1);
        }

        ubCost = newCost;
      }
    }
    else
    {
      nbCores++;
      if (model.size() == 0)
      {
        assert(nbSatisfiable == 0);
        // If no model was found then the MaxSAT formula is unsatisfiable
        printAnswer(_UNSATISFIABLE_);
        exit(_UNSATISFIABLE_);
      }
      else
      {
        printAnswer(_OPTIMUM_);
        exit(_OPTIMUM_);
      }
    }
  }
}

// Public search method
void LinearSU::search()
{

  nbInitialVariables = nVars();

  // If the maximum weight of the soft clauses is 1 then the problem is
  // considered unweighted.
  if (currentWeight == 1)
    problemType = _UNWEIGHTED_;
  else
    is_bmo = isBMO();

  printConfiguration(is_bmo, problemType);

  if (problemType == _WEIGHTED_)
  {
    if (bmoMode && is_bmo)
      bmoSearch();
    else
      normalSearch();
  }
  else
    normalSearch();
}

/************************************************************************************************
 //
 // Rebuild MaxSAT solver
 //
 ************************************************************************************************/

/*_________________________________________________________________________________________________
  |
  |  rebuildSolver : (minWeight : int)  ->  [Solver *]
  |
  |  Description:
  |
  |    Rebuilds a SAT solver with the current MaxSAT formula.
  |    If a weight is specified, then it only considers soft clauses with weight
  |    smaller than the specified weight.
  |    NOTE: a weight is specified in the 'bmo' approach.
  |
  |________________________________________________________________________________________________@*/
Solver *LinearSU::rebuildSolver(int minWeight)
{

  vec<bool> seen;
  seen.growTo(nVars(), false);

  Solver *S = newSATSolver();

  for (int i = 0; i < nVars(); i++)
    newSATVariable(S);

  for (int i = 0; i < nHard(); i++)
    S->addClause(hardClauses[i].clause);

  vec<Lit> clause;
  for (int i = 0; i < nSoft(); i++)
  {
    if (softClauses[i].weight < minWeight) continue;
    clause.clear();
    softClauses[i].clause.copyTo(clause);

    for (int j = 0; j < softClauses[i].relaxationVars.size(); j++)
    {
      clause.push(softClauses[i].relaxationVars[j]);
    }

    S->addClause(clause);
  }

  return S;
}

/*_________________________________________________________________________________________________
  |
  |  rebuildBMO : (functions : int)  ->  [Solver *]
  |
  |  Description:
  |
  |    Rebuilds a SAT solver with the current MaxSAT formula.
  |    Only considers soft clauses with the weight of the current
  |    lexicographical optimization weight ('currentWeight')
  |    For each function already computed in the BMO algorithm it encodes the
  |    respective cardinality constraint.
  |
  |________________________________________________________________________________________________@*/
Solver *LinearSU::rebuildBMO(vec<vec<Lit> > &functions, vec<int> &rhs,
                             int currentWeight)
{

  assert(functions.size() == rhs.size());

  Solver *S = rebuildSolver(currentWeight);

  objFunction.clear();
  coeffs.clear();
  for (int i = 0; i < nSoft(); i++)
  {
    if (softClauses[i].weight == (int)currentWeight)
    {
      objFunction.push(softClauses[i].relaxationVars[0]);
      coeffs.push(softClauses[i].weight);
    }
  }

  for (int i = 0; i < functions.size(); i++)
    encoder.encodeCardinality(S, functions[i], rhs[i]);

  return S;
}

/************************************************************************************************
 //
 // Other protected methods
 //
 ************************************************************************************************/

/*_________________________________________________________________________________________________
  |
  |  initRelaxation : (objective : vec<Lit>&) (weights : vec<int>&)  ->  [void]
  |
  |  Description:
  |
  |    Initializes the relaxation variables by adding a fresh variable to the
  |    'relaxationVars' of each soft clause.
  |
  |  Post-conditions:
  |    * 'objFunction' contains all relaxation variables that were added to soft
  |       clauses.
  |    * 'coeffs' contains the weights of all soft clauses.
  |
  |________________________________________________________________________________________________@*/
void LinearSU::initRelaxation()
{
  for (int i = 0; i < nbSoft; i++)
  {
    Lit l = newLiteral();
    softClauses[i].relaxationVars.push(l);
    objFunction.push(l);
    coeffs.push(softClauses[i].weight);
  }
}

// Print LinearSU configuration.
void LinearSU::print_LinearSU_configuration()
{
  printf("c |  Algorithm: %23s                                             "
         "                      |\n",
         "LinearSU");

  if (problemType == _WEIGHTED_)
  {
    if (bmoMode)
      printf("c |  BMO strategy: %20s                      "
             "                                             |\n",
             "On");
    else
      printf("c |  BMO strategy: %20s                      "
             "                                             |\n",
             "Off");

    if (bmoMode)
    {
      if (is_bmo)
        printf("c |  BMO search: %22s                      "
               "                                             |\n",
               "Yes");
      else
        printf("c |  BMO search: %22s                      "
               "                                             |\n",
               "No");
    }
  }
}

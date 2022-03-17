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

#include "Alg_WMSU3.h"

using namespace NSPACE;

/*_________________________________________________________________________________________________
  |
  |  WMSU3_iterative : [void] ->  [void]
  |
  |  Description:
  |
  |    Incremental iterative encoding for the WMSU3 algorithm.
  |    The PB encoding is build incrementally and the internal state of the
  |    SAT solver is kept across iterations.
  |
  |  For further details see:
  |    *  Ruben Martins, Saurabh Joshi, Vasco Manquinho, Ines Lynce:
  |       On Using Incremental Encodings in Unsatisfiability-based MaxSAT
  |       Solving. JSAT 2015: Special Issue on SAT 2014 Competitions and
  |       Evaluations (under submission).
  |
  |  Pre-conditions:
  |    * Assumes SWC is used as the PB encoding.
  |
  |  Post-conditions:
  |    * 'ubCost' is updated.
  |    * 'lbCost' is updated.
  |    * 'nbSatisfiable' is updated.
  |    * 'nbCores' is updated.
  |
  |________________________________________________________________________________________________@*/
void WMSU3::WMSU3_iterative()
{
  nbInitialVariables = nVars();
  lbool res = l_True;

  initRelaxation();
  solver = rebuildSolver();
  encoder.setIncremental(_INCREMENTAL_ITERATIVE_);

  activeSoft.growTo(nSoft(), false);
  for (int i = 0; i < nSoft(); i++)
    coreMapping[softClauses[i].assumptionVar] = i;

  assumptions.clear();

  vec<Lit> fullObjFunction;
  vec<int> fullCoeffsFunction;

  for (;;)
  {
    res = searchSATSolver(solver, assumptions);
    if (res == l_True)
    {
      nbSatisfiable++;
      uint64_t newCost = computeCostModel(solver->model);
      if (newCost < ubCost || nbSatisfiable == 1)
      {
        saveModel(solver->model);
        printf("o %" PRIu64 "\n", newCost);
        ubCost = newCost;
      }

      if (ubCost == 0 || lbCost == ubCost ||
          (currentWeight == 1 && nbSatisfiable > 1))
      {
        assert(lbCost == ubCost);
        assert(nbSatisfiable > 0);
        printAnswer(_OPTIMUM_);
        exit(_OPTIMUM_);
      }

      for (int i = 0; i < nSoft(); i++)
        if (softClauses[i].weight >= currentWeight && !activeSoft[i])
          assumptions.push(~softClauses[i].assumptionVar);
    }

    if (res == l_False)
    {

      nbCores++;

      // Assumes that the first SAT call is done with the soft clauses disabled.
      if (nbSatisfiable == 0)
      {
        printAnswer(_UNSATISFIABLE_);
        exit(_UNSATISFIABLE_);
      }

      if (lbCost == ubCost)
      {
        assert(nbSatisfiable > 0); // Otherwise, the problem is UNSAT.
        if (verbosity > 0) printf("c LB = UB\n");
        printAnswer(_OPTIMUM_);
        exit(_OPTIMUM_);
      }

      sumSizeCores += solver->conflict.size();

      objFunction.clear();
      coeffs.clear();
      assumptions.clear();

      for (int i = 0; i < solver->conflict.size(); i++)
      {
        if (coreMapping.count(solver->conflict[i]) == 0) continue;

        int index_soft = coreMapping[solver->conflict[i]];
        if (!activeSoft[index_soft])
        {
          activeSoft[index_soft] = true;
          objFunction.push(softClauses[index_soft].relaxationVars[0]);
          coeffs.push(softClauses[index_soft].weight);
        }
      }

      for (int i = 0; i < nSoft(); i++)
      {
        if (!activeSoft[i] && softClauses[i].weight >= currentWeight)
          assumptions.push(~softClauses[i].assumptionVar);
      }

      // This is only used for statistics.
      // Keeps track of active soft clauses.
      for (int i = 0; i < coeffs.size(); i++)
      {
        fullCoeffsFunction.push(coeffs[i]);
        fullObjFunction.push(objFunction[i]);
      }

      if (verbosity > 0)
        printf("c Relaxed soft clauses %d / %d\n", fullCoeffsFunction.size(),
               nSoft());

      lbCost++;
      while (!subsetSum(fullCoeffsFunction, lbCost))
      {
        lbCost++;
      }

      if (verbosity > 0) printf("c LB : %-12" PRIu64 "\n", lbCost);
      if (!encoder.hasPBEncoding())
      {
        encoder.incEncodePB(solver, objFunction, coeffs, lbCost, assumptions,
                            nSoft());
      }
      else
      {
        encoder.incUpdatePB(solver, objFunction, coeffs, lbCost, assumptions);
        encoder.incUpdatePBAssumptions(solver, assumptions);
      }
    }
  }
}

/*_________________________________________________________________________________________________
  |
  |  WMSU3_none : [void] ->  [void]
  |
  |  Description:
  |
  |    Non-incremental WMSU3 algorithm.
  |    Similar to the MSU3 algorithm but uses PB constraints instead of
  |    cardinality constraints.
  |
  |  For further details see:
  |    *  Joao Marques-Silva, Jordi Planes: On Using Unsatisfiability for
  |       Solving Maximum Satisfiability. CoRR abs/0712.1097 (2007)
  |
  |  Post-conditions:
  |    * 'ubCost' is updated.
  |    * 'lbCost' is updated.
  |    * 'nbSatisfiable' is updated.
  |    * 'nbCores' is updated.
  |
  |________________________________________________________________________________________________@*/
void WMSU3::WMSU3_none()
{
  nbInitialVariables = nVars();
  lbool res = l_True;

  initRelaxation();
  solver = rebuildSolver();
  encoder.setIncremental(_INCREMENTAL_NONE_);

  activeSoft.growTo(nSoft(), false);
  for (int i = 0; i < nSoft(); i++)
    coreMapping[softClauses[i].assumptionVar] = i;

  assumptions.clear();
  for (;;)
  {
    res = searchSATSolver(solver, assumptions);
    if (res == l_True)
    {
      nbSatisfiable++;
      uint64_t newCost = computeCostModel(solver->model);
      if (newCost < ubCost || nbSatisfiable == 1)
      {
        saveModel(solver->model);
        printf("o %" PRIu64 "\n", newCost);
        ubCost = newCost;
      }

      if (ubCost == 0 || lbCost == ubCost ||
          (currentWeight == 1 && nbSatisfiable > 1))
      {
        assert(nbSatisfiable > 0);
        printAnswer(_OPTIMUM_);
        exit(_OPTIMUM_);
      }

      for (int i = 0; i < nSoft(); i++)
        if (softClauses[i].weight >= currentWeight && !activeSoft[i])
          assumptions.push(~softClauses[i].assumptionVar);
    }

    if (res == l_False)
    {

      nbCores++;

      // Assumes that the first SAT call is done with the soft clauses disabled.
      if (nbSatisfiable == 0)
      {
        printAnswer(_UNSATISFIABLE_);
        exit(_UNSATISFIABLE_);
      }

      if (lbCost == ubCost)
      {
        assert(nbSatisfiable > 0); // Otherwise, the problem is UNSAT.
        if (verbosity > 0) printf("c LB = UB\n");
        printAnswer(_OPTIMUM_);
        exit(_OPTIMUM_);
      }

      sumSizeCores += solver->conflict.size();

      for (int i = 0; i < solver->conflict.size(); i++)
      {
        int index_soft = coreMapping[solver->conflict[i]];
        assert(!activeSoft[index_soft]);
        activeSoft[index_soft] = true;
      }

      objFunction.clear();
      coeffs.clear();
      assumptions.clear();
      for (int i = 0; i < nSoft(); i++)
      {
        // If a soft clause is active then is added to the 'currentObjFunction'
        if (activeSoft[i])
        {
          objFunction.push(softClauses[i].relaxationVars[0]);
          coeffs.push(softClauses[i].weight);
        }
        // Otherwise, the soft clause is not relaxed and the assumption is used
        // to get an unsat core.
        else if (softClauses[i].weight >= currentWeight)
          assumptions.push(~softClauses[i].assumptionVar);
      }

      if (verbosity > 0)
        printf("c Relaxed soft clauses %d / %d\n", objFunction.size(), nSoft());

      delete solver;
      solver = rebuildSolver();

      lbCost++;
      while (!subsetSum(coeffs, lbCost))
      {
        lbCost++;
      }

      if (verbosity > 0) printf("c LB : %-12" PRIu64 "\n", lbCost);
      encoder.encodePB(solver, objFunction, coeffs, lbCost);
    }
  }
}

/*_________________________________________________________________________________________________
  |
  |  WMSU3_iterative_bmo : [void] ->  [void]
  |
  |  Description:
  |
  |    Incremental iterative encoding for the WMSU3 algorithm with BMO
  |    criterion. Due to the BMO criterion, a cardinality constraint is used
  |    instead of a PB constraint.
  |
  |  For further details see:
  |    *  Ruben Martins, Saurabh Joshi, Vasco Manquinho, Ines Lynce:
  |       Incremental Cardinality Constraints for MaxSAT. CP 2014: 531-548
  |    *  Joao Marques-Silva, Josep Argelich, Ana Graça, Ines Lynce: Boolean
  |       lexicographic optimization: algorithms & applications. Ann. Math.
  |       Artif. Intell. 62(3-4): 317-343 (2011)
  |
  |
  |  Pre-conditions:
  |    * Assumes Totalizer is used as the cardinality encoding.
  |
  |  Post-conditions:
  |    * 'ubCost' is updated.
  |    * 'lbCost' is updated.
  |    * 'nbSatisfiable' is updated.
  |    * 'nbCores' is updated.
  |
  |________________________________________________________________________________________________@*/
void WMSU3::WMSU3_iterative_bmo()
{
  // Assumes that is_bmo was set by method 'isBMO()'.
  assert(is_bmo);

  nbInitialVariables = nVars();
  lbool res = l_True;

  initRelaxation();
  solver = rebuildSolver();
  encoder.setIncremental(_INCREMENTAL_ITERATIVE_);
  vec<Lit> joinObjFunction;
  vec<Lit> encodingAssumptions;
  vec<int> joinCoeffs;

  activeSoft.growTo(nSoft(), false);
  for (int i = 0; i < nSoft(); i++)
    coreMapping[softClauses[i].assumptionVar] = i;

  int64_t minWeight = 0;
  int posWeight = 0;

  uint64_t localCost = 0;

  // BMO
  vec<vec<Lit> > functions;
  vec<int> weights;

  vec<Encoder *> bmo_encodings;
  vec<bool> first_encoding;

  functions.push();
  new (&functions[functions.size() - 1]) vec<Lit>();
  weights.push(0);
  assert(objFunction.size() == 0);

  Encoder *e = new Encoder();
  e->setIncremental(_INCREMENTAL_ITERATIVE_);
  bmo_encodings.push(e);
  first_encoding.push(true);

  for (;;)
  {

    res = searchSATSolver(solver, assumptions);
    if (res == l_True)
    {
      nbSatisfiable++;
      uint64_t newCost = computeCostModel(solver->model);
      if (newCost < ubCost || nbSatisfiable == 1)
      {
        saveModel(solver->model);
        printf("o %" PRIu64 "\n", newCost);
        ubCost = newCost;
      }

      if (nbSatisfiable == 1)
      {
        if (ubCost == 0)
        {
          printAnswer(_OPTIMUM_);
          exit(_OPTIMUM_);
        }

        assert(orderWeights.size() > 0);
        assert(orderWeights[0] > 1);
        minWeight = orderWeights[orderWeights.size() - 1];
        currentWeight = orderWeights[0];

        for (int i = 0; i < nSoft(); i++)
          if (softClauses[i].weight >= currentWeight)
            assumptions.push(~softClauses[i].assumptionVar);
      }
      else
      {
        if (currentWeight == 1 || currentWeight == minWeight)
        {
          printAnswer(_OPTIMUM_);
          exit(_OPTIMUM_);
        }
        else
        {
          // Transform assumptions into unit clauses of
          // non-active soft clauses
          assumptions.clear();
          int64_t previousWeight = currentWeight;
          posWeight++;
          assert((unsigned)posWeight < orderWeights.size());
          currentWeight = orderWeights[posWeight];
          if (objFunction.size() > 0)
            objFunction.copyTo(functions[functions.size() - 1]);

          functions.push();
          new (&functions[functions.size() - 1]) vec<Lit>();
          weights.push(0);
          localCost = 0;

          Encoder *e = new Encoder();
          e->setIncremental(_INCREMENTAL_ITERATIVE_);
          bmo_encodings.push(e);
          first_encoding.push(true);

          for (int i = 0; i < encodingAssumptions.size(); i++)
            solver->addClause(encodingAssumptions[i]);
          encodingAssumptions.clear();

          for (int i = 0; i < nSoft(); i++)
          {
            if (!activeSoft[i] && previousWeight == softClauses[i].weight)
              solver->addClause(~softClauses[i].assumptionVar);

            if (currentWeight == softClauses[i].weight)
              assumptions.push(~softClauses[i].assumptionVar);

            if (activeSoft[i])
            {
              assert(softClauses[i].weight == previousWeight);
              activeSoft[i] = false;
            }
          }
        }
      }
    }

    if (res == l_False)
    {
      localCost++;
      lbCost += currentWeight;

      nbCores++;
      if (verbosity > 0) printf("c LB : %-12" PRIu64 "\n", lbCost);

      // Assumes that the first SAT call is done with the soft clauses disabled.
      if (nbSatisfiable == 0)
      {
        printAnswer(_UNSATISFIABLE_);
        exit(_UNSATISFIABLE_);
      }

      if (lbCost == ubCost)
      {
        assert(nbSatisfiable > 0); // Otherwise, the problem is UNSAT.
        if (verbosity > 0) printf("c LB = UB\n");
        printAnswer(_OPTIMUM_);
        exit(_OPTIMUM_);
      }

      sumSizeCores += solver->conflict.size();

      joinObjFunction.clear();
      joinCoeffs.clear();
      for (int i = 0; i < solver->conflict.size(); i++)
      {
        if (coreMapping.find(solver->conflict[i]) != coreMapping.end())
        {
          if (activeSoft[coreMapping[solver->conflict[i]]]) continue;
          assert(softClauses[coreMapping[solver->conflict[i]]].weight ==
                 currentWeight);
          activeSoft[coreMapping[solver->conflict[i]]] = true;
          joinObjFunction.push(
              softClauses[coreMapping[solver->conflict[i]]].relaxationVars[0]);
          joinCoeffs.push(softClauses[coreMapping[solver->conflict[i]]].weight);
        }
      }

      objFunction.clear();
      coeffs.clear();
      assumptions.clear();
      for (int i = 0; i < nSoft(); i++)
      {
        // If a soft clause is active then is added to the 'currentObjFunction'
        if (activeSoft[i])
        {
          assert(softClauses[i].weight == currentWeight);
          objFunction.push(softClauses[i].relaxationVars[0]);
          coeffs.push(softClauses[i].weight);
        }
        // Otherwise, the soft clause is not relaxed and the assumption is used
        // to get an unsat core.
        else if (currentWeight == softClauses[i].weight)
          assumptions.push(~softClauses[i].assumptionVar);
      }

      if (verbosity > 0)
        printf("c Relaxed soft clauses %d / %d\n", objFunction.size(), nSoft());

      assert(posWeight < functions.size());
      functions[posWeight].clear();
      objFunction.copyTo(functions[posWeight]);
      weights[posWeight] = localCost;

      if (first_encoding[posWeight])
      {
        if (weights[posWeight] != objFunction.size())
        {
          bmo_encodings[posWeight]->buildCardinality(solver, objFunction,
                                                     weights[posWeight]);
          joinObjFunction.clear();
          bmo_encodings[posWeight]->incUpdateCardinality(
              solver, joinObjFunction, objFunction, weights[posWeight],
              encodingAssumptions);
          first_encoding[posWeight] = false;
        }
      }
      else
      {
        bmo_encodings[posWeight]->incUpdateCardinality(
            solver, joinObjFunction, objFunction, weights[posWeight],
            encodingAssumptions);
      }

      for (int i = 0; i < encodingAssumptions.size(); i++)
        assumptions.push(encodingAssumptions[i]);
    }
  }
}

/*_________________________________________________________________________________________________
  |
  |  WMSU3_none_bmo : [void] ->  [void]
  |
  |  Description:
  |
  |    Non-incremental WMSU3 algorithm with BMO criterion.
  |    Due to the BMO criterion, a cardinality encoding is used instead of
  |    a PB encoding.
  |
  |  For further details see:
  |    *  Joao Marques-Silva, Jordi Planes: On Using Unsatisfiability for
  |       Solving Maximum Satisfiability. CoRR abs/0712.1097 (2007)
  |    *  Joao Marques-Silva, Josep Argelich, Ana Graça, Ines Lynce: Boolean
  |       lexicographic optimization: algorithms & applications. Ann. Math.
  |       Artif. Intell. 62(3-4): 317-343 (2011)
  |
  |
  |  Post-conditions:
  |    * 'ubCost' is updated.
  |    * 'lbCost' is updated.
  |    * 'nbSatisfiable' is updated.
  |    * 'nbCores' is updated.
  |
  |________________________________________________________________________________________________@*/
void WMSU3::WMSU3_none_bmo()
{
  // Assumes that is_bmo was set by method 'isBMO()'.
  assert(is_bmo);

  nbInitialVariables = nVars();
  lbool res = l_True;

  initRelaxation();
  solver = rebuildSolver();
  encoder.setIncremental(_INCREMENTAL_NONE_);

  activeSoft.growTo(nSoft(), false);
  for (int i = 0; i < nSoft(); i++)
    coreMapping[softClauses[i].assumptionVar] = i;

  int64_t minWeight = 0;
  unsigned posWeight = 0;

  uint64_t localCost = 0;

  // BMO
  vec<vec<Lit> > functions;
  vec<int> weights;

  functions.push();
  new (&functions[functions.size() - 1]) vec<Lit>();
  weights.push(0);
  assert(objFunction.size() == 0);

  for (;;)
  {

    res = searchSATSolver(solver, assumptions);
    if (res == l_True)
    {
      nbSatisfiable++;
      uint64_t newCost = computeCostModel(solver->model);
      if (newCost < ubCost || nbSatisfiable == 1)
      {
        saveModel(solver->model);
        printf("o %" PRIu64 "\n", newCost);
        ubCost = newCost;
      }

      if (nbSatisfiable == 1)
      {
        if (ubCost == 0)
        {
          printAnswer(_OPTIMUM_);
          exit(_OPTIMUM_);
        }

        assert(orderWeights.size() > 0);
        assert(orderWeights[0] > 1);
        minWeight = orderWeights[orderWeights.size() - 1];
        currentWeight = orderWeights[0];

        for (int i = 0; i < nSoft(); i++)
          if (softClauses[i].weight >= currentWeight)
            assumptions.push(~softClauses[i].assumptionVar);
      }
      else
      {
        if (currentWeight == 1 || currentWeight == minWeight)
        {
          printAnswer(_OPTIMUM_);
          exit(_OPTIMUM_);
        }
        else
        {
          // Transform assumptions into unit clauses of
          // non-active soft clauses
          assumptions.clear();
          int64_t previousWeight = currentWeight;
          posWeight++;
          assert(posWeight < orderWeights.size());
          currentWeight = orderWeights[posWeight];
          if (objFunction.size() > 0)
            objFunction.copyTo(functions[functions.size() - 1]);

          functions.push();
          new (&functions[functions.size() - 1]) vec<Lit>();
          weights.push(0);
          localCost = 0;

          for (int i = 0; i < nSoft(); i++)
          {
            if (!activeSoft[i] && previousWeight == softClauses[i].weight)
            {
              unit_bmo.push(~softClauses[i].assumptionVar);
              solver->addClause(~softClauses[i].assumptionVar);
            }

            if (currentWeight == softClauses[i].weight)
              assumptions.push(~softClauses[i].assumptionVar);

            if (activeSoft[i])
            {
              assert(softClauses[i].weight == previousWeight);
              activeSoft[i] = false;
            }
          }
        }
      }
    }

    if (res == l_False)
    {
      localCost++;
      lbCost += currentWeight;
      nbCores++;

      // Assumes that the first SAT call is done with the soft clauses disabled.
      if (nbSatisfiable == 0)
      {
        printAnswer(_UNSATISFIABLE_);
        exit(_UNSATISFIABLE_);
      }

      if (lbCost == ubCost)
      {
        assert(nbSatisfiable > 0); // Otherwise, the problem is UNSAT.
        if (verbosity > 0) printf("c LB = UB\n");
        printAnswer(_OPTIMUM_);
        exit(_OPTIMUM_);
      }

      sumSizeCores += solver->conflict.size();
      for (int i = 0; i < solver->conflict.size(); i++)
      {
        assert(softClauses[coreMapping[solver->conflict[i]]].weight ==
               currentWeight);
        activeSoft[coreMapping[solver->conflict[i]]] = true;
      }

      objFunction.clear();
      coeffs.clear();
      assumptions.clear();
      for (int i = 0; i < nSoft(); i++)
      {
        // If a soft clause is active then is added to the 'currentObjFunction'
        if (activeSoft[i])
        {
          assert(softClauses[i].weight == currentWeight);
          objFunction.push(softClauses[i].relaxationVars[0]);
          coeffs.push(softClauses[i].weight);
        }
        // Otherwise, the soft clause is not relaxed and the assumption is used
        // to get an unsat core.
        else if (currentWeight == softClauses[i].weight)
          assumptions.push(~softClauses[i].assumptionVar);
      }

      if (verbosity > 0)
        printf("c Relaxed soft clauses %d / %d\n", objFunction.size(), nSoft());

      delete solver;
      solver = rebuildSolver();
      for (int i = 0; i < unit_bmo.size(); i++)
        solver->addClause(unit_bmo[i]);

      if (verbosity > 0) printf("c LB : %-12" PRIu64 "\n", lbCost);
      assert(posWeight < (unsigned)functions.size());
      functions[posWeight].clear();
      objFunction.copyTo(functions[posWeight]);
      weights[posWeight] = localCost;

      // Encode functions.
      for (int i = 0; i < functions.size(); i++)
      {
        if (functions[i].size() > 0)
          encoder.encodeCardinality(solver, functions[i], weights[i]);
      }
    }
  }
}

// Public search method
void WMSU3::search()
{
  if (problemType == _UNWEIGHTED_)
  {
    printf("Error: Currently algorithm WMSU3 does not support unweighted MaxSAT"
           " instances.\n");
    printf("s UNKNOWN\n");
    exit(_ERROR_);
  }

  if (bmo_strategy) is_bmo = isBMO();
  printConfiguration();
  if (!is_bmo) currentWeight = 1; // No weight strategy none.

  switch (incremental_strategy)
  {
  case _INCREMENTAL_NONE_:
    if (is_bmo)
      WMSU3_none_bmo();
    else
      WMSU3_none();
    break;
  case _INCREMENTAL_ITERATIVE_:
    if (is_bmo)
    {
      if (encoder.getCardEncoding() != _CARD_TOTALIZER_)
      {
        printf("Error: Currently iterative encoding in WMSU3 only supports "
               "the Totalizer encoding.\n");
        printf("s UNKNOWN\n");
        exit(_ERROR_);
      }
      WMSU3_iterative_bmo();
    }
    else
      WMSU3_iterative();
    break;
  default:
    printf("Error: Invalid incremental strategy.\n");
    printf("s UNKNOWN\n");
    exit(_ERROR_);
  }
}

/************************************************************************************************
 //
 // Rebuild MaxSAT solver
 //
 ************************************************************************************************/

/*_________________________________________________________________________________________________
  |
  |  rebuildSolver : [void]  ->  [Solver *]
  |
  |  Description:
  |
  |    Rebuilds a SAT solver with the current MaxSAT formula.
  |
  |________________________________________________________________________________________________@*/
Solver *WMSU3::rebuildSolver()
{

  Solver *S = newSATSolver();

  for (int i = 0; i < nVars(); i++)
    newSATVariable(S);

  for (int i = 0; i < nHard(); i++)
    S->addClause(hardClauses[i].clause);

  vec<Lit> clause;
  for (int i = 0; i < nSoft(); i++)
  {
    clause.clear();
    softClauses[i].clause.copyTo(clause);
    for (int j = 0; j < softClauses[i].relaxationVars.size(); j++)
      clause.push(softClauses[i].relaxationVars[j]);

    S->addClause(clause);
  }

  return S;
}

/************************************************************************************************
 //
 // Other protected methods
 //
 ************************************************************************************************/

/*_________________________________________________________________________________________________
  |
  |  initRelaxation : [void] ->  [void]
  |
  |  Description:
  |
  |    Initializes the relaxation variables by adding a fresh variable to the
  |    'relaxationVars' of each soft clause.
  |
  |  Post-conditions:
  |    * 'objFunction' contains all relaxation variables that were added to soft
  |      clauses.
  |
  |________________________________________________________________________________________________@*/
void WMSU3::initRelaxation()
{
  for (int i = 0; i < nbSoft; i++)
  {
    Lit l = newLiteral();
    softClauses[i].relaxationVars.push(l);
    softClauses[i].assumptionVar = l;
  }
}

// Returns true if there is a subset of set[] with sum equal to given sum
bool WMSU3::subsetSum(vec<int> &set, int64_t sum)
{
  int n = set.size();
  
  // Declaring the array dynamically to avoid stack frame limit
  bool ** subset = new bool*[sum+1];
  for (int i = 0; i <= sum; i++)
    subset[i] = new bool[n+1];

  // The value of subset[i][j] will be true if there is a subset of set[0..j-1]
  //  with sum equal to i
  
  // If sum is 0, then answer is true
  for (int i = 0; i <= n; i++)
  {
    //assert (i < n+1);
    //printf("i %d\n",i);
    subset[0][i] = true;
  }

  // If sum is not 0 and set is empty, then answer is false
  for (int i = 1; i <= sum; i++)
    subset[i][0] = false;

  // Fill the subset table in bottom up manner
  for (int i = 1; i <= sum; i++)
  {
    for (int j = 1; j <= n; j++)
    {
      subset[i][j] = subset[i][j - 1];
      if (i >= set[j - 1])
        subset[i][j] = subset[i][j] || subset[i - set[j - 1]][j - 1];
    }
  }

  bool is_subset = subset[sum][n];
  for (int i = 0; i <= sum; i++)
    delete [] subset[i];
  delete [] subset;

  return is_subset;
}

// Print WSMU3 configuration.
void WMSU3::print_WMSU3_configuration()
{
  if (is_bmo)
  {
    if (bmo_strategy)
      printf("c |  BMO strategy: %20s                      "
             "                                             |\n",
             "On");
    else
      printf("c |  BMO strategy: %20s                      "
             "                                             |\n",
             "Off");

    printf("c |  BMO search: %22s                      "
           "                                             |\n",
           "Yes");
    print_Card_configuration(encoder.getCardEncoding());
  }
  else
  {
    if (bmo_strategy)
      printf("c |  BMO strategy: %20s                      "
             "                                             |\n",
             "On");
    else
      printf("c |  BMO strategy: %20s                      "
             "                                             |\n",
             "Off");

    printf("c |  BMO search: %22s                      "
           "                                             |\n",
           "No");
    print_PB_configuration(encoder.getPBEncoding());
  }
}

/*!
 * This file has been modified for use with the Pakota extension and status enforcement solver.
 * More specifically, some print and exit statements have been commented out and replaced by breaks.
 * 
 * - Andreas Niskanen
 */

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

#include "Alg_MSU3.h"

using namespace NSPACE;

/*_________________________________________________________________________________________________
  |
  |  MSU3_none : [void] ->  [void]
  |
  |  Description:
  |
  |    Non-incremental MSU3 algorithm.
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
void MSU3::MSU3_none()
{

  nbInitialVariables = nVars();
  lbool res = l_True;

  initRelaxation();
  solver = rebuildSolver();
  vec<Lit> assumptions;
  vec<Lit> currentObjFunction;
  encoder.setIncremental(_INCREMENTAL_NONE_);

  activeSoft.growTo(nSoft(), false);
  for (int i = 0; i < nSoft(); i++)
    coreMapping[softClauses[i].assumptionVar] = i;

  for (;;)
  {

    res = searchSATSolver(solver, assumptions);
    if (res == l_True)
    {
      nbSatisfiable++;
      uint64_t newCost = computeCostModel(solver->model);
      saveModel(solver->model);
      printf("o %" PRIu64 "\n", newCost);

      ubCost = newCost;

      if (nbSatisfiable == 1)
      {
        // The first SAT call is done with the soft clauses disabled.
        // This call is used to get an initial UB or to prove unsatisfiability
        // of the hard clauses.
        for (int i = 0; i < objFunction.size(); i++)
          assumptions.push(~objFunction[i]);
      }
      else
      {
        printAnswer(_OPTIMUM_);
        exit(_OPTIMUM_);
      }
    }

    if (res == l_False)
    {
      lbCost++;
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

      for (int i = 0; i < solver->conflict.size(); i++)
      {
        assert(!activeSoft[coreMapping[solver->conflict[i]]]);
        activeSoft[coreMapping[solver->conflict[i]]] = true;
      }

      currentObjFunction.clear();
      assumptions.clear();
      for (int i = 0; i < nSoft(); i++)
      {
        // If a soft clause is active then is added to the 'currentObjFunction'
        if (activeSoft[i])
          currentObjFunction.push(softClauses[i].relaxationVars[0]);
        // Otherwise, the soft clause is not relaxed and the assumption is used
        // to get an unsat core.
        else
          assumptions.push(~softClauses[i].assumptionVar);
      }

      if (verbosity > 0)
        printf("c Relaxed soft clauses %d / %d\n", currentObjFunction.size(),
               objFunction.size());

      delete solver;
      solver = rebuildSolver();
      encoder.encodeCardinality(solver, currentObjFunction, lbCost);
    }
  }
}

/*_________________________________________________________________________________________________
  |
  |  MSU3_blocking : [void] ->  [void]
  |
  |  Description:
  |
  |    Incremental blocking MSU3 algorithm.
  |    A blocking literal is added to each clause of the CNF encoding.
  |    At each iteration of the algorithm, a new CNF encoding is generated and
  |    previous iterations are disabled.
  |    This algorithm is similar to the non-incremental version of MSU3
  |    algorithm.
  |    It is advised to first check the non-incremental version and then check
  |    the differences.
  |
  |  For further details see:
  |    *  Ruben Martins, Saurabh Joshi, Vasco Manquinho, Ines Lynce:
  |       Incremental Cardinality Constraints for MaxSAT. CP 2014: 531-548
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
void MSU3::MSU3_blocking()
{
  assert(encoding == _CARD_TOTALIZER_);

  nbInitialVariables = nVars();
  lbool res = l_True;

  initRelaxation();
  solver = rebuildSolver();
  vec<Lit> assumptions;
  vec<Lit> encodingAssumptions;
  vec<Lit> currentObjFunction;
  encoder.setIncremental(_INCREMENTAL_BLOCKING_);

  vec<Lit> join; // dummy empty vector.

  activeSoft.growTo(nSoft(), false);
  for (int i = 0; i < nSoft(); i++)
    coreMapping[softClauses[i].assumptionVar] = i;

  for (;;)
  {

    res = searchSATSolver(solver, assumptions);
    if (res == l_True)
    {
      nbSatisfiable++;
      uint64_t newCost = computeCostModel(solver->model);
      saveModel(solver->model);
      printf("o %" PRIu64 "\n", newCost);

      ubCost = newCost;

      if (nbSatisfiable == 1)
      {
        for (int i = 0; i < objFunction.size(); i++)
          assumptions.push(~objFunction[i]);
      }
      else
      {
        printAnswer(_OPTIMUM_);
        exit(_OPTIMUM_);
      }
    }

    if (res == l_False)
    {
      lbCost++;
      nbCores++;
      if (verbosity > 0) printf("c LB : %-12" PRIu64 "\n", lbCost);

      if (nbSatisfiable == 0)
      {
        printAnswer(_UNSATISFIABLE_);
        exit(_UNSATISFIABLE_);
      }

      if (lbCost == ubCost)
      {
        assert(nbSatisfiable > 0);
        if (verbosity > 0) printf("c LB = UB\n");
        printAnswer(_OPTIMUM_);
        exit(_OPTIMUM_);
      }

      sumSizeCores += solver->conflict.size();

      for (int i = 0; i < solver->conflict.size(); i++)
      {
        if (coreMapping.find(solver->conflict[i]) != coreMapping.end())
        {
          assert(!activeSoft[coreMapping[solver->conflict[i]]]);
          activeSoft[coreMapping[solver->conflict[i]]] = true;
        }
      }

      currentObjFunction.clear();
      assumptions.clear();
      for (int i = 0; i < nSoft(); i++)
      {
        if (activeSoft[i])
          currentObjFunction.push(softClauses[i].relaxationVars[0]);
        else
          assumptions.push(~softClauses[i].assumptionVar);
      }

      if (verbosity > 0)
        printf("c Relaxed soft clauses %d / %d\n", currentObjFunction.size(),
               objFunction.size());

      // A new encoding is built at each iteration of the algorithm.
      encoder.buildCardinality(solver, currentObjFunction, lbCost);
      // Previous encodings are disabled by setting the
      // blocking literal to true in the 'incrementalUpdate' method.
      encoder.incUpdateCardinality(solver, join, objFunction, lbCost,
                                   encodingAssumptions);

      // Disable the blocking literal for the current encoding.
      for (int i = 0; i < encodingAssumptions.size(); i++)
        assumptions.push(encodingAssumptions[i]);
    }
  }
}

/*_________________________________________________________________________________________________
  |
  |  MSU3_weakening : [void] ->  [void]
  |
  |  Description:
  |
  |    Incremental weakening MSU3 algorithm.
  |    The cardinality constraint is only encoded once.
  |    Assumptions are used to restrict the values on the right-hand side of the
  |    cardinality constraint.
  |    This algorithm is similar to the non-incremental version of MSU3
  |    algorithm.
  |    It is advised to first check the non-incremental version and then check
  |    the differences.
  |
  |  For further details see:
  |    *  Ruben Martins, Saurabh Joshi, Vasco Manquinho, Ines Lynce:
  |       Incremental Cardinality Constraints for MaxSAT. CP 2014: 531-548
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
void MSU3::MSU3_weakening()
{

  nbInitialVariables = nVars();
  lbool res = l_True;

  initRelaxation();
  solver = rebuildSolver();
  vec<Lit> assumptions;
  vec<Lit> encodingAssumptions;
  vec<Lit> currentObjFunction;
  encoder.setIncremental(_INCREMENTAL_WEAKENING_);

  vec<Lit> join; // dummy empty vector.

  activeSoft.growTo(nSoft(), false);
  for (int i = 0; i < nSoft(); i++)
    coreMapping[softClauses[i].assumptionVar] = i;

  for (;;)
  {

    res = searchSATSolver(solver, assumptions);
    if (res == l_True)
    {
      nbSatisfiable++;
      uint64_t newCost = computeCostModel(solver->model);
      saveModel(solver->model);
      printf("o %" PRIu64 "\n", newCost);

      ubCost = newCost;

      if (nbSatisfiable == 1)
      {
        for (int i = 0; i < objFunction.size(); i++)
          assumptions.push(~objFunction[i]);
      }
      else
      {
        printAnswer(_OPTIMUM_);
        exit(_OPTIMUM_);
      }
    }

    if (res == l_False)
    {
      lbCost++;
      nbCores++;
      if (verbosity > 0) printf("c LB : %-12" PRIu64 "\n", lbCost);

      if (nbSatisfiable == 0)
      {
        printAnswer(_UNSATISFIABLE_);
        exit(_UNSATISFIABLE_);
      }

      if (lbCost == ubCost)
      {
        assert(nbSatisfiable > 0);
        if (verbosity > 0) printf("c LB = UB\n");
        printAnswer(_OPTIMUM_);
        exit(_OPTIMUM_);
      }

      sumSizeCores += solver->conflict.size();

      for (int i = 0; i < solver->conflict.size(); i++)
      {
        if (coreMapping.find(solver->conflict[i]) != coreMapping.end())
        {
          assert(!activeSoft[coreMapping[solver->conflict[i]]]);
          activeSoft[coreMapping[solver->conflict[i]]] = true;
        }
      }

      currentObjFunction.clear();
      assumptions.clear();
      for (int i = 0; i < nSoft(); i++)
      {
        if (activeSoft[i])
          currentObjFunction.push(softClauses[i].relaxationVars[0]);
        else
          assumptions.push(~softClauses[i].assumptionVar);
      }

      if (verbosity > 0)
        printf("c Relaxed soft clauses %d / %d\n", currentObjFunction.size(),
               objFunction.size());

      // The cardinality encoding is only build once.
      if (!encoder.hasCardEncoding())
        encoder.buildCardinality(solver, objFunction, ubCost);

      // At each iteration the right-hand side is updated using assumptions.
      // NOTE: 'encodingAsssumptions' is modified in 'incrementalUpdate'.
      encoder.incUpdateCardinality(solver, join, objFunction, lbCost,
                                   encodingAssumptions);

      for (int i = 0; i < encodingAssumptions.size(); i++)
        assumptions.push(encodingAssumptions[i]);
    }
  }
}

/*_________________________________________________________________________________________________
  |
  |  MSU3_iterative : [void] ->  [void]
  |
  |  Description:
  |
  |    Incremental iterative encoding for the MSU3 algorithm.
  |    The cardinality constraint is build incrementally in an iterative
  |    fashion.
  |    This algorithm is similar to the non-incremental version of MSU3
  |    algorithm.
  |    It is advised to first check the non-incremental version and then check
  |    the differences.
  |
  |  For further details see:
  |    *  Ruben Martins, Saurabh Joshi, Vasco Manquinho, Ines Lynce:
  |       Incremental Cardinality Constraints for MaxSAT. CP 2014: 531-548
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
void MSU3::MSU3_iterative()
{

  if (encoding != _CARD_TOTALIZER_)
  {
    printf("Error: Currently algorithm MSU3 with iterative encoding only "
           "supports the totalizer encoding.\n");
    printf("s UNKNOWN\n");
    exit(_ERROR_);
  }

  nbInitialVariables = nVars();
  lbool res = l_True;
  initRelaxation();
  solver = rebuildSolver();
  vec<Lit> assumptions;
  vec<Lit> joinObjFunction;
  vec<Lit> currentObjFunction;
  vec<Lit> encodingAssumptions;
  encoder.setIncremental(_INCREMENTAL_ITERATIVE_);

  activeSoft.growTo(nSoft(), false);
  for (int i = 0; i < nSoft(); i++)
    coreMapping[softClauses[i].assumptionVar] = i;

  for (;;)
  {

    res = searchSATSolver(solver, assumptions);
    if (res == l_True)
    {
      nbSatisfiable++;
      uint64_t newCost = computeCostModel(solver->model);
      saveModel(solver->model);
      //printf("o %" PRIu64 "\n", newCost);

      ubCost = newCost;

      if (nbSatisfiable == 1)
      {
        for (int i = 0; i < objFunction.size(); i++)
          assumptions.push(~objFunction[i]);
      }
      else
      {
        //assert(lbCost == newCost);
        //printAnswer(_OPTIMUM_);
        break;
        //exit(_OPTIMUM_);
      }
    }

    if (res == l_False)
    {
      lbCost++;
      nbCores++;
      if (verbosity > 0) printf("c LB : %-12" PRIu64 "\n", lbCost);

      if (nbSatisfiable == 0)
      {
        //printAnswer(_UNSATISFIABLE_);
        break;
        //exit(_UNSATISFIABLE_);
      }

      if (lbCost == ubCost)
      {
        assert(nbSatisfiable > 0);
        if (verbosity > 0) printf("c LB = UB\n");
        //printAnswer(_OPTIMUM_);
        break;
        //exit(_OPTIMUM_);
      }

      sumSizeCores += solver->conflict.size();

      if (solver->conflict.size() == 0)
      {
        //printAnswer(_UNSATISFIABLE_);
        break;
        //exit(_UNSATISFIABLE_);
      }

      joinObjFunction.clear();
      for (int i = 0; i < solver->conflict.size(); i++)
      {
        if (coreMapping.find(solver->conflict[i]) != coreMapping.end())
        {
          assert(!activeSoft[coreMapping[solver->conflict[i]]]);
          activeSoft[coreMapping[solver->conflict[i]]] = true;
          joinObjFunction.push(
              softClauses[coreMapping[solver->conflict[i]]].relaxationVars[0]);
        }
      }

      currentObjFunction.clear();
      assumptions.clear();
      for (int i = 0; i < nSoft(); i++)
      {
        if (activeSoft[i])
          currentObjFunction.push(softClauses[i].relaxationVars[0]);
        else
          assumptions.push(~softClauses[i].assumptionVar);
      }

      if (verbosity > 0)
        printf("c Relaxed soft clauses %d / %d\n", currentObjFunction.size(),
               objFunction.size());

      if (!encoder.hasCardEncoding())
      {
        if (lbCost != (unsigned)currentObjFunction.size())
        {
          encoder.buildCardinality(solver, currentObjFunction, lbCost);
          joinObjFunction.clear();
          encoder.incUpdateCardinality(solver, joinObjFunction,
                                       currentObjFunction, lbCost,
                                       encodingAssumptions);
        }
      }
      else
      {
        encoder.incUpdateCardinality(solver, joinObjFunction,
                                     currentObjFunction, lbCost,
                                     encodingAssumptions);
      }

      for (int i = 0; i < encodingAssumptions.size(); i++)
        assumptions.push(encodingAssumptions[i]);
    }
  }
}

// Public search method
void MSU3::search()
{
  if (problemType == _WEIGHTED_)
  {
    printf("Error: Currently algorithm MSU3 does not support weighted MaxSAT "
           "instances.\n");
    printf("s UNKNOWN\n");
    exit(_ERROR_);
  }

  //printConfiguration();

  switch (incremental_strategy)
  {
  case _INCREMENTAL_NONE_:
    MSU3_none();
    break;
  case _INCREMENTAL_BLOCKING_:
    if (encoder.getCardEncoding() != _CARD_TOTALIZER_)
    {
      printf("Error: Currently incremental blocking in MSU3 only supports "
             "the Totalizer encoding.\n");
      printf("s UNKNOWN\n");
      exit(_ERROR_);
    }
    MSU3_blocking();
    break;
  case _INCREMENTAL_WEAKENING_:
    if (encoder.getCardEncoding() != _CARD_TOTALIZER_)
    {
      printf("Error: Currently incremental weakening in MSU3 only supports "
             "the Totalizer encoding.\n");
      printf("s UNKNOWN\n");
      exit(_ERROR_);
    }
    MSU3_weakening();
    break;
  case _INCREMENTAL_ITERATIVE_:
    if (encoder.getCardEncoding() != _CARD_TOTALIZER_)
    {
      printf("Error: Currently iterative encoding in MSU3 only supports "
             "the Totalizer encoding.\n");
      printf("s UNKNOWN\n");
      exit(_ERROR_);
    }
    MSU3_iterative();
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
Solver *MSU3::rebuildSolver()
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
void MSU3::initRelaxation()
{
  for (int i = 0; i < nbSoft; i++)
  {
    Lit l = newLiteral();
    softClauses[i].relaxationVars.push(l);
    softClauses[i].assumptionVar = l;
    objFunction.push(l);
  }
}

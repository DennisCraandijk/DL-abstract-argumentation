/*!
 * Copyright (c) <2018> <Andreas Niskanen, University of Helsinki>
 * 
 * 
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * 
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * 
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "OpenWBOSolver.h"
#include <iostream>

using namespace std;

using namespace openwbo;

OpenWBOSolver::OpenWBOSolver()
{
    initial_time = cpuTime();
	formula = new MaxSATFormula();
    formula->setProblemType(_WEIGHTED_);
}

void OpenWBOSolver::build_solver(int hard_weight)
{
    formula->setHardWeight(hard_weight);
    formula->setProblemType(_UNWEIGHTED_);
    formula->setFormat(_FORMAT_MAXSAT_);
    MaxSAT * S = NULL;
    S = new MSU3(_VERBOSITY_MINIMAL_);
    S->loadFormula(formula);
    S->setPrintModel(false);
    S->setInitialTime(initial_time);
    mxsolver = S;
}

void OpenWBOSolver::add_hard_clause(vector<int> & clause)
{
	vec<Lit> lits;
    for (int i = 0; i < clause.size(); i++) {
        int var = abs(clause[i])-1;
        while (var >= formula->nVars())
            formula->newVar();
        lits.push((clause[i] > 0) ? mkLit(var) : ~mkLit(var));
    }
    formula->addHardClause(lits);
    hard_clauses.push_back(clause);
}

void OpenWBOSolver::add_soft_clause(int weight, vector<int> & clause)
{
	vec<Lit> lits;
	for (int i = 0; i < clause.size(); i++) {
        int var = abs(clause[i])-1;
        while (var >= formula->nVars())
            formula->newVar();
        lits.push((clause[i] > 0) ? mkLit(var) : ~mkLit(var));
    }
    formula->setMaximumWeight(weight);
    formula->updateSumWeights(weight);
    formula->addSoftClause(weight, lits);
    soft_clauses.push_back(make_pair(weight, clause));
}

void OpenWBOSolver::solve()
{
    formula_stored = formula->copyMaxSATFormula();
    mxsolver->search();
    for (int i = 0; i < mxsolver->model.size(); i++) {
        assignment[i] = (mxsolver->model[i] == l_True) ? 1 : 0;
    }
    formula = formula_stored;
}
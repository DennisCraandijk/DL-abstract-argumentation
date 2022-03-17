/*!
 * Copyright (c) <2017> <Andreas Niskanen, University of Helsinki>
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

using namespace std;

using namespace NSPACE;

OpenWBOSolver::OpenWBOSolver()
{
	double initial_time = cpuTime();
    solver = new MSU3(_VERBOSITY_MINIMAL_, _INCREMENTAL_ITERATIVE_, _CARD_TOTALIZER_);
    solver->setInitialTime(initial_time);
    solver->setProblemType(_UNWEIGHTED_);
}

void OpenWBOSolver::add_hard_clause(vector<int> & clause)
{
	vec<Lit> lits;
    for (int i = 0; i < clause.size(); i++) {
        int var = abs(clause[i])-1;
        while (var >= solver->nVars()) solver->newVar();
        lits.push((clause[i] > 0) ? mkLit(var) : ~mkLit(var));
    }
    solver->addHardClause(lits);
    hard_clauses.push_back(clause);
}

void OpenWBOSolver::add_soft_clause(int weight, vector<int> & clause)
{
	vec<Lit> lits;
	for (int i = 0; i < clause.size(); i++) {
        int var = abs(clause[i])-1;
        while (var >= solver->nVars()) solver->newVar();
        lits.push((clause[i] > 0) ? mkLit(var) : ~mkLit(var));
    }
    solver->setCurrentWeight(weight);
    solver->updateSumWeights(weight);
    solver->addSoftClause(weight, lits);
    soft_clauses.push_back(make_pair(weight, clause));
}

void OpenWBOSolver::solve()
{
	solver->search();
	for (int i = 0; i < solver->model.size(); i++) {
        assignment[i] = (solver->model[i] == l_True) ? 1 : 0;
    }
    delete solver;
    double initial_time = cpuTime();
    solver = new MSU3(_VERBOSITY_MINIMAL_, _INCREMENTAL_ITERATIVE_, _CARD_TOTALIZER_);
    solver->setInitialTime(initial_time);
    solver->setProblemType(_UNWEIGHTED_);
    for (int i = 0; i < hard_clauses.size(); i++) {
        vec<Lit> lits;
        for (int j = 0; j < hard_clauses[i].size(); j++) {
            int var = abs(hard_clauses[i][j])-1;
            while (var >= solver->nVars()) solver->newVar();
            lits.push((hard_clauses[i][j] > 0) ? mkLit(var) : ~mkLit(var));
        }
        solver->addHardClause(lits);
    }
    for (int i = 0; i < soft_clauses.size(); i++) {
        vec<Lit> lits;
        for (int j = 0; j < soft_clauses[i].second.size(); j++) {
            int var = abs(soft_clauses[i].second[j])-1;
            while (var >= solver->nVars()) solver->newVar();
            lits.push((soft_clauses[i].second[j] > 0) ? mkLit(var) : ~mkLit(var));
        }
        solver->addSoftClause(soft_clauses[i].first, lits);
    }
}
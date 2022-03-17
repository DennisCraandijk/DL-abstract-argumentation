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

#include "LMHSSolver.h"

using namespace std;

LMHSSolver::LMHSSolver()
{
    LMHS::initialize();
}

LMHSSolver::~LMHSSolver()
{
    LMHS::reset();
}

void LMHSSolver::add_hard_clause(vector<int> & clause)
{
	LMHS::addHardClause(clause);
    hard_clauses.push_back(clause);
}

void LMHSSolver::add_soft_clause(int weight, vector<int> & clause)
{
	LMHS::addSoftClause(weight, clause);
    soft_clauses.push_back(make_pair(weight, clause));
}

void LMHSSolver::solve()
{
    double cost = 0.0;
    vector<int> solution;
	LMHS::getOptimalSolution(cost, solution);
    for (int i = 0; i < solution.size(); i++) {
        assignment[i] = (solution[i] > 0 ? 1 : 0);
    }
}
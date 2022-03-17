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

#if defined(MAXSAT_OPENWBO)
#include "OpenWBOSolver.h"
typedef OpenWBOSolver MaxSAT_Solver;
#elif defined(MAXSAT_LMHS)
#include "LMHSSolver.h"
typedef LMHSSolver MaxSAT_Solver;
#else
#error "No MaxSAT solver defined"
#endif

#if defined(SAT_MINISAT)
#include "MiniSATSolver.h"
typedef MiniSATSolver SAT_Solver;
#elif defined(SAT_OPENWBO)
#include "OpenWBOSATSolver.h"
typedef OpenWBOSATSolver SAT_Solver;
#else
#error "No SAT solver defined"
#endif

#include "ArguFramework.h"
#include "Enumeration.h"

#include <iostream>
#include <fstream>

using namespace std;

namespace SkeptEnforcement {

/*!
 * Main function for credulous status enforcement. Stable semantics assumed.
 *
 * af - argumentation framework with enforcement request
 */
AF enforce(AF& af)
{
    vector<vector<int>> hard_clauses;
    vector<vector<int>> soft_clauses;
    int top = af.n_args*af.n_args+1;

    AF newAF;
    for (int i = 0; i < af.args.size(); i++) {
        newAF.addArgument(af.intToArg[af.args[i]]);
    }

    // if no arguments enforced negatively, enforce a stable extension containing positively enforced arguments
    if (af.neg_enfs.size() == 0) {
        for (int i = 0; i < af.args.size(); i++) {
            for (int j = 0; j < af.args.size(); j++) {
                if (!af.enforce[af.args[i]] && !af.enforce[af.args[j]]) {
                    vector<int> clause;
                    clause.push_back(-af.arg_var[make_pair(0, af.args[i])]);
                    clause.push_back(-af.arg_var[make_pair(0, af.args[j])]);
                    clause.push_back(-af.attToVar[make_pair(af.args[i], af.args[j])]);
                    hard_clauses.push_back(clause);
                }
                if (af.enforce[af.args[i]] && !af.enforce[af.args[j]]) {
                    vector<int> clause;
                    clause.push_back(-af.arg_var[make_pair(0, af.args[j])]);
                    clause.push_back(-af.attToVar[make_pair(af.args[i], af.args[j])]);
                    hard_clauses.push_back(clause);
                }
                if (!af.enforce[af.args[i]] && af.enforce[af.args[j]]) {
                    vector<int> clause;
                    clause.push_back(-af.arg_var[make_pair(0, af.args[i])]);
                    clause.push_back(-af.attToVar[make_pair(af.args[i], af.args[j])]);
                    hard_clauses.push_back(clause);
                }
            }
            if (!af.enforce[af.args[i]]) {
                vector<int> clause;
                clause.push_back(-af.arg_var[make_pair(0, af.args[i])]);
                clause.push_back(-af.attToVar[make_pair(af.args[i], af.args[i])]);
                hard_clauses.push_back(clause);
            }
        }
        for (int i = 0; i < af.args.size(); i++) {
            if (!af.enforce[af.args[i]]) {
                vector<int> clause;
                clause.push_back(af.arg_var[make_pair(0, af.args[i])]);
                for (int j = 0; j < af.args.size(); j++) {
                    if (!af.enforce[af.args[j]]) {
                        clause.push_back(af.att_var[make_pair(0, make_pair(af.args[j], af.args[i]))]);
                    }
                }
                for (int j = 0; j < af.enfs.size(); j++) {
                    clause.push_back(af.attToVar[make_pair(af.enfs[j], af.args[i])]);
                }
                hard_clauses.push_back(clause);
            }
        }
        for (int i = 0; i < af.args.size(); i++) {
            if (!af.enforce[af.args[i]]) {
                for (int j = 0; j < af.args.size(); j++) {
                    if (!af.enforce[af.args[j]]) {
                        vector<int> clause;
                        clause.push_back(-af.att_var[make_pair(0, make_pair(af.args[j], af.args[i]))]);
                        clause.push_back(af.arg_var[make_pair(0, af.args[j])]);
                        hard_clauses.push_back(clause);
                        clause.clear();
                        clause.push_back(-af.att_var[make_pair(0, make_pair(af.args[j], af.args[i]))]);
                        clause.push_back(af.attToVar[make_pair(af.args[j], af.args[i])]);
                        hard_clauses.push_back(clause);
                        clause.clear();
                        clause.push_back(-af.arg_var[make_pair(0, af.args[j])]);
                        clause.push_back(-af.attToVar[make_pair(af.args[j], af.args[i])]);
                        clause.push_back(af.att_var[make_pair(0, make_pair(af.args[j], af.args[i]))]);
                        hard_clauses.push_back(clause);
                    }
                }
            }
        }
    }

    // generate hard clauses
    for (int i = 0; i < af.neg_enfs.size(); i++) {
        for (int j = 0; j < af.args.size(); j++) {
            for (int k = 0; k < af.args.size(); k++) {
                if (af.args[j] != af.args[k] && af.args[j] != af.neg_enfs[i] && af.args[k] != af.neg_enfs[i]) {
                    if (!af.enforce[af.args[j]] && !af.enforce[af.args[k]]) {
                        vector<int> clause;
                        clause.push_back(-af.arg_var[make_pair(af.neg_enfs[i], af.args[j])]);
                        clause.push_back(-af.arg_var[make_pair(af.neg_enfs[i], af.args[k])]);
                        clause.push_back(-af.attToVar[make_pair(af.args[j], af.args[k])]);
                        hard_clauses.push_back(clause);
                    }
                    if (af.enforce[af.args[j]] && !af.enforce[af.args[k]]) {
                        vector<int> clause;
                        clause.push_back(-af.arg_var[make_pair(af.neg_enfs[i], af.args[k])]);
                        clause.push_back(-af.attToVar[make_pair(af.args[j], af.args[k])]);
                        hard_clauses.push_back(clause);
                    }
                    if (!af.enforce[af.args[j]] && af.enforce[af.args[k]]) {
                        vector<int> clause;
                        clause.push_back(-af.arg_var[make_pair(af.neg_enfs[i], af.args[j])]);
                        clause.push_back(-af.attToVar[make_pair(af.args[j], af.args[k])]);
                        hard_clauses.push_back(clause);
                    }
                }
            }
            if (af.args[j] != af.neg_enfs[i] && !af.enforce[af.args[j]]) {
                vector<int> clause;
                clause.push_back(-af.arg_var[make_pair(af.neg_enfs[i], af.args[j])]);
                clause.push_back(-af.attToVar[make_pair(af.args[j], af.args[j])]);
                hard_clauses.push_back(clause);
            }
        }
    }

    for (int i = 0; i < af.neg_enfs.size(); i++) {
        for (int j = 0; j < af.args.size(); j++) {
            if (!af.enforce[af.args[j]]) {
                vector<int> clause;
                if (af.neg_enfs[i] != af.args[j]) clause.push_back(af.arg_var[make_pair(af.neg_enfs[i], af.args[j])]);
                for (int k = 0; k < af.args.size(); k++) {
                    if (af.args[k] != af.neg_enfs[i] && !af.enforce[af.args[k]]) {
                        clause.push_back(af.att_var[make_pair(af.neg_enfs[i], make_pair(af.args[k], af.args[j]))]);
                    }
                }
                for (int k = 0; k < af.enfs.size(); k++) {
                    clause.push_back(af.attToVar[make_pair(af.enfs[k], af.args[j])]);
                }
                hard_clauses.push_back(clause);
            }
        }
    }

    for (int i = 0; i < af.neg_enfs.size(); i++) {
        for (int j = 0; j < af.args.size(); j++) {
            if (!af.enforce[af.args[j]]) {
                for (int k = 0; k < af.args.size(); k++) {
                    if (af.args[k] != af.neg_enfs[i] && !af.enforce[af.args[k]]) {
                        vector<int> clause;
                        clause.push_back(-af.att_var[make_pair(af.neg_enfs[i], make_pair(af.args[k], af.args[j]))]);
                        clause.push_back(af.arg_var[make_pair(af.neg_enfs[i], af.args[k])]);
                        hard_clauses.push_back(clause);
                        clause.clear();
                        clause.push_back(-af.att_var[make_pair(af.neg_enfs[i], make_pair(af.args[k], af.args[j]))]);
                        clause.push_back(af.attToVar[make_pair(af.args[k], af.args[j])]);
                        hard_clauses.push_back(clause);
                        clause.clear();
                        clause.push_back(-af.arg_var[make_pair(af.neg_enfs[i], af.args[k])]);
                        clause.push_back(-af.attToVar[make_pair(af.args[k], af.args[j])]);
                        clause.push_back(af.att_var[make_pair(af.neg_enfs[i], make_pair(af.args[k], af.args[j]))]);
                        hard_clauses.push_back(clause);
                    }
                }
            }
        }
    }

    // generate soft clauses
    for (int i = 0; i < af.args.size(); i++) {
        for (int j = 0; j < af.args.size(); j++) {
            if (!af.enforce[af.args[i]] || !af.enforce[af.args[j]]) {
                vector<int> clause;
                if (af.att_exists[make_pair(af.args[i], af.args[j])]) {
                    clause.push_back(af.attToVar[make_pair(af.args[i], af.args[j])]);
                } else {
                    clause.push_back(-af.attToVar[make_pair(af.args[i], af.args[j])]);
                }
                soft_clauses.push_back(clause);
            }
        }
    }

    // initialize MaxSAT solver
    MaxSAT_Solver maxsat_solver = MaxSAT_Solver();

    // add generated clauses to MaxSAT solver    
    for (int i = 0; i < hard_clauses.size(); i++) {
        maxsat_solver.add_hard_clause(hard_clauses[i]);
    }

    for (int i = 0; i < soft_clauses.size(); i++) {
        maxsat_solver.add_soft_clause(1, soft_clauses[i]);
    }

    // enter CEGAR loop
    int count = 0;
    while (true) {
        ++count;
        // compute optimal solution via MaxSAT
        maxsat_solver.solve();
        // initialize SAT solver
        SAT_Solver sat_solver = SAT_Solver();
        // construct the AF proposed by solution
        AF temp;
        for (int i = 0; i < af.args.size(); i++) {
            temp.addArgument(af.intToArg[af.args[i]]);
        }
        for (int i = 0; i < maxsat_solver.assignment.rbegin()->first; i++) {
            if (af.varToAtt.find(i+1) != af.varToAtt.end() && maxsat_solver.assignment[i]) {
                temp.addAttack(make_pair(af.intToArg[af.varToAtt[i+1].first], af.intToArg[af.varToAtt[i+1].second]));
            }
        }
        // generate clauses for SAT check
        vector<vector<int>> check_clauses = Enumeration::stable_clauses(temp);
        vector<int> clause;
        for (int i = 0; i < af.enfs.size(); i++) {
            clause.push_back(-af.enfs[i]);
        }
        check_clauses.push_back(clause);
        // add generated clauses to SAT solver
        for (int i = 0; i < check_clauses.size(); i++) {
            sat_solver.add_clause(check_clauses[i]);
        }
        // if satisfiable
        if (sat_solver.solve()) {
            // add refinement clause
            vector<int> clause;
            for (int i = 0; i < af.args.size(); i++) {
                for (int j = 0; j < af.args.size(); j++) {
                    if (!af.enforce[af.args[i]] || !af.enforce[af.args[j]]) {
                        if (temp.att_exists[make_pair(af.args[i], af.args[j])]) {
                            clause.push_back(-af.attToVar[make_pair(af.args[i], af.args[j])]);
                        } else {
                            clause.push_back(af.attToVar[make_pair(af.args[i], af.args[j])]);
                        }
                    }
                }
            }
            hard_clauses.push_back(clause);
            maxsat_solver.add_hard_clause(clause);
        // unsatisfiable - return optimal AF
        } else {
            cout << "Number of iterations (CEGAR): " << count << "\n";
            /*for (int i = 0; i < mxsolver->model.size(); i++) {
                if (af.var_arg.find(i+1) != af.var_arg.end() && mxsolver->model[i] == l_True) {
                    cout << af.intToArg[af.var_arg[i+1].first] << " " << af.intToArg[af.var_arg[i+1].second] << "\n";
                }
            }*/
            for (int i = 0; i < maxsat_solver.assignment.rbegin()->first; i++) {
                if (af.varToAtt.find(i+1) != af.varToAtt.end() && maxsat_solver.assignment[i]) {
                    newAF.addAttack(make_pair(af.intToArg[af.varToAtt[i+1].first], af.intToArg[af.varToAtt[i+1].second]));
                }
            }
            return newAF;
        }
    }
}

}

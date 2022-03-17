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

#if defined(MAXSAT_OPENWBO)
#include "OpenWBOSolver.h"
typedef OpenWBOSolver MaxSAT_Solver;
#else
#error "No MaxSAT solver defined"
#endif

#if defined(SAT_OPENWBO)
#include "OpenWBOSATSolver.h"
typedef OpenWBOSATSolver SAT_Solver;
#else
#error "No SAT solver defined"
#endif

#include "Enforcement.h"
#include "Grounded.h"

#include <iostream>
#include <fstream>

using namespace std;

namespace Enforcement {

/*!
 * MaxSAT clauses for strict enforcement under admissible semantics.
 */
vector<vector<int>> admissible_strict_clauses(AF& af)
{
    vector<vector<int>> clauses;
    for (int i = 0; i < af.args.size(); i++) {
        if (af.enforce[af.args[i]]) {
            for (int j = 0; j < af.args.size(); j++) {
                if (!af.enforce[af.args[j]]) {
                    vector<int> clause;
                    clause.push_back(-af.attToVar[make_pair(af.args[j], af.args[i])]);
                    for (int k = 0; k < af.args.size(); k++) {
                        if (af.enforce[af.args[k]]) {
                            clause.push_back(af.attToVar[make_pair(af.args[k], af.args[j])]);
                        }
                    }
                    clauses.push_back(clause);
                }
            }
        }
    }
    return clauses;
}

/*!
 * MaxSAT clauses for strict enforcement under complete semantics.
 */
vector<vector<int>> complete_strict_clauses(AF& af)
{
    vector<vector<int>> clauses = admissible_strict_clauses(af);
    for (int i = 0; i < af.args.size(); i++) {
        if (!af.enforce[af.args[i]]) {
            vector<int> clause;
            for (int j = 0; j < af.args.size(); j++) {
                if (!af.enforce[af.args[j]]) {
                    clause.push_back(af.attackVar[make_pair(af.args[i], af.args[j])]);
                } else {
                    clause.push_back(af.attToVar[make_pair(af.args[j], af.args[i])]);
                }
            }
            clauses.push_back(clause);
        }
    }
    for (int i = 0; i < af.args.size(); i++) {
        if (!af.enforce[af.args[i]]) {
            for (int j = 0; j < af.args.size(); j++) {
                if (!af.enforce[af.args[j]]) {
                    vector<int> clause;
                    clause.push_back(-af.attackVar[make_pair(af.args[i], af.args[j])]);
                    clause.push_back(af.attToVar[make_pair(af.args[j], af.args[i])]);
                    clauses.push_back(clause);
                }
            }
        }
    }
    for (int i = 0; i < af.args.size(); i++) {
        if (!af.enforce[af.args[i]]) {
            for (int j = 0; j < af.args.size(); j++) {
                if (!af.enforce[af.args[j]]) {
                    for (int k = 0; k < af.args.size(); k++) {
                        if (af.enforce[af.args[k]]) {
                            vector<int> clause;
                            clause.push_back(-af.attackVar[make_pair(af.args[i], af.args[j])]);
                            clause.push_back(-af.attToVar[make_pair(af.args[k], af.args[j])]);
                            clauses.push_back(clause);
                        }
                    }
                }
            }
        }
    }
    for (int i = 0; i < af.args.size(); i++) {
        if (!af.enforce[af.args[i]]) {
            for (int j = 0; j < af.args.size(); j++) {
                if (!af.enforce[af.args[j]]) {
                    vector<int> clause;
                    clause.push_back(-af.attToVar[make_pair(af.args[j], af.args[i])]);
                    for (int k = 0; k < af.args.size(); k++) {
                        if (af.enforce[af.args[k]]) {
                            clause.push_back(af.attToVar[make_pair(af.args[k], af.args[j])]);
                        }
                    }
                    clause.push_back(af.attackVar[make_pair(af.args[i], af.args[j])]);
                    clauses.push_back(clause);
                }
            }
        }
    }
    return clauses;
}

/*!
 * MaxSAT clauses for non-strict enforcement under conflict free semantics.
 */
vector<vector<int>> cf_non_strict_clauses(AF& af)
{
    vector<vector<int>> clauses;
    for (int i = 0; i < af.args.size(); i++) {
        for (int j = 0; j < af.args.size(); j++) {
            if (!af.enforce[af.args[i]] && !af.enforce[af.args[j]]) {
                vector<int> clause;
                clause.push_back(-af.attToVar[make_pair(af.args[i], af.args[j])]);
                clause.push_back(-af.argToVar[af.args[i]]);
                if (i != j) clause.push_back(-af.argToVar[af.args[j]]);
                clauses.push_back(clause);
            } else if (!af.enforce[af.args[i]]) {
                vector<int> clause;
                clause.push_back(-af.attToVar[make_pair(af.args[i], af.args[j])]);
                clause.push_back(-af.argToVar[af.args[i]]);
                clauses.push_back(clause);
            } else if (!af.enforce[af.args[j]]) {
                vector<int> clause;
                clause.push_back(-af.attToVar[make_pair(af.args[i], af.args[j])]);
                clause.push_back(-af.argToVar[af.args[j]]);
                clauses.push_back(clause);
            }
        }
    }
    return clauses;
}

/*!
 * MaxSAT clauses for non-strict enforcement under admissible semantics.
 */
vector<vector<int>> admissible_non_strict_clauses(AF& af)
{
    vector<vector<int>> clauses = cf_non_strict_clauses(af);
    for (int i = 0; i < af.args.size(); i++) {
        if (!af.enforce[af.args[i]]) {
            for (int j = 0; j < af.args.size(); j++) {
                if (!af.enforce[af.args[j]]) {
                    vector<int> clause;
                    clause.push_back(-af.attackedVar[make_pair(af.args[i], af.args[j])]);
                    for (int k = 0; k < af.args.size(); k++) {
                        if (!af.enforce[af.args[k]]) {
                            clause.push_back(af.attackVar[make_pair(af.args[k], af.args[j])]);
                        } else {
                            clause.push_back(af.attToVar[make_pair(af.args[k], af.args[j])]);
                        }
                    }
                    clauses.push_back(clause);
                }
            }
        } else {
            for (int j = 0; j < af.args.size(); j++) {
                if (!af.enforce[af.args[j]]) {
                    vector<int> clause;
                    clause.push_back(-af.attToVar[make_pair(af.args[j], af.args[i])]);
                    for (int k = 0; k < af.args.size(); k++) {
                        if (!af.enforce[af.args[k]]) {
                            clause.push_back(af.attackVar[make_pair(af.args[k], af.args[j])]);
                        } else {
                            clause.push_back(af.attToVar[make_pair(af.args[k], af.args[j])]);
                        }
                    }
                    clauses.push_back(clause);
                }
            }
        }
    }
    for (int i = 0; i < af.args.size(); i++) {
        for (int j = 0; j < af.args.size(); j++) {
            if (!af.enforce[af.args[i]] && !af.enforce[af.args[j]]) {
                vector<int> clause;
                clause.push_back(af.argToVar[af.args[i]]);
                clause.push_back(-af.attackedVar[make_pair(af.args[i], af.args[j])]);
                clauses.push_back(clause);
            }
        }
    }
    for (int i = 0; i < af.args.size(); i++) {
        for (int j = 0; j < af.args.size(); j++) {
            if (!af.enforce[af.args[i]] && !af.enforce[af.args[j]]) {
                vector<int> clause;
                clause.push_back(af.attToVar[make_pair(af.args[j], af.args[i])]);
                clause.push_back(-af.attackedVar[make_pair(af.args[i], af.args[j])]);
                clauses.push_back(clause);
            }
        }
    }
    for (int i = 0; i < af.args.size(); i++) {
        for (int j = 0; j < af.args.size(); j++) {
            if (!af.enforce[af.args[i]] && !af.enforce[af.args[j]]) {
                vector<int> clause;
                clause.push_back(-af.argToVar[af.args[i]]);
                clause.push_back(-af.attToVar[make_pair(af.args[j], af.args[i])]);
                clause.push_back(af.attackedVar[make_pair(af.args[i], af.args[j])]);
                clauses.push_back(clause);
            }
        }
    }
    for (int i = 0; i < af.args.size(); i++) {
        for (int j = 0; j < af.args.size(); j++) {
            if (!af.enforce[af.args[i]] && !af.enforce[af.args[j]]) {
                vector<int> clause;
                clause.push_back(af.argToVar[af.args[i]]);
                clause.push_back(-af.attackVar[make_pair(af.args[i], af.args[j])]);
                clauses.push_back(clause);
            }
        }
    }
    for (int i = 0; i < af.args.size(); i++) {
        for (int j = 0; j < af.args.size(); j++) {
            if (!af.enforce[af.args[i]] && !af.enforce[af.args[j]]) {
                vector<int> clause;
                clause.push_back(af.attToVar[make_pair(af.args[i], af.args[j])]);
                clause.push_back(-af.attackVar[make_pair(af.args[i], af.args[j])]);
                clauses.push_back(clause);
            }
        }
    }
    for (int i = 0; i < af.args.size(); i++) {
        for (int j = 0; j < af.args.size(); j++) {
            if (!af.enforce[af.args[i]] && !af.enforce[af.args[j]]) {
                vector<int> clause;
                clause.push_back(-af.argToVar[af.args[i]]);
                clause.push_back(-af.attToVar[make_pair(af.args[i], af.args[j])]);
                clause.push_back(af.attackVar[make_pair(af.args[i], af.args[j])]);
                clauses.push_back(clause);
            }
        }
    }
    return clauses;
}

/*!
 * MaxSAT clauses for strict enforcement under grounded semantics.
 */
vector<vector<int>> grounded_strict_clauses(AF& af)
{
    vector<vector<int>> clauses;
    // level one
    for (int i = 0; i < af.args.size(); i++) {
        if (af.enforce[af.args[i]]) {
            for (int j = 0; j < af.args.size(); j++) {
                if (!af.enforce[af.args[j]]) {
                    vector<int> clause;
                    clause.push_back(-af.level_var[make_pair(1, af.args[i])]);
                    clause.push_back(-af.attToVar[make_pair(af.args[j], af.args[i])]);
                    clauses.push_back(clause);
                }
            }
            vector<int> clause;
            clause.push_back(af.level_var[make_pair(1, af.args[i])]);
            for (int j = 0; j < af.args.size(); j++) {
                if (!af.enforce[af.args[j]]) {
                    clause.push_back(af.attToVar[make_pair(af.args[j], af.args[i])]);
                }
            }
            clauses.push_back(clause);
        }
    }
    // some argument is on first level
    vector<int> clause;
    for (int i = 0; i < af.args.size(); i++) {
        if (af.enforce[af.args[i]]) {
            clause.push_back(af.level_var[make_pair(1, af.args[i])]);
        }
    }
    clauses.push_back(clause);
    // levels n >= 2
    for (int n = 2; n <= af.enfs.size(); n++) {
        for (int i = 0; i < af.args.size(); i++) {
            if (af.enforce[af.args[i]]) {
                for (int j = 0; j < af.args.size(); j++) {
                    if (!af.enforce[af.args[j]]) {
                        vector<int> clause;
                        clause.push_back(-af.level_var[make_pair(n, af.args[i])]);
                        clause.push_back(-af.level_not_defended_var[make_pair(n-1, make_pair(af.args[j], af.args[i]))]);
                        clauses.push_back(clause);
                    }
                }
                vector<int> clause;
                clause.push_back(af.level_var[make_pair(n, af.args[i])]);
                for (int j = 0; j < af.args.size(); j++) {
                    if (!af.enforce[af.args[j]]) {
                        clause.push_back(af.level_not_defended_var[make_pair(n-1, make_pair(af.args[j], af.args[i]))]);
                    }
                }
                clauses.push_back(clause);
            }
        }
        // define "level attack var"
        for (int j = 0; j < af.args.size(); j++) {
            if (!af.enforce[af.args[j]]) {
                for (int k = 0; k < af.args.size(); k++) {
                    if (af.enforce[af.args[k]]) {
                        vector<int> clause;
                        clause.push_back(-af.level_attack_var[make_pair(n-1, make_pair(af.args[k], af.args[j]))]);
                        clause.push_back(af.attToVar[make_pair(af.args[k], af.args[j])]);
                        clauses.push_back(clause);
                        clause.clear();
                        clause.push_back(-af.level_attack_var[make_pair(n-1, make_pair(af.args[k], af.args[j]))]);
                        clause.push_back(af.level_var[make_pair(n-1, af.args[k])]);
                        clauses.push_back(clause);
                        clause.clear();
                        clause.push_back(af.level_attack_var[make_pair(n-1, make_pair(af.args[k], af.args[j]))]);
                        clause.push_back(-af.attToVar[make_pair(af.args[k], af.args[j])]);
                        clause.push_back(-af.level_var[make_pair(n-1, af.args[k])]);
                        clauses.push_back(clause);
                        clause.clear();
                    }
                }
            }
        }
        // define "level not defended var"
        for (int i = 0; i < af.args.size(); i++) {
            if (af.enforce[af.args[i]]) {
                for (int j = 0; j < af.args.size(); j++) {
                    if (!af.enforce[af.args[j]]) {
                        vector<int> clause;
                        clause.push_back(-af.level_not_defended_var[make_pair(n-1, make_pair(af.args[j], af.args[i]))]);
                        clause.push_back(af.attToVar[make_pair(af.args[j], af.args[i])]);
                        clauses.push_back(clause);
                        for (int k = 0; k < af.args.size(); k++) {
                            if (af.enforce[af.args[k]]) {
                                vector<int> clause;
                                clause.push_back(-af.level_not_defended_var[make_pair(n-1, make_pair(af.args[j], af.args[i]))]);
                                clause.push_back(-af.level_attack_var[make_pair(n-1, make_pair(af.args[k], af.args[j]))]);
                                clauses.push_back(clause);
                            }
                        }
                        clause.clear();
                        clause.push_back(af.level_not_defended_var[make_pair(n-1, make_pair(af.args[j], af.args[i]))]);
                        clause.push_back(-af.attToVar[make_pair(af.args[j], af.args[i])]);
                        for (int k = 0; k < af.args.size(); k++) {
                            if (af.enforce[af.args[k]]) {
                                clause.push_back(af.level_attack_var[make_pair(n-1, make_pair(af.args[k], af.args[j]))]);
                            }
                        }
                        clauses.push_back(clause);
                        clause.clear();
                    }
                }
            }
        }
    }
    // propagate levels
    for (int i = 0; i < af.args.size(); i++) {
        if (af.enforce[af.args[i]]) {
            for (int n = 2; n <= af.enfs.size(); n++) {
                vector<int> clause;
                clause.push_back(-af.level_var[make_pair(n-1, af.args[i])]);
                clause.push_back(af.level_var[make_pair(n, af.args[i])]);
                clauses.push_back(clause);
            }
        }
    }
    // now we still need to "not enforce" (from strict complete)
    for (int i = 0; i < af.args.size(); i++) {
        if (!af.enforce[af.args[i]]) {
            vector<int> clause;
            for (int j = 0; j < af.args.size(); j++) {
                if (!af.enforce[af.args[j]]) {
                    clause.push_back(af.attackVar[make_pair(af.args[i], af.args[j])]);
                } else {
                    clause.push_back(af.attToVar[make_pair(af.args[j], af.args[i])]);
                }
            }
            clauses.push_back(clause);
        }
    }
    for (int i = 0; i < af.args.size(); i++) {
        if (!af.enforce[af.args[i]]) {
            for (int j = 0; j < af.args.size(); j++) {
                if (!af.enforce[af.args[j]]) {
                    vector<int> clause;
                    clause.push_back(-af.attackVar[make_pair(af.args[i], af.args[j])]);
                    clause.push_back(af.attToVar[make_pair(af.args[j], af.args[i])]);
                    clauses.push_back(clause);
                }
            }
        }
    }
    for (int i = 0; i < af.args.size(); i++) {
        if (!af.enforce[af.args[i]]) {
            for (int j = 0; j < af.args.size(); j++) {
                if (!af.enforce[af.args[j]]) {
                    for (int k = 0; k < af.args.size(); k++) {
                        if (af.enforce[af.args[k]]) {
                            vector<int> clause;
                            clause.push_back(-af.attackVar[make_pair(af.args[i], af.args[j])]);
                            clause.push_back(-af.attToVar[make_pair(af.args[k], af.args[j])]);
                            clauses.push_back(clause);
                        }
                    }
                }
            }
        }
    }
    for (int i = 0; i < af.args.size(); i++) {
        if (!af.enforce[af.args[i]]) {
            for (int j = 0; j < af.args.size(); j++) {
                if (!af.enforce[af.args[j]]) {
                    vector<int> clause;
                    clause.push_back(-af.attToVar[make_pair(af.args[j], af.args[i])]);
                    for (int k = 0; k < af.args.size(); k++) {
                        if (af.enforce[af.args[k]]) {
                            clause.push_back(af.attToVar[make_pair(af.args[k], af.args[j])]);
                        }
                    }
                    clause.push_back(af.attackVar[make_pair(af.args[i], af.args[j])]);
                    clauses.push_back(clause);
                }
            }
        }
    }
    return clauses;
}

/*!
 * MaxSAT clauses for non-strict enforcement under grounded semantics.
 */
vector<vector<int>> grounded_non_strict_clauses(AF& af)
{
    vector<vector<int>> clauses;
    // level one
    for (int i = 0; i < af.args.size(); i++) {
        for (int j = 0; j < af.args.size(); j++) {
            if (!af.enforce[af.args[i]] || !af.enforce[af.args[j]]) {
                vector<int> clause;
                clause.push_back(-af.level_var[make_pair(1, af.args[i])]);
                clause.push_back(-af.attToVar[make_pair(af.args[j], af.args[i])]);
                clauses.push_back(clause);
            }
        }
        vector<int> clause;
        clause.push_back(af.level_var[make_pair(1, af.args[i])]);
        for (int j = 0; j < af.args.size(); j++) {
            if (!af.enforce[af.args[i]] || !af.enforce[af.args[j]]) {
                clause.push_back(af.attToVar[make_pair(af.args[j], af.args[i])]);
            }
        }
        clauses.push_back(clause);
    }
    // some argument is on first level
    vector<int> clause;
    for (int i = 0; i < af.args.size(); i++) {
        clause.push_back(af.level_var[make_pair(1, af.args[i])]);
    }
    clauses.push_back(clause);
    // levels n >= 2
    for (int n = 2; n <= (af.args.size()+1)/2; n++) {
        for (int i = 0; i < af.args.size(); i++) {
            for (int j = 0; j < af.args.size(); j++) {
                vector<int> clause;
                clause.push_back(-af.level_var[make_pair(n, af.args[i])]);
                clause.push_back(-af.level_not_defended_var[make_pair(n-1, make_pair(af.args[j], af.args[i]))]);
                clauses.push_back(clause);
            }
            vector<int> clause;
            clause.push_back(af.level_var[make_pair(n, af.args[i])]);
            for (int j = 0; j < af.args.size(); j++) {
                clause.push_back(af.level_not_defended_var[make_pair(n-1, make_pair(af.args[j], af.args[i]))]);
            }
            clauses.push_back(clause);
        }
        // define "level attack var"
        for (int j = 0; j < af.args.size(); j++) {
            for (int k = 0; k < af.args.size(); k++) {
                vector<int> clause;
                clause.push_back(-af.level_attack_var[make_pair(n-1, make_pair(af.args[k], af.args[j]))]);
                if (!af.enforce[af.args[k]] || !af.enforce[af.args[j]]) {
                    clause.push_back(af.attToVar[make_pair(af.args[k], af.args[j])]);
                }
                clauses.push_back(clause);
                clause.clear();
                clause.push_back(-af.level_attack_var[make_pair(n-1, make_pair(af.args[k], af.args[j]))]);
                clause.push_back(af.level_var[make_pair(n-1, af.args[k])]);
                clauses.push_back(clause);
                clause.clear();
                if (!af.enforce[af.args[k]] || !af.enforce[af.args[j]]) {
                    clause.push_back(af.level_attack_var[make_pair(n-1, make_pair(af.args[k], af.args[j]))]);
                    clause.push_back(-af.attToVar[make_pair(af.args[k], af.args[j])]);
                    clause.push_back(-af.level_var[make_pair(n-1, af.args[k])]);
                    clauses.push_back(clause);
                    clause.clear();
                }
            }
        }
        // define "level not defended var"
        for (int i = 0; i < af.args.size(); i++) {
            for (int j = 0; j < af.args.size(); j++) {
                vector<int> clause;
                clause.push_back(-af.level_not_defended_var[make_pair(n-1, make_pair(af.args[j], af.args[i]))]);
                if (!af.enforce[af.args[j]] || !af.enforce[af.args[i]]) {
                    clause.push_back(af.attToVar[make_pair(af.args[j], af.args[i])]);
                }
                clauses.push_back(clause);
                for (int k = 0; k < af.args.size(); k++) {
                    vector<int> clause;
                    clause.push_back(-af.level_not_defended_var[make_pair(n-1, make_pair(af.args[j], af.args[i]))]);
                    clause.push_back(-af.level_attack_var[make_pair(n-1, make_pair(af.args[k], af.args[j]))]);
                    clauses.push_back(clause);
                }
                clause.clear();
                if (!af.enforce[af.args[j]] || !af.enforce[af.args[i]]) {
                    clause.push_back(af.level_not_defended_var[make_pair(n-1, make_pair(af.args[j], af.args[i]))]);
                    clause.push_back(-af.attToVar[make_pair(af.args[j], af.args[i])]);
                    for (int k = 0; k < af.args.size(); k++) {
                        clause.push_back(af.level_attack_var[make_pair(n-1, make_pair(af.args[k], af.args[j]))]);
                    }
                    clauses.push_back(clause);
                }
            }
        }
    }
    // propagate levels
    for (int i = 0; i < af.args.size(); i++) {
        for (int n = 2; n <= (af.args.size()+1)/2; n++) {
            vector<int> clause;
            clause.push_back(-af.level_var[make_pair(n-1, af.args[i])]);
            clause.push_back(af.level_var[make_pair(n, af.args[i])]);
            clauses.push_back(clause);
        }
    }
    return clauses;
}

vector<vector<int>> level_one_clauses_strict(AF& af)
{
    vector<vector<int>> clauses;
    // level one
    for (int i = 0; i < af.args.size(); i++) {
        if (af.enforce[af.args[i]]) {
            for (int j = 0; j < af.args.size(); j++) {
                if (!af.enforce[af.args[j]]) {
                    vector<int> clause;
                    clause.push_back(-af.level_var[make_pair(1, af.args[i])]);
                    clause.push_back(-af.attToVar[make_pair(af.args[j], af.args[i])]);
                    clauses.push_back(clause);
                }
            }
            vector<int> clause;
            clause.push_back(af.level_var[make_pair(1, af.args[i])]);
            for (int j = 0; j < af.args.size(); j++) {
                if (!af.enforce[af.args[j]]) {
                    clause.push_back(af.attToVar[make_pair(af.args[j], af.args[i])]);
                }
            }
            clauses.push_back(clause);
        }
    }
    // some argument is on first level
    vector<int> clause;
    for (int i = 0; i < af.args.size(); i++) {
        if (af.enforce[af.args[i]]) {
            clause.push_back(af.level_var[make_pair(1, af.args[i])]);
        }
    }
    clauses.push_back(clause);
    return clauses;
}

vector<vector<int>> level_one_clauses_nonstrict(AF& af)
{
    vector<vector<int>> clauses;
    // level one
    for (int i = 0; i < af.args.size(); i++) {
        for (int j = 0; j < af.args.size(); j++) {
            if (!af.enforce[af.args[i]] || !af.enforce[af.args[j]]) {
                vector<int> clause;
                clause.push_back(-af.level_var[make_pair(1, af.args[i])]);
                clause.push_back(-af.attToVar[make_pair(af.args[j], af.args[i])]);
                clauses.push_back(clause);
            }
        }
        vector<int> clause;
        clause.push_back(af.level_var[make_pair(1, af.args[i])]);
        for (int j = 0; j < af.args.size(); j++) {
            if (!af.enforce[af.args[i]] || !af.enforce[af.args[j]]) {
                clause.push_back(af.attToVar[make_pair(af.args[j], af.args[i])]);
            }
        }
        clauses.push_back(clause);
    }
    // some argument is on first level
    vector<int> clause;
    for (int i = 0; i < af.args.size(); i++) {
        clause.push_back(af.level_var[make_pair(1, af.args[i])]);
    }
    clauses.push_back(clause);
    // if is root then is in extension
    for (int i = 0; i < af.args.size(); i++) {
        if (!af.enforce[af.args[i]]) {
            vector<int> clause;
            clause.push_back(-af.level_var[make_pair(1, af.args[i])]);
            clause.push_back(af.argToVar[af.args[i]]);
            clauses.push_back(clause);
        }
    }
    return clauses;
}

/*!
 * Wcnf standard output.
 *
 * nbvar - number of variables
 * nbclauses - number of clauses
 * top - weight assigned to hard clauses (number of soft clauses + 1)
 * hard_clauses - hard clauses in MaxSAT instance
 * soft_clauses - soft clauses in MaxSAT instance (unit weights assumed)
 * filename - name of output file
 */
void write_wcnf_to_stdout(int nbvar, int nbclauses, double top, 
                   vector<vector<int>>& hard_clauses, 
                   vector<vector<int>>& soft_clauses)
{
    cout << "p wcnf " << nbvar << " " << nbclauses << " " << top << "\n";
    for (int i = 0; i < hard_clauses.size(); i++) {
        cout << top << " ";
        for (int j = 0; j < hard_clauses[i].size(); j++) {
            cout << hard_clauses[i][j] << " ";
        }
        cout << "0\n";
    }
    for (int i = 0; i < soft_clauses.size(); i++) {
        cout << 1 << " ";
        for (int j = 0; j < soft_clauses[i].size(); j++) {
            cout << soft_clauses[i][j] << " ";
        }
        cout << "0\n";
    }
}

/*!
 * Wcnf file output.
 *
 * nbvar - number of variables
 * nbclauses - number of clauses
 * top - weight assigned to hard clauses (number of soft clauses + 1)
 * hard_clauses - hard clauses in MaxSAT instance
 * soft_clauses - soft clauses in MaxSAT instance (unit weights assumed)
 * filename - name of output file
 */
bool write_wcnf_to_file(int nbvar, int nbclauses, double top, 
                   vector<vector<int>>& hard_clauses, 
                   vector<vector<int>>& soft_clauses, 
                   string filename)
{
    ofstream file(filename);
    if (file.is_open()) {
        file << "p wcnf " << nbvar << " " << nbclauses << " " << top << "\n";
        for (int i = 0; i < hard_clauses.size(); i++) {
            file << top << " ";
            for (int j = 0; j < hard_clauses[i].size(); j++) {
                file << hard_clauses[i][j] << " ";
            }
            file << "0\n";
        }
        for (int i = 0; i < soft_clauses.size(); i++) {
            file << 1 << " ";
            for (int j = 0; j < soft_clauses[i].size(); j++) {
                file << soft_clauses[i][j] << " ";
            }
            file << "0\n";
        }
        return true;
    }
    return false;
}

/*!
 * Lp file output via standard translation of MaxSAT to ILP.
 *
 * nbvar - number of variables
 * hard_clauses - hard clauses in MaxSAT instance
 * soft_clauses - soft clauses in MaxSAT instance (unit weights assumed)
 * filename - name of output file
 */
bool write_lp_to_file(int nbvar, 
                      vector<vector<int>> & hard_clauses, 
                      vector<vector<int>> & soft_clauses, 
                      string filename)
{
    ofstream file(filename);
    if (file.is_open()) {
        file << "Minimize\n obj: ";
        for (int i = 1; i <= soft_clauses.size(); i++) {
            file << "b" << i;
            if (i != soft_clauses.size()) file << " + ";
        }
        file << "\nSubject To\n";
        for (int i = 0; i < soft_clauses.size(); i++) {
            int count_neg = 0;
            for (int j = 0; j < soft_clauses[i].size(); j++) {
                file << " ";
                if (soft_clauses[i][j] > 0) {
                    if (j != 0) file << " + ";
                    file << "x" << abs(soft_clauses[i][j]);
                } else {
                    if (j != 0) file << " - ";
                    else file << "-";
                    file << "x" << abs(soft_clauses[i][j]);
                    count_neg--;
                }
            }
            file << " + b" << i+1 << " >= " << ++count_neg << "\n";
        }
        for (int i = 0; i < hard_clauses.size(); i++) {
            int count_neg = 0;
            for (int j = 0; j < hard_clauses[i].size(); j++) {
                file << " ";
                if (hard_clauses[i][j] > 0) {
                    if (j != 0) file << "+ ";
                    file << "x" << abs(hard_clauses[i][j]);
                } else {
                    file << "- x" << abs(hard_clauses[i][j]);
                    count_neg--;
                }
            }
            file << " >= " << ++count_neg << "\n";
        }
        file << "Bounds\n";
        for (int i = 1; i <= soft_clauses.size(); i++) {
            file << " 0 <= b" << i << " <= 1\n";
        }
        for (int i = 1; i <= nbvar; i++) {
            file << " 0 <= x" << i << " <= 1\n";
        }
        file << "End\n";
        return true;
    }
    return false;
}

/*!
 * Main function for extension enforcement.
 *
 * af - argumentation framework with enforcement request
 * sem - semantics (adm, com, stb, prf, sem or stg)
 * strict - choice of strict (true) or non-strict (false) enforcement
 * outfile - if specified, output the clauses to outfile and exit
 * type - type of outfile (wncf or lp)
 */
AF enforce(AF& af, bool strict, bool cegar, string outfile, string type)
{
    vector<vector<int>> hard_clauses;
    vector<vector<int>> soft_clauses;
    int top = af.n_args*af.n_args-af.enfs.size()*af.enfs.size()+1;

    AF newAF;
    for (int i = 0; i < af.args.size(); i++) {
        newAF.addArgument(af.intToArg[af.args[i]]);
    }

    // generate hard clauses for direct MaxSAT
    if (!cegar) {
        if (strict) {
            hard_clauses = grounded_strict_clauses(af);
            for (int i = 0; i < af.args.size(); i++) {
                if (af.enforce[af.args[i]]) {
                    vector<int> clause;
                    clause.push_back(af.level_var[make_pair(af.enfs.size(), af.args[i])]);
                    hard_clauses.push_back(clause);
                }
            }
        } else {
            hard_clauses = grounded_non_strict_clauses(af);
            for (int i = 0; i < af.args.size(); i++) {
                if (af.enforce[af.args[i]]) {
                    vector<int> clause;
                    clause.push_back(af.level_var[make_pair((af.args.size()+1)/2, af.args[i])]);
                    hard_clauses.push_back(clause);
                }
            }
        }
    // generate hard clauses for CEGAR
    } else {
        if (strict) {
            hard_clauses = complete_strict_clauses(af);
            vector<vector<int>> level_one_clauses = level_one_clauses_strict(af);
            hard_clauses.insert(hard_clauses.end(), level_one_clauses.begin(), level_one_clauses.end());
        } else {
            hard_clauses = admissible_non_strict_clauses(af);
            vector<vector<int>> level_one_clauses = level_one_clauses_nonstrict(af);
            hard_clauses.insert(hard_clauses.end(), level_one_clauses.begin(), level_one_clauses.end());
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

    // if file specified output wcnf/lp and exit
    if (outfile != "") {
        int nbvar = 0;
        for (int i = 0; i < hard_clauses.size(); i++) {
            for (int j = 0; j < hard_clauses[i].size(); j++) {
                nbvar = max(nbvar, abs(hard_clauses[i][j]));
            }
        }
        for (int i = 0; i < soft_clauses.size(); i++) {
            for (int j = 0; j < soft_clauses[i].size(); j++) {
                nbvar = max(nbvar, abs(soft_clauses[i][j]));
            }
        }
        if (type == "wcnf") {
            if (outfile != "stdout")
                write_wcnf_to_file(nbvar, hard_clauses.size() + soft_clauses.size(), top, hard_clauses, soft_clauses, outfile);
            else
                write_wcnf_to_stdout(nbvar, hard_clauses.size() + soft_clauses.size(), top, hard_clauses, soft_clauses);
        } else if (type == "lp") {
            write_lp_to_file(nbvar, hard_clauses, soft_clauses, outfile);
        }
        return newAF;
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

    // direct MaxSAT call
    if (!cegar) {
        maxsat_solver.build_solver(top);
        maxsat_solver.solve();
        for (int i = 0; i <= maxsat_solver.assignment.rbegin()->first; i++) {
            if (af.varToAtt.find(i+1) != af.varToAtt.end() && maxsat_solver.assignment[i]) {
                newAF.addAttack(make_pair(af.intToArg[af.varToAtt[i+1].first], af.intToArg[af.varToAtt[i+1].second]));
            }
        }
    // enter CEGAR
    } else {
        int iters=0;
        while (true) {
            iters++;
            // solve abstraction using MaxSAT
            maxsat_solver.build_solver(top);
            maxsat_solver.solve();
            // construct the AF from the truth assignment
            AF temp;
            for (int i = 0; i < af.args.size(); i++) {
                temp.addArgument(af.intToArg[af.args[i]]);
            }
            for (int i = 0; i <= maxsat_solver.assignment.rbegin()->first; i++) {
                if (af.varToAtt.find(i+1) != af.varToAtt.end() && maxsat_solver.assignment[i]) {
                    temp.addAttack(make_pair(af.intToArg[af.varToAtt[i+1].first], af.intToArg[af.varToAtt[i+1].second]));
                }
            }
            // compute the grounded extension of the AF
            map<int,int> grd = grounded_labeling(temp);
            bool refine = false;
            if (strict && !is_grounded(temp, af.enfs)) {
                refine = true;
            } else if (!strict && !is_subset_of_grounded(temp, af.enfs)) {
                refine = true;
            }
            // abstraction is okay - return current AF
            if (!refine) {
                for (int i = 0; i <= maxsat_solver.assignment.rbegin()->first; i++) {
                    if (af.varToAtt.find(i+1) != af.varToAtt.end() && maxsat_solver.assignment[i]) {
                        newAF.addAttack(make_pair(af.intToArg[af.varToAtt[i+1].first], af.intToArg[af.varToAtt[i+1].second]));
                    }
                }
                break;
            // not okay - add refinement clause and continue
            } else {
                vector<int> clause;
                for (int i = 0; i < temp.args.size(); i++) {
                    for (int j = 0; j < temp.args.size(); j++) {
                        if (!af.enforce[temp.args[i]] || !af.enforce[temp.args[j]]) {
                            if (temp.att_exists[make_pair(temp.args[i], temp.args[j])]) {
                                if (grd[temp.args[i]] == ACCEPTED && grd[temp.args[j]] == REJECTED) {
                                    clause.push_back(-af.attToVar[make_pair(temp.args[i], temp.args[j])]);
                                } else if (grd[temp.args[i]] == UNDECIDED && grd[temp.args[j]] == UNDECIDED) {
                                    clause.push_back(-af.attToVar[make_pair(temp.args[i], temp.args[j])]);
                                }
                            } else {
                                if (grd[temp.args[i]] == ACCEPTED && grd[temp.args[j]] == ACCEPTED) {
                                    clause.push_back(af.attToVar[make_pair(temp.args[i], temp.args[j])]);
                                } else if (grd[temp.args[i]] == ACCEPTED && grd[temp.args[j]] == UNDECIDED) {
                                    clause.push_back(af.attToVar[make_pair(temp.args[i], temp.args[j])]);
                                } else if (grd[temp.args[i]] == REJECTED && grd[temp.args[j]] == ACCEPTED) {
                                    clause.push_back(af.attToVar[make_pair(temp.args[i], temp.args[j])]);
                                } else if (grd[temp.args[i]] == UNDECIDED && grd[temp.args[j]] == ACCEPTED) {
                                    clause.push_back(af.attToVar[make_pair(temp.args[i], temp.args[j])]);
                                }
                            }
                        }
                    }
                }
                /*for (int i = 0; i <= maxsat_solver.assignment.rbegin()->first; i++) {
                    if (af.varToAtt.find(i+1) != af.varToAtt.end()) {
                        if (maxsat_solver.assignment[i]) {
                            clause.push_back(-(i+1));
                        } else {
                            clause.push_back(i+1);
                        }
                    }
                }*/
                maxsat_solver.add_hard_clause(clause);
            }
        }
        cout << "Number of iterations:\t" << iters << "\n";
    }

    return newAF;
}

}

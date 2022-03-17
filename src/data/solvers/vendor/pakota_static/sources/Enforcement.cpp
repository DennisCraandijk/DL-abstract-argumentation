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

#include "Enforcement.h"
#include "Enumeration.h"

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
 * MaxSAT clauses for strict enforcement under stable semantics.
 */
vector<vector<int>> stable_strict_clauses(AF& af)
{
    vector<vector<int>> clauses;
    for (int i = 0; i < af.args.size(); i++) {
        if (!af.enforce[af.args[i]]) {
            vector<int> clause;
            for (int j = 0; j < af.args.size(); j++) {
                if (af.enforce[af.args[j]]) {
                    clause.push_back(af.attToVar[make_pair(af.args[j], af.args[i])]);
                }
            }
            clauses.push_back(clause);
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
 * MaxSAT clauses for non-strict enforcement under stable semantics.
 */
vector<vector<int>> stable_non_strict_clauses(AF& af)
{
    vector<vector<int>> clauses = cf_non_strict_clauses(af);
    for (int i = 0; i < af.args.size(); i++) {
        if (!af.enforce[af.args[i]]) {
            vector<int> clause;
            clause.push_back(af.argToVar[af.args[i]]);
            for (int j = 0; j < af.args.size(); j++) {
                if (af.enforce[af.args[j]]) {
                    clause.push_back(af.attToVar[make_pair(af.args[j], af.args[i])]);
                } else {
                    clause.push_back(af.attackVar[make_pair(af.args[j], af.args[i])]);
                }
            }
            clauses.push_back(clause);
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
AF enforce(AF& af, string sem, bool strict, string outfile, string type)
{
    vector<vector<int>> hard_clauses;
    vector<vector<int>> soft_clauses;
    int top = af.n_args*af.n_args-af.enfs.size()*af.enfs.size()+1;

    AF newAF;
    for (int i = 0; i < af.args.size(); i++) {
        newAF.addArgument(af.intToArg[af.args[i]]);
    }

    // generate hard clauses
    if (strict) {
        if (sem == "adm") {
            hard_clauses = admissible_strict_clauses(af);
        } else if (sem == "com" || sem == "prf" || sem == "sem") {
            hard_clauses = complete_strict_clauses(af);
        } else if (sem == "stb") {
            hard_clauses = stable_strict_clauses(af);
        }
    } else {
        if (sem == "stg") {
            hard_clauses = cf_non_strict_clauses(af);
        } else if (sem == "adm" || sem == "com" || sem == "prf" || sem == "sem") {
            hard_clauses = admissible_non_strict_clauses(af);
        } else if (sem == "stb") {
            hard_clauses = stable_non_strict_clauses(af);
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
        if (sem != "prf" && sem != "sem" && sem != "stg") {
            if (type == "wcnf")
                write_wcnf_to_file(nbvar, hard_clauses.size() + soft_clauses.size(), top, hard_clauses, soft_clauses, outfile);
            else if (type == "lp")
                write_lp_to_file(nbvar, hard_clauses, soft_clauses, outfile);
            return newAF;
        } else {
            cerr << "Error! Clause output is not supported for preferred, semi-stable and stage semantics.\n";
            return newAF;
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

    // use MaxSAT for problems on the first level
    if (!(strict && sem == "prf") && sem != "sem" && sem != "stg") {
        maxsat_solver.solve();
        for (int i = 0; i <= maxsat_solver.assignment.rbegin()->first; i++) {
            if (af.varToAtt.find(i+1) != af.varToAtt.end() && maxsat_solver.assignment[i]) {
                newAF.addAttack(make_pair(af.intToArg[af.varToAtt[i+1].first], af.intToArg[af.varToAtt[i+1].second]));
            }
        }
        return newAF;
    // use CEGAR for problems on the second level
    } else {
        if (!strict) {
            for (int i = 0; i < af.args.size(); i++) {
                if (!af.enforce[af.args[i]]) {
                    vector<int> clause;
                    clause.push_back(-af.rangeVar[af.args[i]]);
                    clause.push_back(af.argToVar[af.args[i]]);
                    for (int j = 0; j < af.args.size(); j++) {
                        if (af.enforce[af.args[j]]) {
                            clause.push_back(af.attToVar[make_pair(af.args[j], af.args[i])]);
                        } else {
                            clause.push_back(af.attackVar[make_pair(af.args[j], af.args[i])]);
                        }
                    }
                    maxsat_solver.add_hard_clause(clause);
                }
            }
            for (int i = 0; i < af.args.size(); i++) {
                if (!af.enforce[af.args[i]]) {
                    vector<int> clause;
                    clause.push_back(-af.argToVar[af.args[i]]);
                    clause.push_back(af.rangeVar[af.args[i]]);
                    maxsat_solver.add_hard_clause(clause);
                }
            }
            for (int i = 0; i < af.args.size(); i++) {
                if (!af.enforce[af.args[i]]) {
                    for (int j = 0; j < af.args.size(); j++) {
                        vector<int> clause;
                        if (af.enforce[af.args[j]]) {
                            clause.push_back(-af.attToVar[make_pair(af.args[j], af.args[i])]);
                        } else {
                            clause.push_back(-af.attackVar[make_pair(af.args[j], af.args[i])]);
                        }
                        clause.push_back(af.rangeVar[af.args[i]]);
                        maxsat_solver.add_hard_clause(clause);
                    }
                }
            }
            if (sem == "stg") {
                for (int i = 0; i < af.args.size(); i++) {
                    for (int j = 0; j < af.args.size(); j++) {
                        if (!af.enforce[af.args[i]] && !af.enforce[af.args[j]]) {
                            vector<int> clause;
                            clause.push_back(af.argToVar[af.args[i]]);
                            clause.push_back(-af.attackVar[make_pair(af.args[i], af.args[j])]);
                            maxsat_solver.add_hard_clause(clause);
                        }
                    }
                }
                for (int i = 0; i < af.args.size(); i++) {
                    for (int j = 0; j < af.args.size(); j++) {
                        if (!af.enforce[af.args[i]] && !af.enforce[af.args[j]]) {
                            vector<int> clause;
                            clause.push_back(af.attToVar[make_pair(af.args[i], af.args[j])]);
                            clause.push_back(-af.attackVar[make_pair(af.args[i], af.args[j])]);
                            maxsat_solver.add_hard_clause(clause);
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
                            maxsat_solver.add_hard_clause(clause);
                        }
                    }
                }
            }
        }
        while (true) {
            // compute optimal solution via MaxSAT
            maxsat_solver.solve();
            // initialize SAT solver
            SAT_Solver sat_solver = SAT_Solver();
            // construct the AF proposed by the solution
            AF temp;
            for (int i = 0; i < af.args.size(); i++) {
                temp.addArgument(af.intToArg[af.args[i]]);
            }
            for (int i = 0; i <= maxsat_solver.assignment.rbegin()->first; i++) {
                if (af.varToAtt.find(i+1) != af.varToAtt.end() && maxsat_solver.assignment[i]) {
                    temp.addAttack(make_pair(af.intToArg[af.varToAtt[i+1].first], af.intToArg[af.varToAtt[i+1].second]));
                }
            }
            for (int i = 0; i < af.args.size(); i++) {
                if (af.enforce[af.args[i]]) {
                    temp.addEnforcement(af.intToArg[af.args[i]]);
                }
            }
            if (!strict) {
                for (int i = 0; i <= maxsat_solver.assignment.rbegin()->first; i++) {
                    if (af.varToArg.find(i+1) != af.varToArg.end() && maxsat_solver.assignment[i]) {
                        temp.addEnforcement(af.intToArg[af.varToArg[i+1]]);
                    }
                }
            }
            temp.initialize_enum(sem);
            // generate clauses for SAT check
            vector<vector<int>> temp_clauses;
            if (sem != "stg") {
                temp_clauses = Enumeration::complete_clauses(temp);
            } else {
                temp_clauses = Enumeration::conflictFree_clauses(temp);
            }
            if (sem == "prf") {
                for (int i = 0; i < af.args.size(); i++) {
                    if (af.enforce[af.args[i]]) {
                        vector<int> clause;
                        clause.push_back(af.args[i]);
                        temp_clauses.push_back(clause);
                    }
                }
                vector<int> clause;
                for (int i = 0; i < af.args.size(); i++) {
                    if (!af.enforce[af.args[i]]) {
                        clause.push_back(af.args[i]);
                    }
                }
                temp_clauses.push_back(clause);
            } else {
                for (int i = 0; i < temp.args.size(); i++) {
                    if (temp.in_range[temp.args[i]]) {
                        vector<int> clause;
                        clause.push_back(temp.rangeVar[temp.args[i]]);
                        temp_clauses.push_back(clause);
                    }
                }
                vector<int> clause;
                for (int i = 0; i < temp.args.size(); i++) {
                    if (!temp.in_range[temp.args[i]]) {
                        clause.push_back(temp.rangeVar[temp.args[i]]);
                    }
                }
                temp_clauses.push_back(clause);
                for (int i = 0; i < temp.args.size(); i++) {
                    vector<int> clause;
                    clause.push_back(-temp.rangeVar[temp.args[i]]);
                    clause.push_back(temp.argToVar[temp.args[i]]);
                    for (int j = 0; j < temp.attackers[temp.args[i]].size(); j++) {
                        clause.push_back(temp.argToVar[temp.attackers[temp.args[i]][j]]);
                    }
                    temp_clauses.push_back(clause);           
                }
                for (int i = 0; i < temp.args.size(); i++) {
                    vector<int> clause;
                    clause.push_back(-temp.argToVar[temp.args[i]]);
                    clause.push_back(temp.rangeVar[temp.args[i]]);
                    temp_clauses.push_back(clause);           
                }
                for (int i = 0; i < temp.args.size(); i++) {
                    for (int j = 0; j < temp.attackers[temp.args[i]].size(); j++) {
                        vector<int> clause;
                        clause.push_back(-temp.argToVar[temp.attackers[temp.args[i]][j]]);
                        clause.push_back(temp.rangeVar[temp.args[i]]);
                        temp_clauses.push_back(clause);
                    }
                }
            }
            // add generated clauses to SAT solver
            for (int i = 0; i < temp_clauses.size(); i++) {
                sat_solver.add_clause(temp_clauses[i]);
            }
            // if satisfiable
            if (sat_solver.solve()) {
                // add refinement clause
                vector<int> clause;
                for (int i = 0; i <= maxsat_solver.assignment.rbegin()->first; i++) {
                    if (af.varToAtt.find(i+1) != af.varToAtt.end()) {
                        if (maxsat_solver.assignment[i]) {
                            clause.push_back(-(i+1));
                        } else {
                            clause.push_back(i+1);
                        }
                    }
                }
                if (!strict) {
                    for (int i = 0; i < af.args.size(); i++) {
                        if (!temp.in_range[af.args[i]]) {
                            clause.push_back(af.rangeVar[af.args[i]]);
                        }
                    }
                }
                maxsat_solver.add_hard_clause(clause);
            // unsatisfiable - output the new AF
            } else {
                for (int i = 0; i <= maxsat_solver.assignment.rbegin()->first; i++) {
                    if (af.varToAtt.find(i+1) != af.varToAtt.end() && maxsat_solver.assignment[i]) {
                        newAF.addAttack(make_pair(af.intToArg[af.varToAtt[i+1].first], af.intToArg[af.varToAtt[i+1].second]));
                    }
                }
                return newAF;
            }
        }
    }
}

}

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

#include "ArguFramework.h"

AF::AF() : n_args(0), count(0) {}

/*!
 * Adds a new argument to the AF instance.
 */
void AF::addArgument(std::string arg)
{
    n_args++;
    args.push_back(n_args);
    argToInt[arg] = n_args;
    intToArg[n_args] = arg;
}

/*!
 * Adds a new attack to the AF instance.
 */
void AF::addAttack(std::pair<std::string,std::string> att)
{
    atts.push_back(std::make_pair(argToInt[att.first], argToInt[att.second]));
    attackers[argToInt[att.second]].push_back(argToInt[att.first]);
    attacked[argToInt[att.first]].push_back(argToInt[att.second]);
    att_exists[std::make_pair(argToInt[att.first], argToInt[att.second])] = true;
}

/*!
 * Adds an enforcement to the AF instance.
 */
void AF::addEnforcement(std::string arg)
{
    enforce[argToInt[arg]] = true;
    enfs.push_back(argToInt[arg]);
    in_range[argToInt[arg]] = true;
    for (int i = 0; i < attacked[argToInt[arg]].size(); i++) {
        in_range[attacked[argToInt[arg]][i]] = true;
    }
}

void AF::addNegEnforcement(std::string arg)
{
    neg_enforce[argToInt[arg]] = true;
    neg_enfs.push_back(argToInt[arg]);
}

/*!
 * Constructs the variable mappings for MaxSAT clauses (extension enforcement).
 */
void AF::initialize(std::string sem, bool strict)
{
    if (!strict) {
        if (sem == "stb") {
            for (int i = 0; i < args.size(); i++) {
                if (!enforce[args[i]]) {
                    count++;
                    argToVar[args[i]] = count;
                    varToArg[count] = args[i];
                }
            }
            for (int i = 0; i < args.size(); i++) {
                for (int j = 0; j < args.size(); j++) {
                    if (!enforce[args[i]] || !enforce[args[j]]) {
                        count++;
                        attToVar[std::make_pair(args[i], args[j])] = count;
                        varToAtt[count] = std::make_pair(args[i], args[j]);
                    }
                }
            }
            for (int i = 0; i < args.size(); i++) {
                for (int j = 0; j < args.size(); j++) {
                    if (!enforce[args[i]] && !enforce[args[j]]) {
                        count++;
                        attackVar[std::make_pair(args[i], args[j])] = count;
                    }
                }
            }
        } else if (sem == "adm" || sem == "com" || sem == "prf") {
            for (int i = 0; i < args.size(); i++) {
                if (!enforce[args[i]]) {
                    count++;
                    argToVar[args[i]] = count;
                    varToArg[count] = args[i];
                }
            }
            for (int i = 0; i < args.size(); i++) {
                for (int j = 0; j < args.size(); j++) {
                    if (!enforce[args[i]] || !enforce[args[j]]) {
                        count++;
                        attToVar[std::make_pair(args[i], args[j])] = count;
                        varToAtt[count] = std::make_pair(args[i], args[j]);
                    }
                }
            }
            for (int i = 0; i < args.size(); i++) {
                for (int j = 0; j < args.size(); j++) {
                    if (!enforce[args[i]] && !enforce[args[j]]) {
                        count++;
                        attackedVar[std::make_pair(args[i], args[j])] = count;
                    }
                }
            }
            for (int i = 0; i < args.size(); i++) {
                for (int j = 0; j < args.size(); j++) {
                    if (!enforce[args[i]] && !enforce[args[j]]) {
                        count++;
                        attackVar[std::make_pair(args[i], args[j])] = count;
                    }
                }
            }
        } else if (sem == "sem") {
            for (int i = 0; i < args.size(); i++) {
                if (!enforce[args[i]]) {
                    count++;
                    argToVar[args[i]] = count;
                    varToArg[count] = args[i];
                }
            }
            for (int i = 0; i < args.size(); i++) {
                for (int j = 0; j < args.size(); j++) {
                    if (!enforce[args[i]] || !enforce[args[j]]) {
                        count++;
                        attToVar[std::make_pair(args[i], args[j])] = count;
                        varToAtt[count] = std::make_pair(args[i], args[j]);
                    }
                }
            }
            for (int i = 0; i < args.size(); i++) {
                for (int j = 0; j < args.size(); j++) {
                    if (!enforce[args[i]] && !enforce[args[j]]) {
                        count++;
                        attackedVar[std::make_pair(args[i], args[j])] = count;
                    }
                }
            }
            for (int i = 0; i < args.size(); i++) {
                for (int j = 0; j < args.size(); j++) {
                    if (!enforce[args[i]] && !enforce[args[j]]) {
                        count++;
                        attackVar[std::make_pair(args[i], args[j])] = count;
                    }
                }
            }
            for (int i = 0; i < args.size(); i++) {
                if (!enforce[args[i]]) {
                    count++;
                    rangeVar[args[i]] = count;
                }
            }
        } else if (sem == "stg") {
            for (int i = 0; i < args.size(); i++) {
                if (!enforce[args[i]]) {
                    count++;
                    argToVar[args[i]] = count;
                    varToArg[count] = args[i];
                }
            }
            for (int i = 0; i < args.size(); i++) {
                for (int j = 0; j < args.size(); j++) {
                    if (!enforce[args[i]] || !enforce[args[j]]) {
                        count++;
                        attToVar[std::make_pair(args[i], args[j])] = count;
                        varToAtt[count] = std::make_pair(args[i], args[j]);
                    }
                }
            }
            for (int i = 0; i < args.size(); i++) {
                for (int j = 0; j < args.size(); j++) {
                    if (!enforce[args[i]] && !enforce[args[j]]) {
                        count++;
                        attackVar[std::make_pair(args[i], args[j])] = count;
                    }
                }
            }
            for (int i = 0; i < args.size(); i++) {
                if (!enforce[args[i]]) {
                    count++;
                    rangeVar[args[i]] = count;
                }
            }
        }
    } else {
        for (int i = 0; i < args.size(); i++) {
            for (int j = 0; j < args.size(); j++) {
                if (!enforce[args[i]] || !enforce[args[j]]) {
                    count++;
                    attToVar[std::make_pair(args[i], args[j])] = count;
                    varToAtt[count] = std::make_pair(args[i], args[j]);
                }
            }
        }
        for (int i = 0; i < args.size(); i++) {
            if (!enforce[args[i]]) {
                for (int j = 0; j < args.size(); j++) {
                    if (!enforce[args[j]]) {
                        count++;
                        attackVar[std::make_pair(args[i], args[j])] = count;
                    }
                }
            }
        }
    }
}

/*!
 * Constructs the variable mappings for MaxSAT clauses (credulous status enforcement).
 */
void AF::initialize_cred() {
    for (int i = 0; i < enfs.size(); i++) {
        for (int j = 0; j < args.size(); j++) {
            if ((!enforce[args[j]] || enfs[i] != args[j]) && !neg_enforce[args[j]]) {
                count++;
                arg_var[std::make_pair(enfs[i], args[j])] = count;
                var_arg[count] = std::make_pair(enfs[i], args[j]);
            }
        }
    }
    for (int i = 0; i < args.size(); i++) {
        for (int j = 0; j < args.size(); j++) {
            if (!enforce[args[i]] || !enforce[args[j]] || args[i] != args[j]) {
                count++;
                attToVar[std::make_pair(args[i], args[j])] = count;
                varToAtt[count] = std::make_pair(args[i], args[j]);
            }
        }
    }
    for (int i = 0; i < enfs.size(); i++) {
        for (int j = 0; j < args.size(); j++) {
            if (args[j] != enfs[i]) {
                for (int k = 0; k < args.size(); k++) {
                    if ((!enforce[args[j]] || args[k] != args[j]) && args[k] != enfs[i] && !neg_enforce[args[k]]) {
                        count++;
                        att_var[std::make_pair(enfs[i], std::make_pair(args[k], args[j]))] = count;
                    }
                }
            }
        }
    }
}

/*!
 * Constructs the variable mappings for MaxSAT clauses (skeptical status enforcement).
 */
void AF::initialize_skept()
{
    if (neg_enfs.size() == 0) {
        for (int i = 0; i < args.size(); i++) {
            if (!enforce[args[i]]) {
                count++;
                arg_var[std::make_pair(0, args[i])] = count;
                var_arg[count] = std::make_pair(0, args[i]);
            }
        }
        for (int i = 0; i < args.size(); i++) {
            for (int j = 0; j < args.size(); j++) {
                if (!enforce[args[i]] || !enforce[args[j]]) {
                    count++;
                    attToVar[std::make_pair(args[i], args[j])] = count;
                    varToAtt[count] = std::make_pair(args[i], args[j]);
                }
            }
        }
        for (int i = 0; i < args.size(); i++) {
            if (!enforce[args[i]]) {
                for (int j = 0; j < args.size(); j++) {
                    if (!enforce[args[j]]) {
                        count++;
                        att_var[std::make_pair(0, std::make_pair(args[j], args[i]))] = count;
                    }
                }
            }
        }
    } else {
        for (int i = 0; i < neg_enfs.size(); i++) {
            for (int j = 0; j < args.size(); j++) {
                if (!enforce[args[j]] && neg_enfs[i] != args[j]) {
                    count++;
                    arg_var[std::make_pair(neg_enfs[i], args[j])] = count;
                    var_arg[count] = std::make_pair(neg_enfs[i], args[j]);
                }
            }
        }
        for (int i = 0; i < args.size(); i++) {
            for (int j = 0; j < args.size(); j++) {
                if (!enforce[args[i]] || !enforce[args[j]]) {
                    count++;
                    attToVar[std::make_pair(args[i], args[j])] = count;
                    varToAtt[count] = std::make_pair(args[i], args[j]);
                }
            }
        }
        for (int i = 0; i < neg_enfs.size(); i++) {
            for (int j = 0; j < args.size(); j++) {
                if (!enforce[args[j]]) {
                    for (int k = 0; k < args.size(); k++) {
                        if (args[k] != neg_enfs[i] && !enforce[args[k]]) {
                            count++;
                            att_var[std::make_pair(neg_enfs[i], std::make_pair(args[k], args[j]))] = count;
                        }
                    }
                }
            }
        }   
    } 
}

void AF::initialize_enum(std::string sem)
{
    for (int i = 0; i < args.size(); i++) {
        count++;
        argToVar[args[i]] = count;
        varToArg[count] = args[i];
    }
    if (sem != "stg") {
        for (int i = 0; i < args.size(); i++) {
            count++;
            defendVar[args[i]] = count;
        }
    }
    if (sem != "prf") {
        for (int i = 0; i < args.size(); i++) {
            count++;
            rangeVar[args[i]] = count;
        }
    }
}

/*!
 * Outputs the number of inner conflicts in the AF, i.e. attacks inside the enforced subset of arguments.
 * These attacks are automatically removed and do not contribute to the objective function value.
 */
int AF::number_of_conflicts()
{
    int conflicts = 0;
    for (int i = 0; i < atts.size(); i++) {
        if (enforce[atts[i].first] && enforce[atts[i].second])
            conflicts++;
    }
    return conflicts;
}
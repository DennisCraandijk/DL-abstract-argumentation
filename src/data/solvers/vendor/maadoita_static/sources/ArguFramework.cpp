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
    range[argToInt[att.first]].push_back(argToInt[att.second]);
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
    for (int i = 0; i < range[argToInt[arg]].size(); i++) {
        in_range[range[argToInt[arg]][i]] = true;
    }
}

/*!
 * Constructs the variable mappings for MaxSAT clauses.
 */
void AF::initialize(bool strict, bool cegar)
{
    if (!cegar) {
        if (strict) {
            for (int i = 0; i < args.size(); i++) {
                for (int j = 0; j < args.size(); j++) {
                    if (!enforce[args[i]] || !enforce[args[j]]) {
                        count++;
                        attToVar[std::make_pair(args[i], args[j])] = count;
                        varToAtt[count] = std::make_pair(args[i], args[j]);
                    }
                }
            }
            for (int n = 1; n <= enfs.size(); n++) {
                for (int i = 0; i < args.size(); i++) {
                    if (enforce[args[i]]) {
                        count++;
                        level_var[std::make_pair(n, args[i])] = count;
                    }
                }
            }
            for (int n = 2; n <= enfs.size(); n++) {
                for (int i = 0; i < args.size(); i++) {
                    if (enforce[args[i]]) {
                        for (int j = 0; j < args.size(); j++) {
                            if (!enforce[args[j]]) {
                                count++;
                                level_attack_var[std::make_pair(n-1, std::make_pair(args[i], args[j]))] = count;
                            }
                        }
                    }
                }
            }
            for (int n = 2; n <= enfs.size(); n++) {
                for (int i = 0; i < args.size(); i++) {
                    if (!enforce[args[i]]) {
                        for (int j = 0; j < args.size(); j++) {
                            if (enforce[args[j]]) {
                                count++;
                                level_not_defended_var[std::make_pair(n-1, std::make_pair(args[i], args[j]))] = count;
                            }
                        }
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
            for (int n = 1; n <= (args.size()+1)/2; n++) {
                for (int i = 0; i < args.size(); i++) {
                    count++;
                    level_var[std::make_pair(n, args[i])] = count;
                }
            }
            for (int n = 2; n <= (args.size()+1)/2; n++) {
                for (int i = 0; i < args.size(); i++) {
                    for (int j = 0; j < args.size(); j++) {
                        count++;
                        level_attack_var[std::make_pair(n-1, std::make_pair(args[i], args[j]))] = count;
                    }
                }
            }
            for (int n = 2; n <= (args.size()+1)/2; n++) {
                for (int i = 0; i < args.size(); i++) {
                    for (int j = 0; j < args.size(); j++) {
                        count++;
                        level_not_defended_var[std::make_pair(n-1, std::make_pair(args[i], args[j]))] = count;
                    }
                }
            }
        }
    } else {
        if (!strict) {
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
                count++;
                level_var[std::make_pair(1, args[i])] = count;
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
            for (int i = 0; i < args.size(); i++) {
                if (enforce[args[i]]) {
                    count++;
                    level_var[std::make_pair(1, args[i])] = count;
                }
            }
        }
    }
}

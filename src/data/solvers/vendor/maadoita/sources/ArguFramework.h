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

#ifndef ARGU_FRAMEWORK_H
#define ARGU_FRAMEWORK_H

#include <vector>
#include <map>

class AF {
public:
    
AF();

/*!
 * Number of arguments and count for MaxSAT variables.
 */    
int n_args;
int count;

/*!
 * Arguments, attacks and enforcements of the AF instance.
 */
std::vector<int> args;
std::vector<std::pair<int,int>> atts;
std::vector<int> enfs;
std::vector<int> neg_enfs;

/*!
 * Maps an argument to an integer and the other way around.
 */
std::map<std::string,int> argToInt;
std::map<int,std::string> intToArg;

/*!
 * Maps an argument to true if it is enforced.
 */
std::map<int,bool> enforce;
std::map<int,bool> neg_enforce;

/*!
 * Maps an attack to true if the attack exists.
 */
std::map<std::pair<int,int>,bool> att_exists;

/*!
 * Maps an argument to its attackers.
 */
std::map<int,std::vector<int>> attackers;
std::map<int,std::vector<int>> range;

/*!
 * Maps an argument to its MaxSAT variable and the other way around.
 */
std::map<int,int> argToVar;
std::map<int,int> varToArg;

/*!
 * Maps an attack to its MaxSAT variable and the other way around.
 */
std::map<std::pair<int,int>,int> attToVar;
std::map<int,std::pair<int,int>> varToAtt;

/*!
 * Other MaxSAT variables.
 */
std::map<int,int> defendVar;
std::map<std::pair<int,int>,int> attackVar;
std::map<std::pair<int,int>,int> attackedVar;

std::map<std::pair<int,int>,int> level_var;
std::map<std::pair<int,std::pair<int,int>>,int> level_attack_var;
std::map<std::pair<int,std::pair<int,int>>,int> level_not_defended_var; 

std::map<int,int> rangeVar;
std::map<int,bool> in_range;

/*!
 * Adds a new argument to the AF instance.
 */
void addArgument(std::string arg);

/*!
 * Adds a new attack to the AF instance.
 */
void addAttack(std::pair<std::string,std::string> att);

/*!
 * Adds an enforcement to the AF instance.
 */
void addEnforcement(std::string arg);

/*!
 * Constructs the variable mappings needed for generation of MaxSAT clauses.
 */
//void initialize(std::string sem, bool strict);
void initialize(bool strict, bool cegar);

/*!
 * Outputs the number of inner conflicts in the AF, i.e. attacks inside the enforced subset of arguments.
 * These attacks are automatically removed and do not contribute to the objective function value.
 */
int number_of_conflicts();

};

#endif

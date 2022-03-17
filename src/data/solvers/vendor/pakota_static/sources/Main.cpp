/*!
 * Copyright (c) <2016> <Andreas Niskanen, University of Helsinki>
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
#include "Enforcement.h"
#include "CredEnforcement.h"
#include "SkeptEnforcement.h"

#include <iostream>
#include <fstream>
#include <set>
#include <algorithm>
#include <unistd.h>

using namespace std;

static void show_usage() {
    cout << "USAGE: ./pakota [file] [mode] [sem] [options]\n\n"
         << "COMMAND LINE ARGUMENTS:\n\n"
         << "file : Input filename for enforcement instance in apx format.\n"
         << "mode : Enforcement variant. mode={strict|non-strict|cred|skept}\n"
         << "    strict     : strict extension enforcement\n"
         << "    non-strict : non-strict extension enforcement\n"
         << "    cred       : credulous status enforcement\n"
         << "    skept      : skeptical status enforcement\n"
         << "sem  : Argumentation semantics. sem={adm|com|stb|prf|sem|stg}\n"
         << "    adm        : admissible\n"
         << "    com        : complete\n"
         << "    stb        : stable\n"
         << "    prf        : preferred\n"
         << "    sem        : semi-stable\n"
         << "    stg        : stage\n\n"
         << "COMMAND LINE OPTIONS:\n\n"
         << "-h      : Display this help message.\n"
         << "-v      : Display the version of the program.\n"
         << "-o out  : Output clauses to file out and exit.\n"
         << "-t type : Output clauses in format type={wcnf|lp} (default: wcnf).\n";
}

static void show_version() {
    cout << "Pakota (version 2017-05-31)\n"
         << "Solver for extension and status enforcement in abstract argumentation\n";
}

int main(int argc, char **argv)
{
    if (argc == 1) {
        show_usage();
        return 1;
    }

    string filename, mode, sem;

    if (argc > 3) {
        filename = argv[1];
        mode = argv[2];
        sem = argv[3];
    }

    string outfile = "";
    string type = "";

    char tmp;
    while ((tmp = getopt(argc, argv, "ho:t:v")) != -1) {
        switch (tmp) {
            case 'h':
                show_usage();
                return 0;
            case 'v':
                show_version();
                return 0;
            case 'o':
                outfile = optarg;
                break;
            case 't':
                type = optarg;
                break;
        }
    }

    if (argc < 4) {
        show_usage();
        return 1;
    }

    if ((sem != "adm" && sem != "com" && sem != "stb" && sem != "prf" && sem != "sem" && sem != "stg")
        || (mode != "strict" && mode != "non-strict" && mode != "cred" && mode != "skept")) {
        cout << "Error: Semantics {adm|com|stb|prf|sem|stg} and mode {strict|non-strict|cred|skept} must be specified.\n";
        //show_usage();
        return 1;
    }

    if (sem == "adm" && mode == "skept") {
        cout << "Error: No solution for skeptical status enforcement under admissible semantics.\n";
        return 1;
    }

    if ((sem == "sem" || sem == "stg") && mode == "cred") {
        cout << "Error: Only admissible and stable semantics supported for credulous status enforcement.\n";
        return 1;
    }

    if ((sem == "com" || sem == "prf" || sem == "sem" || sem == "stg") && mode == "skept") {
        cout << "Error: Only stable semantics supported for skeptical status enforcement.\n";
        return 1;
    }

    if ((sem == "com" || sem == "prf") && (mode == "non-strict" || mode == "cred")) {
        sem = "adm";
    }

    if (outfile != "" && (type != "wcnf" && type != "lp")) {
        cout << "Warning: Using default option of outputting clauses in wcnf format.\n";
        type = "wcnf";
    }

    bool strict = false;
    if (mode == "strict") strict = true;
    
    ifstream input;
    input.open(filename);

    if (!input.good()) {
        cout << "Error: Cannot open input file.\n";
        return 1;
    }

    AF af;
    string line, arg, attacker, attacked;

    while (!input.eof()) {
        getline(input, line);
        line.erase(remove_if(line.begin(), line.end(), ::isspace), line.end());
        if (line.length() == 0 || line[0] == '/' || line[0] == '%') continue;
        if (line.length() < 6) cout << "Warning! Cannot parse line: " << line << "\n";
        string op = line.substr(0,3);
        if (op == "arg") {
            if (line[3] == '(' && line.find(')') != string::npos) {
                arg = line.substr(4,line.find(')')-4);
                af.addArgument(arg);
            } else {
                cout << "Warning: Cannot parse line: " << line << "\n";
            }
        } else if (op == "att") {
            if (line[3] == '(' && line.find(',') != string::npos && line.find(')') != string::npos) {
                attacker = line.substr(4,line.find(',')-4);
                attacked = line.substr(line.find(',')+1,line.find(')')-line.find(',')-1);
                af.addAttack(make_pair(attacker, attacked));
            } else {
                cout << "Warning: Cannot parse line: " << line << "\n";
            }
        } else if (op == "enf" && line.find(')') != string::npos) {
            if (mode != "strict" && mode != "non-strict") {
                cout << "Error: enf predicate not used for status enforcement.\n";
                return 1;
            }
            if (line[3] == '(') {
                arg = line.substr(4,line.find(')')-4);
                af.addEnforcement(arg);
            } else {
                cout << "Warning: Cannot parse line: " << line << "\n";
            }
        } else if (op == "pos" && line.find(')') != string::npos) {
            if (mode != "cred" && mode != "skept") {
                cout << "Error: pos predicate not used for extension enforcement.\n";
                return 1;
            }
            if (line[3] == '(') {
                arg = line.substr(4,line.find(')')-4);
                af.addEnforcement(arg);
            } else {
                cout << "Warning: Cannot parse line: " << line << "\n";
            }
        } else if (op == "neg" && line.find(')') != string::npos) {
            if (mode != "cred" && mode != "skept") {
                cout << "Error: neg predicate not used for extension enforcement.\n";
                return 1;
            }
            if (line[3] == '(') {
                arg = line.substr(4,line.find(')')-4);
                af.addNegEnforcement(arg);
            } else {
                cout << "Warning: Cannot parse line: " << line << "\n";
            }    
        } else {
            cout << "Warning: Cannot parse line: " << line << "\n";
        }
    }

    cout << "Number of arguments:          " << af.args.size() << "\n";
    cout << "Number of attacks:            " << af.atts.size() << "\n";
    if (mode == "cred" || mode == "skept") {
        cout << "Number of pos.enf. arguments: " << af.enfs.size() << "\n";    
        cout << "Number of neg.enf. arguments: " << af.neg_enfs.size() << "\n";
    } else {
        cout << "Number of enforced arguments: " << af.enfs.size() << "\n";
    }

    AF newAF;
    if (mode == "cred") {
        af.initialize_cred();
        newAF = CredEnforcement::enforce(af, sem, outfile, type);
    } else if (mode == "skept") {
        af.initialize_skept();
        newAF = SkeptEnforcement::enforce(af);
    } else {
        af.initialize(sem, strict);
        newAF = Enforcement::enforce(af, sem, strict, outfile, type);
    }

    if (outfile == "") {
        for (int i = 0; i < newAF.args.size(); i++)
            cout << "arg(" << newAF.intToArg[newAF.args[i]] << ").\n";
        for (int i = 0; i < newAF.atts.size(); i++)
            cout << "att(" << newAF.intToArg[newAF.atts[i].first] << "," << newAF.intToArg[newAF.atts[i].second] << ").\n";
        set<pair<int,int>> first(af.atts.begin(),af.atts.end());
        set<pair<int,int>> second(newAF.atts.begin(),newAF.atts.end());
        set<pair<int,int>> result;
        for (auto it = first.begin(); it != first.end(); ++it)
            if (second.find(*it) == second.end() && result.find(*it) == result.end())
                result.insert(*it);
        for (auto it = second.begin(); it != second.end(); ++it)
            if (first.find(*it) == first.end() && result.find(*it) == result.end())
                result.insert(*it);
        cout << "Number of changes: " << result.size() << "\n";
    }

    return 0;
}

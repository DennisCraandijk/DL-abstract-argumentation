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
#include "Enforcement.h"
#include "Grounded.h"

#include <iostream>
#include <fstream>
#include <set>
#include <algorithm>
#include <unistd.h>

using namespace std;

static void show_usage() {
    cout << "USAGE: ./maadoita [file] [mode] [options]\n\n"
         << "COMMAND LINE ARGUMENTS:\n\n"
         << "file : Input filename for enforcement instance in apx format.\n"
         << "mode : Enforcement variant. mode={strict|non-strict}\n"
         << "    strict     : strict extension enforcement\n"
         << "    non-strict : non-strict extension enforcement\n"
         << "COMMAND LINE OPTIONS:\n\n"
         << "-h      : Display this help message.\n"
         << "-v      : Display the version of the program.\n"
         << "-c      : Use CEGAR instead of direct MaxSAT encoding.\n"
         << "-s      : Output clauses to stdout and exit.\n"
         << "-o out  : Output clauses to file out and exit.\n"
         << "-t type : Output clauses in format type={wcnf|lp} (default: wcnf).\n";
}

static void show_version() {
    cout << "Maadoita (version 2018-11-01)\n"
         << "Solver for grounded enforcement in abstract argumentation\n";
}

int main(int argc, char **argv)
{
    if (argc == 1) {
        show_usage();
        return 1;
    }

    string filename, mode;

    if (argc > 2) {
        filename = argv[1];
        mode = argv[2];
    }

    string outfile = "";
    string type = "";
    bool cegar = false;
    bool grounded = false;

    char tmp;
    while ((tmp = getopt(argc, argv, "cgho:st:v")) != -1) {
        switch (tmp) {
            case 'c':
                cegar = true;
                break;
            case 'g':
                grounded = true;
                break;
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
            case 's':
                outfile = "stdout";
                break;
        }
    }

    if (mode != "strict" && mode != "non-strict") {
        cout << "Error: Mode {strict|non-strict} must be specified.\n";
        return 1;
    }

    if (outfile != "" && (type != "wcnf" && type != "lp")) {
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
            if (line[3] == '(') {
                arg = line.substr(4,line.find(')')-4);
                af.addEnforcement(arg);
            } else {
                cout << "Warning: Cannot parse line: " << line << "\n";
            }
        } else {
            cout << "Warning: Cannot parse line: " << line << "\n";
        }
    }

    if (outfile != "stdout") {
        cout << "Number of arguments:\t" << af.args.size() << "\n";
        cout << "Number of attacks:\t" << af.atts.size() << "\n";
        cout << "Number of enforced:\t" << af.enfs.size() << "\n";
    }

    if (grounded) {
        vector<int> grd = grounded_extension(af);
        for (auto arg : grd) {
            cout << "arg(" << af.intToArg[arg] << ").\n";
        }
        return 0;
    }


    AF newAF;
    af.initialize(strict, cegar);
    newAF = Enforcement::enforce(af, strict, cegar, outfile, type);

    if (outfile == "") {
        set<pair<int,int>> first(af.atts.begin(),af.atts.end());
        set<pair<int,int>> second(newAF.atts.begin(),newAF.atts.end());
        set<pair<int,int>> result;
        for (auto it = first.begin(); it != first.end(); ++it)
            if (second.find(*it) == second.end() && result.find(*it) == result.end())
                result.insert(*it);
        for (auto it = second.begin(); it != second.end(); ++it)
            if (first.find(*it) == first.end() && result.find(*it) == result.end())
                result.insert(*it);
        cout << "Number of changes:\t" << result.size() << "\n";
        for (int i = 0; i < newAF.args.size(); i++)
            cout << "arg(" << newAF.intToArg[newAF.args[i]] << ").\n";
        for (int i = 0; i < newAF.atts.size(); i++)
            cout << "att(" << newAF.intToArg[newAF.atts[i].first] << "," << newAF.intToArg[newAF.atts[i].second] << ").\n";
    }

    return 0;
}

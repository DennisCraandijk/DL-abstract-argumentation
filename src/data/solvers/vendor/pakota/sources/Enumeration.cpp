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

#include "Enumeration.h"

using namespace std;

namespace Enumeration {

vector<vector<int>> conflictFree_clauses(AF& af)
{
    vector<vector<int>> clauses;
    for (int i = 0; i < af.atts.size(); i++) {
        vector<int> clause;
        clause.push_back(-af.atts[i].first);
        clause.push_back(-af.atts[i].second);
        clauses.push_back(clause);
    }
    return clauses;
}

vector<vector<int>> admissible_clauses(AF& af)
{
    vector<vector<int>> clauses = conflictFree_clauses(af);
    for (int i = 0; i < af.atts.size(); i++) {
        vector<int> clause;
        clause.push_back(-af.atts[i].second);
        clause.insert(clause.end(), af.attackers[af.atts[i].first].begin(), af.attackers[af.atts[i].first].end());
        clauses.push_back(clause);
    }
    return clauses;
}

vector<vector<int>> complete_clauses(AF& af)
{
    vector<vector<int>> clauses = admissible_clauses(af);
    for (int i = 0; i < af.args.size(); i++) {
        vector<int> clause;
        for (int j = 0; j < af.attackers[af.args[i]].size(); j++) {
            clause.push_back(-af.defendVar[af.attackers[af.args[i]][j]]);
        }
        clause.push_back(af.args[i]);
        clauses.push_back(clause);
    }
    for (int i = 0; i < af.args.size(); i++) {
        vector<int> clause;
        clause.push_back(-af.defendVar[af.args[i]]);
        clause.insert(clause.end(), af.attackers[af.args[i]].begin(), af.attackers[af.args[i]].end());
        clauses.push_back(clause);
    }
    for (int i = 0; i < af.args.size(); i++) {
        for (int j = 0; j < af.attackers[af.args[i]].size(); j++) {
            vector<int> clause;
            clause.push_back(-af.attackers[af.args[i]][j]);
            clause.push_back(af.defendVar[af.args[i]]);
            clauses.push_back(clause);
        }
    }
    return clauses;
}

vector<vector<int>> stable_clauses(AF& af)
{
    vector<vector<int>> clauses = conflictFree_clauses(af);
    for (int i = 0; i < af.args.size(); i++) {
        vector<int> clause;
        clause.push_back(af.args[i]);
        clause.insert(clause.end(), af.attackers[af.args[i]].begin(), af.attackers[af.args[i]].end());
        clauses.push_back(clause);
    }
    return clauses;
}

}

/*
vector<vector<string>> enumerate(AF& af, string sem)
{
	clock_t t = clock();
	double top = af.args.size()+1.0;
	vector<vector<int>> clauses;
	vector<double> weights;
	double weight;
	vector<int> solution;
	vector<vector<string>> extensions;

	if (sem == "cf" || sem == "stg") {
		clauses = conflictFree_clauses(af);
	} else if (sem == "adm" || sem == "sem" || sem == "prf") {
		clauses = admissible_clauses(af);
	} else if (sem == "com" || sem == "grd") {
		clauses = complete_clauses(af);
	} else if (sem == "stb") {
		clauses = stable_clauses(af);
	}

	for (int i = 0; i < clauses.size(); i++) {
		for (int j = 0; j < clauses[i].size(); j++) {
			cout << clauses[i][j] << " ";
		}
		cout << "0\n";
	}

	for (int i = 0; i < clauses.size(); i++) {
		weights.push_back(top);
	}

	t = clock() - t;
	cout << "init: " << (double)t/CLOCKS_PER_SEC << "s\n";

	if (sem == "cf" || sem == "adm" || sem == "com" || sem == "stb") {
		if (MAXSAT::initialize(top, weights, clauses)) {
			t = clock();
			int count = 0;
			while (MAXSAT::getSolution(weight, solution)) {
				t = clock() - t;
				count++;
				cout << "ext " << count << ": " << (double)t/CLOCKS_PER_SEC << "s\n";
				vector<string> temp;
				for (int i = 0; i < solution.size(); i++) {
					int val = solution[i];
					if (val > 0 && val <= af.args.size()) temp.push_back(af.intToArg[val]);
				}
				extensions.push_back(temp);
				MAXSAT::forbidLastModel();
				t = clock();
			}
			return extensions;
		}
	} else {
		if (sem == "prf") {
			for (int i = 0; i < af.args.size(); i++) {
				vector<int> clause;
				clause.push_back(af.args[i]);
				clauses.push_back(clause);
				weights.push_back(1.0);
			}
		} else if (sem == "sem" || sem == "stg") {
			for (int i = 0; i < af.args.size(); i++) {
				vector<int> clause;
				clause.push_back(af.args[i]);
				clause.insert(clause.end(),af.attackers[af.args[i]].begin(),af.attackers[af.args[i]].end());
				clauses.push_back(clause);
				weights.push_back(1.0);
			}
		} else if (sem == "grd") {
			for (int i = 0; i < af.args.size(); i++) {
				vector<int> clause;
				clause.push_back(-af.args[i]);
				clauses.push_back(clause);
				weights.push_back(1.0);
			}
		}
		if (MAXSAT::initialize(top, weights, clauses)) {
			t = clock();
			int count = 0;
			bool flag = false;
			while (MAXSAT::getSolution(weight, solution)) {
				if (flag && weight) break;
				t = clock() - t;
				count++;
				cout << count << ": " << (double)t/CLOCKS_PER_SEC << "s\n";
				vector<string> temp;
				for (int i = 0; i < solution.size(); i++) {
					int val = solution[i];
					if (val > 0 && val <= af.args.size()) temp.push_back(af.intToArg[val]);
				}
				extensions.push_back(temp);
				if (!weight) {
					flag = true;
					MAXSAT::forbidLastModel();
				} else {
					MAXSAT::forbidLastHS();
				}
				t = clock();
			}
			return extensions;
		}
	}
	return extensions;
}
*/
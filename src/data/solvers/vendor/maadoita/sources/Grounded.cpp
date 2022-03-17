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

#include "Grounded.h"
#include <iostream>

using namespace std;

void print_ext(AF& af, vector<int> ext) {
	for (int i = 0; i < ext.size(); i++) {
		cout << af.intToArg[ext[i]];
		if (i != ext.size()-1) {
			cout << ",";
		}
	}
	cout << "\n";
}

vector<int> grounded_extension(AF& af)
{
	vector<int> accepted;
	map<int,bool> is_accepted;
	vector<int> rejected;
	map<int,bool> is_rejected;
	while (true) {
		vector<int> next;
		bool fixpoint = true;
		for (int i = 0; i < af.args.size(); i++) {
			if (!is_accepted[af.args[i]] && !is_rejected[af.args[i]]) {
				bool arg_defended = true;
				for (int j = 0; j < af.attackers[af.args[i]].size(); j++) {
					bool attack_countered = false;
					for (int k = 0; k < accepted.size(); k++) {
						if (af.att_exists[make_pair(accepted[k], af.attackers[af.args[i]][j])]) {
							attack_countered = true;
							break;
						}
					}
					if (!attack_countered) {
						arg_defended = false;
						break;
					}
				}
				if (arg_defended) {
					next.push_back(af.args[i]);
					fixpoint = false;
				}
			}
		}
		if (fixpoint) {
			break;
		}
		for (int i = 0; i < next.size(); i++) {
			accepted.push_back(next[i]);
			is_accepted[next[i]] = true;
			for (int j = 0; j < af.range[next[i]].size(); j++) {
				if (!is_rejected[af.range[next[i]][j]]) {
					rejected.push_back(af.range[next[i]][j]);
					is_rejected[af.range[next[i]][j]] = true;
				}
			}
		}
	}
	return accepted;
}

map<int,int> grounded_labeling(AF& af) {
	map<int,int> arg_to_label;
	vector<int> accepted;
	map<int,bool> is_accepted;
	vector<int> rejected;
	map<int,bool> is_rejected;
	while (true) {
		vector<int> next;
		bool fixpoint = true;
		for (int i = 0; i < af.args.size(); i++) {
			if (!is_accepted[af.args[i]] && !is_rejected[af.args[i]]) {
				bool arg_defended = true;
				for (int j = 0; j < af.attackers[af.args[i]].size(); j++) {
					bool attack_countered = false;
					for (int k = 0; k < accepted.size(); k++) {
						if (af.att_exists[make_pair(accepted[k], af.attackers[af.args[i]][j])]) {
							attack_countered = true;
							break;
						}
					}
					if (!attack_countered) {
						arg_defended = false;
						break;
					}
				}
				if (arg_defended) {
					next.push_back(af.args[i]);
					fixpoint = false;
				}
			}
		}
		if (fixpoint) {
			break;
		}
		for (int i = 0; i < next.size(); i++) {
			accepted.push_back(next[i]);
			is_accepted[next[i]] = true;
			arg_to_label[next[i]] = ACCEPTED;
			for (int j = 0; j < af.range[next[i]].size(); j++) {
				if (!is_rejected[af.range[next[i]][j]]) {
					rejected.push_back(af.range[next[i]][j]);
					is_rejected[af.range[next[i]][j]] = true;
					arg_to_label[af.range[next[i]][j]] = REJECTED;
				}
			}
		}
	}
	return arg_to_label;
}

bool is_grounded(AF& af, vector<int> & subset) {
	vector<int> extension;
	map<int,bool> in_extension;
	map<int,bool> in_subset;
	for (int i = 0; i < subset.size(); i++) {
		in_subset[subset[i]] = true;
	}
	while (true) {
		vector<int> next;
		bool fixpoint = true;
		for (int i = 0; i < af.args.size(); i++) {
			if (!in_extension[af.args[i]]) {
				bool arg_defended = true;
				for (int j = 0; j < af.attackers[af.args[i]].size(); j++) {
					bool attack_countered = false;
					for (int k = 0; k < extension.size(); k++) {
						if (af.att_exists[make_pair(extension[k], af.attackers[af.args[i]][j])]) {
							attack_countered = true;
							break;
						}
					}
					if (!attack_countered) {
						arg_defended = false;
						break;
					}
				}
				if (arg_defended) {
					if (!in_subset[af.args[i]]) {
						return false;
					}
					next.push_back(af.args[i]);
					fixpoint = false;
				}
			}
		}
		if (fixpoint) {
			break;
		}
		for (int i = 0; i < next.size(); i++) {
			extension.push_back(next[i]);
			in_extension[next[i]] = true;
		}
	}
	for (int i = 0; i < subset.size(); i++) {
		if (!in_extension[subset[i]]) {
			return false;
		}
	}
	return true;
}

bool is_subset_of_grounded(AF& af, vector<int> & subset) {
	vector<int> extension;
	map<int,bool> in_extension;
	while (true) {
		vector<int> next;
		bool fixpoint = true;
		for (int i = 0; i < af.args.size(); i++) {
			if (!in_extension[af.args[i]]) {
				bool arg_defended = true;
				for (int j = 0; j < af.attackers[af.args[i]].size(); j++) {
					bool attack_countered = false;
					for (int k = 0; k < extension.size(); k++) {
						if (af.att_exists[make_pair(extension[k], af.attackers[af.args[i]][j])]) {
							attack_countered = true;
							break;
						}
					}
					if (!attack_countered) {
						arg_defended = false;
						break;
					}
				}
				if (arg_defended) {
					next.push_back(af.args[i]);
					fixpoint = false;
				}
			}
		}
		if (fixpoint) {
			break;
		}
		for (int i = 0; i < next.size(); i++) {
			extension.push_back(next[i]);
			in_extension[next[i]] = true;
		}
	}
	for (int i = 0; i < subset.size(); i++) {
		if (!in_extension[subset[i]]) {
			return false;
		}
	}
	return true;
}
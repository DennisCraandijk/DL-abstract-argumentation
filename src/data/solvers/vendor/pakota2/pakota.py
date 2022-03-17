#!/usr/bin/env python3

## Copyright (c) <2019> <Andreas Niskanen, University of Helsinki>
## 
## 
## 
## Permission is hereby granted, free of charge, to any person obtaining a copy
## of this software and associated documentation files (the "Software"), to deal
## in the Software without restriction, including without limitation the rights
## to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
## copies of the Software, and to permit persons to whom the Software is
## furnished to do so, subject to the following conditions:
## 
## 
## 
## The above copyright notice and this permission notice shall be included in
## all copies or substantial portions of the Software.
## 
## 
## 
## THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
## IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
## FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
## AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
## LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
## OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
## THE SOFTWARE.

from pysat.examples.rc2 import RC2
from pysat.formula import WCNF
from pysat.solvers import Glucose3

import os
import sys
import itertools

class AF():

    def __init__(self, args, atts, mode='', semantics='', enfs=[], pos_enfs=[], neg_enfs=[]):
        self.args = list(range(len(args)))
        self.int_to_arg = args
        self.arg_to_int = { self.int_to_arg[arg] : arg for arg in self.args }
        self.atts = [(self.arg_to_int[s], self.arg_to_int[t]) for s,t in atts]
        self.mode = mode
        self.var_counter = itertools.count(1)
        if mode == 'static':
            self.attackers = { a : [] for a in self.args }
            for a,b in self.atts:
                self.attackers[b].append(a)
            self.arg_accepted_var = { a : next(self.var_counter) for a in self.args }
            self.arg_rejected_var = { a : next(self.var_counter) for a in self.args }
        self.att_exists = { (a,b) : False for a in self.args for b in self.args }
        for a,b in self.atts:
            self.att_exists[(a,b)] = True
        if mode == 'strict' or mode == 'nonstrict':
            self.enforce = { arg: False for arg in self.args }
            for enf in enfs:
                self.enforce[self.arg_to_int[enf]] = True
        elif mode == 'cred' or mode == 'skept':
            self.pos_enfs = [self.arg_to_int[p] for p in pos_enfs]
            self.neg_enfs = [self.arg_to_int[n] for n in neg_enfs]
            self.enforce = { arg: 0 for arg in self.args }
            for enf in self.pos_enfs:
                self.enforce[enf] = 1
            for enf in self.neg_enfs:
                self.enforce[enf] = -1
        if mode == 'strict':
            self.att_var = { (a,b) : next(self.var_counter) for a in self.args for b in self.args \
                         if not self.enforce[a] or not self.enforce[b] }
            self.var_to_att = { self.att_var[(a,b)] : (a,b) for a in self.args for b in self.args \
                                if not self.enforce[a] or not self.enforce[b] }
            if semantics == 'com' or semantics == 'prf':
                self.att_and_no_counterattack_var = { (a,b) : next(self.var_counter) for a in self.args for b in self.args \
                                                      if not self.enforce[a] and not self.enforce[b] }
        elif mode == 'nonstrict':
            self.att_var = { (a,b) : next(self.var_counter) for a in self.args for b in self.args \
                         if not self.enforce[a] or not self.enforce[b] }
            self.var_to_att = { self.att_var[(a,b)] : (a,b) for a in self.args for b in self.args \
                                if not self.enforce[a] or not self.enforce[b] }
            self.arg_accepted_var = { a : next(self.var_counter) for a in self.args if not self.enforce[a] }
            self.att_exists_and_source_accepted_var = { (a,b) : next(self.var_counter) for a in self.args for b in self.args \
                                                         if not self.enforce[a] and not self.enforce[b] }
            if semantics == 'adm' or semantics == 'com' or semantics == 'prf':
                self.att_exists_and_target_accepted_var = { (a,b) : next(self.var_counter) for a in self.args for b in self.args \
                                                             if not self.enforce[a] and not self.enforce[b] }
        elif mode == 'cred':
            self.att_var = { (a,b) : next(self.var_counter) for a in self.args for b in self.args }
            self.var_to_att = { self.att_var[(a,b)] : (a,b) for a in self.args for b in self.args }
            self.arg_accepted_in_witness_var = { (p,a) : next(self.var_counter) for p in self.pos_enfs for a in self.args }
            self.att_exists_and_source_accepted_in_witness_var = { (p,a,b) : next(self.var_counter) for p in self.pos_enfs for a in self.args for b in self.args }
        elif mode == 'skept':
            self.att_var = { (a,b) : next(self.var_counter) for a in self.args for b in self.args }
            self.var_to_att = { self.att_var[(a,b)] : (a,b) for a in self.args for b in self.args }
            if len(neg_enfs) == 0:
                self.arg_accepted_var = { a : next(self.var_counter) for a in self.args }
                self.att_exists_and_source_accepted_var = { (a,b) : next(self.var_counter) for a in self.args for b in self.args }
                if semantics == 'adm':
                    self.att_exists_and_target_accepted_var = { (a,b) : next(self.var_counter) for a in self.args for b in self.args }
            else:
                self.arg_accepted_in_witness_var = { (n,a) : next(self.var_counter) for n in self.neg_enfs for a in self.args }
                self.att_exists_and_source_accepted_in_witness_var = { (n,a,b) : next(self.var_counter) for n in self.neg_enfs for a in self.args for b in self.args }

    def number_of_conflicts(self):
        return len([(a,b) for (a,b) in self.atts if self.enforce[a] and self.enforce[b]])

    def print(self, int_to_arg):
        for a in self.args:
            print('arg(' + str(int_to_arg[a]) + ').')
        for a,b in self.atts:
            print('att(' + str(int_to_arg[a]) + ',' + str(int_to_arg[b]) + ').')

    def print_to_file(self, int_to_arg, filename):
        f = open(filename, "w")
        for a in self.args:
            f.write('arg(' + str(int_to_arg[a]) + ').\n')
        for a,b in self.atts:
            f.write('att(' + str(int_to_arg[a]) + ',' + str(int_to_arg[b]) + ').\n')
        f.close()

def add_arg_rejected_var_clauses(af, solver):
    for a in af.args:
        solver.add_clause([-af.arg_rejected_var[a]] + [af.arg_accepted_var[b] for b in af.attackers[a]])
        for b in af.attackers[a]:
            solver.add_clause([af.arg_rejected_var[a], -af.arg_accepted_var[b]])

def add_cf_clauses(af, solver):
    for a,b in af.atts:
        solver.add_clause([-af.arg_accepted_var[a], -af.arg_accepted_var[b]])

def add_adm_clauses(af, solver):
    add_cf_clauses(af, solver)
    add_arg_rejected_var_clauses(af, solver)
    for a in af.args:
        for b in af.attackers[a]:
            solver.add_clause([-af.arg_accepted_var[a], af.arg_rejected_var[b]])

def add_com_clauses(af, solver):
    add_adm_clauses(af, solver)
    for a in af.args:
        solver.add_clause([af.arg_accepted_var[a]] + [-af.arg_rejected_var[b] for b in af.attackers[a]])

def add_stb_clauses(af, solver):
    add_cf_clauses(af, solver)
    add_arg_rejected_var_clauses(af, solver)
    for a in af.args:
        solver.add_clause([af.arg_accepted_var[a], af.arg_rejected_var[a]])

def add_att_and_no_counterattack_var_clauses(af, solver):
    for a in af.args:
        if not af.enforce[a]:
            for b in af.args:
                if not af.enforce[b]:
                    solver.add_clause([-af.att_and_no_counterattack_var[(a,b)], af.att_var[(b,a)]])
                    for c in af.args:
                        if af.enforce[c]:
                            solver.add_clause([-af.att_and_no_counterattack_var[(a,b)], -af.att_var[(c,b)]])
                    solver.add_clause([af.att_and_no_counterattack_var[(a,b)]] + [-af.att_var[(b,a)]] \
                                      + [af.att_var[(c,b)] for c in af.args if af.enforce[c]])

def add_adm_strict_clauses(af, solver):
    for a in af.args:
        if af.enforce[a]:
            for b in af.args:
                if not af.enforce[b]:
                    solver.add_clause([-af.att_var[(b,a)]] + [af.att_var[(c,b)] for c in af.args if af.enforce[c]])

def add_com_strict_clauses(af, solver):
    add_adm_strict_clauses(af, solver)
    add_att_and_no_counterattack_var_clauses(af, solver)
    for a in af.args:
        if not af.enforce[a]:
            solver.add_clause([af.att_var[(b,a)] for b in af.args if af.enforce[b]] \
                              + [af.att_and_no_counterattack_var[(a,b)] for b in af.args if not af.enforce[b]])

def add_stb_strict_clauses(af, solver):
    for a in af.args:
        if not af.enforce[a]:
            solver.add_clause([af.att_var[(b,a)] for b in af.args if af.enforce[b]])

def add_att_exists_and_source_accepted_var_clauses(af, solver):
    if af.mode != "skept":
        for a in af.args:
            if not af.enforce[a]:
                for b in af.args:
                    if not af.enforce[b]:
                        solver.add_clause([-af.att_exists_and_source_accepted_var[(a,b)], af.att_var[(a,b)]])
                        solver.add_clause([-af.att_exists_and_source_accepted_var[(a,b)], af.arg_accepted_var[a]])
                        solver.add_clause([af.att_exists_and_source_accepted_var[(a,b)], -af.att_var[(a,b)], -af.arg_accepted_var[a]])
    else:
        for a in af.args:
            for b in af.args:
                solver.add_clause([-af.att_exists_and_source_accepted_var[(a,b)], af.att_var[(a,b)]])
                solver.add_clause([-af.att_exists_and_source_accepted_var[(a,b)], af.arg_accepted_var[a]])
                solver.add_clause([af.att_exists_and_source_accepted_var[(a,b)], -af.att_var[(a,b)], -af.arg_accepted_var[a]])

def add_att_exists_and_target_accepted_var_clauses(af, solver):
    if af.mode != "skept":
        for a in af.args:
            if not af.enforce[a]:
                for b in af.args:
                    if not af.enforce[b]:
                        solver.add_clause([-af.att_exists_and_target_accepted_var[(a,b)], af.att_var[(a,b)]])
                        solver.add_clause([-af.att_exists_and_target_accepted_var[(a,b)], af.arg_accepted_var[b]])
                        solver.add_clause([af.att_exists_and_target_accepted_var[(a,b)], -af.att_var[(a,b)], -af.arg_accepted_var[b]])
    else:
        for a in af.args:
            for b in af.args:
                solver.add_clause([-af.att_exists_and_target_accepted_var[(a,b)], af.att_var[(a,b)]])
                solver.add_clause([-af.att_exists_and_target_accepted_var[(a,b)], af.arg_accepted_var[b]])
                solver.add_clause([af.att_exists_and_target_accepted_var[(a,b)], -af.att_var[(a,b)], -af.arg_accepted_var[b]])

def add_cf_nonstrict_clauses(af, solver):
    if af.mode != "skept":
        for a in af.args:
            for b in af.args:
                if not af.enforce[a] and not af.enforce[b]:
                    solver.add_clause([-af.att_var[(a,b)], -af.arg_accepted_var[a], -af.arg_accepted_var[b]])
                elif not af.enforce[a]:
                    solver.add_clause([-af.att_var[(a,b)], -af.arg_accepted_var[a]])
                elif not af.enforce[b]:
                    solver.add_clause([-af.att_var[(a,b)], -af.arg_accepted_var[b]])
    else:
        for a in af.args:
            for b in af.args:
                solver.add_clause([-af.att_var[(a,b)], -af.arg_accepted_var[a], -af.arg_accepted_var[b]])

def add_adm_nonstrict_clauses(af, solver):
    add_cf_nonstrict_clauses(af, solver)
    add_att_exists_and_source_accepted_var_clauses(af, solver)
    add_att_exists_and_target_accepted_var_clauses(af, solver)
    if af.mode != "skept":
        for a in af.args:
            if not af.enforce[a]:
                for b in af.args:
                    if not af.enforce[b]:
                        solver.add_clause([-af.att_exists_and_target_accepted_var[(b,a)]] \
                                      + [af.att_exists_and_source_accepted_var[(c,b)] for c in af.args if not af.enforce[c]] \
                                      + [af.att_var[(c,b)] for c in af.args if af.enforce[c]])
            else:
                for b in af.args:
                    if not af.enforce[b]:
                        solver.add_clause([-af.att_var[(b,a)]] \
                                      + [af.att_exists_and_source_accepted_var[(c,b)] for c in af.args if not af.enforce[c]] \
                                      + [af.att_var[(c,b)] for c in af.args if af.enforce[c]])
    else:
        for a in af.args:
            for b in af.args:
                solver.add_clause([-af.att_exists_and_target_accepted_var[(b,a)]] \
                                      + [af.att_exists_and_source_accepted_var[(c,b)] for c in af.args])

def add_stb_nonstrict_clauses(af, solver):
    add_cf_nonstrict_clauses(af, solver)
    add_att_exists_and_source_accepted_var_clauses(af, solver)
    if af.mode != "skept":
        for a in af.args:
            if not af.enforce[a]:
                solver.add_clause([af.arg_accepted_var[a]] \
                              + [af.att_exists_and_source_accepted_var[(b,a)] for b in af.args if not af.enforce[b]] \
                              + [af.att_var[(b,a)] for b in af.args if af.enforce[b]])
    else:
        for a in af.args:
            solver.add_clause([af.arg_accepted_var[a]] + [af.att_exists_and_source_accepted_var[(b,a)] for b in af.args])

def add_att_exists_and_source_accepted_in_witness_clauses(af, arg, solver):
    for a in af.args:
        for b in af.args:
            solver.add_clause([-af.att_exists_and_source_accepted_in_witness_var[(arg,a,b)], af.att_var[(a,b)]])
            solver.add_clause([-af.att_exists_and_source_accepted_in_witness_var[(arg,a,b)], af.arg_accepted_in_witness_var[(arg,a)]])
            solver.add_clause([af.att_exists_and_source_accepted_in_witness_var[(arg,a,b)], -af.att_var[(a,b)], -af.arg_accepted_in_witness_var[(arg,a)]])

def add_arg_in_cf_ext_clauses(af, arg, solver):
    for a in af.args:
        for b in af.args:
            solver.add_clause([-af.att_var[(a,b)], -af.arg_accepted_in_witness_var[(arg, a)], -af.arg_accepted_in_witness_var[(arg, b)]])

def add_arg_in_adm_ext_clauses(af, arg, solver):
    add_arg_in_cf_ext_clauses(af, arg, solver)
    add_att_exists_and_source_accepted_in_witness_clauses(af, arg, solver)
    for a in af.args:
        for b in af.args:
            solver.add_clause([-af.att_var[(b,a)], -af.arg_accepted_in_witness_var[(arg, a)]]
                              + [af.att_exists_and_source_accepted_in_witness_var[(arg, c, b)] for c in af.args])

def add_arg_in_stb_ext_clauses(af, arg, solver):
    add_arg_in_cf_ext_clauses(af, arg, solver)
    add_att_exists_and_source_accepted_in_witness_clauses(af, arg, solver)
    for a in af.args:
        solver.add_clause([af.arg_accepted_in_witness_var[(arg, a)]]
                          + [af.att_exists_and_source_accepted_in_witness_var[(arg, b, a)] for b in af.args])

def add_soft_clauses(af, solver):
    for a in af.args:
        for b in af.args:
            if (af.mode != "strict" and af.mode != "nonstrict") or (not af.enforce[a] or not af.enforce[b]):
                if af.att_exists[(a,b)]:
                    solver.add_clause([af.att_var[(a,b)]], weight=1)
                else:
                    solver.add_clause([-af.att_var[(a,b)]], weight=1)

def add_refinement_clause(af, solver, new_af, model=None):
    if model == None:
        if af.mode == "strict" or af.mode == "nonstrict":
            solver.add_clause([(-1 if new_af.att_exists[(a,b)] else 1)*af.att_var[(a,b)] \
                                for a in af.args for b in af.args if not af.enforce[a] or not af.enforce[b]])
        else:
            solver.add_clause([(-1 if new_af.att_exists[(a,b)] else 1)*af.att_var[(a,b)] for a in af.args for b in af.args])
    else:
        labeling = { a : 0 for a in new_af.args }
        for a in new_af.args:
            if model[new_af.arg_accepted_var[a]-1] > 0:
                labeling[a] = 1
            elif model[new_af.arg_rejected_var[a]-1] > 0:
                labeling[a] = -1
        clause = []
        for a in new_af.args:
            for b in new_af.args:
                if new_af.att_exists[(a,b)]:
                    if labeling[a] == 1 and labeling[b] == -1:
                        clause += [-af.att_var[(a,b)]]
                else:
                    if (labeling[a] == 1 and labeling[b] == 1) or (labeling[a] == 0 and labeling[b] == 1):
                        if af.mode == "cred" or af.enforce[a] != 1 or af.enforce[b] != 1:
                            clause += [af.att_var[(a,b)]]
        solver.add_clause(clause)

def enforce_extension(af, mode, sem, solver, strong=True):
    if mode == 'strict':
        if sem == 'adm':
            add_adm_strict_clauses(af, solver)
        elif sem == 'com' or sem == 'prf':
            add_com_strict_clauses(af, solver)
        elif sem == 'stb':
            add_stb_strict_clauses(af, solver)
    else:
        if sem == 'adm' or sem == 'com' or sem == 'prf':
            add_adm_nonstrict_clauses(af, solver)
        elif sem == 'stb':
            add_stb_nonstrict_clauses(af, solver)
    add_soft_clauses(af, solver)
    if not (mode == 'strict' and sem == 'prf'):
        model = solver.compute()
        print('s OPTIMUM FOUND')
        print('o {0}'.format(solver.cost + af.number_of_conflicts()))
        new_atts = []
        for i in model:
            if i > 0 and i in af.var_to_att:
                new_atts += [af.var_to_att[i]]
        return AF(af.args, new_atts)
    count = 0
    while True:
        count += 1
        model = solver.compute()
        new_atts = []
        for i in model:
            if i > 0 and i in af.var_to_att:
                new_atts += [af.var_to_att[i]]
        new_af = AF(af.args, new_atts, mode='static')
        sat_solver = Glucose3()
        add_com_clauses(new_af, sat_solver)
        for a in new_af.args:
            if af.enforce[a]:
                sat_solver.add_clause([new_af.arg_accepted_var[a]])
        sat_solver.add_clause([new_af.arg_accepted_var[a] for a in new_af.args if not af.enforce[a]])
        result = sat_solver.solve()
        if result == False:
            print('Number of iterations:', count)
            print('s OPTIMUM FOUND')
            print('o {0}'.format(solver.cost + af.number_of_conflicts()))
            return new_af
        add_refinement_clause(af, solver, new_af, sat_solver.get_model() if strong else None)

def enforce_status(af, model, sem, solver, strong=True):
    if mode == 'cred' and len(af.neg_enfs) == 0:
        for p in af.pos_enfs:
            solver.add_clause([af.arg_accepted_in_witness_var[(p,p)]])
            if sem == 'adm':
                add_arg_in_adm_ext_clauses(af, p, solver)
            elif sem == 'stb':
                add_arg_in_stb_ext_clauses(af, p, solver)
            else:
                print('Semantics', sem, 'not supported with mode', mode + '.')
                sys.exit(1)
        add_soft_clauses(af, solver)
        model = solver.compute()
        print('s OPTIMUM FOUND')
        print('o {0}'.format(solver.cost))
        new_atts = []
        for i in model:
            if i > 0 and i in af.var_to_att:
                new_atts += [af.var_to_att[i]]
        return AF(af.args, new_atts)
    if mode == 'cred':
        for p in af.pos_enfs:
            solver.add_clause([af.arg_accepted_in_witness_var[(p,p)]])
            for n in af.neg_enfs:
                solver.add_clause([-af.arg_accepted_in_witness_var[(p,n)]])
            if sem == 'adm':
                add_arg_in_adm_ext_clauses(af, p, solver)
            elif sem == 'stb':
                add_arg_in_stb_ext_clauses(af, p, solver)
            else:
                print('Semantics', sem, 'not supported with mode', mode + '.')
                sys.exit(1)
        add_soft_clauses(af, solver)
        count = 0
        while True:
            count += 1
            model = solver.compute()
            new_atts = []
            for i in model:
                if i > 0 and i in af.var_to_att:
                    new_atts += [af.var_to_att[i]]
            new_af = AF(af.args, new_atts, mode='static')
            sat_solver = Glucose3()
            if sem == 'adm':
                add_adm_clauses(new_af, sat_solver)
            elif sem == 'stb':
                add_stb_clauses(new_af, sat_solver)
            sat_solver.add_clause([new_af.arg_accepted_var[n] for n in af.neg_enfs])
            result = sat_solver.solve()
            if result == False:
                print('Number of iterations:', count)
                print('s OPTIMUM FOUND')
                print('o {0}'.format(solver.cost))
                return new_af
            add_refinement_clause(af, solver, new_af, sat_solver.get_model() if strong else None)
    elif mode == 'skept':
        if len(af.neg_enfs) == 0:
            if sem == 'adm':
                add_adm_nonstrict_clauses(af, solver)
                for a in af.args:
                    if af.enforce[a]:
                        solver.add_clause([af.arg_accepted_var[a]])
            elif sem == 'stb':
                add_stb_nonstrict_clauses(af, solver)
                for a in af.args:
                    if af.enforce[a]:
                        solver.add_clause([af.arg_accepted_var[a]])
            else:
                print('Semantics', sem, 'not supported with mode', mode + '.')
        for n in af.neg_enfs:
            solver.add_clause([-af.arg_accepted_in_witness_var[(n,n)]])
            for p in af.pos_enfs:
                solver.add_clause([af.arg_accepted_in_witness_var[(n,p)]])
            if sem == 'adm':
                add_arg_in_adm_ext_clauses(af, n, solver)
            elif sem == 'stb':
                add_arg_in_stb_ext_clauses(af, n, solver)
            else:
                print('Semantics', sem, 'not supported with mode', mode + '.')
        add_soft_clauses(af, solver)
        count = 0
        while True:
            count += 1
            model = solver.compute()
            new_atts = [af.var_to_att[i] for i in model if i > 0 and i in af.var_to_att]
            new_af = AF(af.args, new_atts, mode='static')
            sat_solver = Glucose3()
            if sem == 'adm':
                add_adm_clauses(new_af, sat_solver)
                sat_solver.add_clause([new_af.arg_accepted_var[a] for a in new_af.args])
            elif sem == 'stb':
                add_stb_clauses(new_af, sat_solver)
            sat_solver.add_clause([-new_af.arg_accepted_var[p] for p in af.pos_enfs])
            result = sat_solver.solve()
            if result == False:
                print('Number of iterations:', count)
                print('s OPTIMUM FOUND')
                print('o {0}'.format(solver.cost))
                return new_af
            add_refinement_clause(af, solver, new_af, sat_solver.get_model() if strong else None)

def parse_af_and_enfs(filename, mode):
    lines = open(filename, 'r').read().split('\n')
    args = [line.replace('arg(', '').replace(').', '') for line in lines if line.startswith('arg')]
    atts = [line.replace('att(', '').replace(').', '').split(',') for line in lines if line.startswith('att')]
    if mode == 'strict' or mode == 'nonstrict':
        enfs = [line.replace('enf(', '').replace(').', '') for line in lines if line.startswith('enf')]
        return args, atts, enfs
    elif mode == 'cred' or mode == 'skept':
        pos_enfs = [line.replace('pos(', '').replace(').', '') for line in lines if line.startswith('pos')]
        neg_enfs = [line.replace('neg(', '').replace(').', '') for line in lines if line.startswith('neg')]
        return args, atts, pos_enfs, neg_enfs

def print_usage():
    print('Usage:', os.path.basename(sys.argv[0]), 'file mode sem')
    print('Arguments:')
    print('    file : Input filename for enforcement instance in apx format.')
    print('    mode : Enforcement variant. mode={strict|nonstrict|cred|skept}')
    print('        strict    : strict extension enforcement')
    print('        nonstrict : nonstrict extension enforcement')
    print('        cred      : credulous status enforcement')
    print('        skept     : skeptical status enforcement')
    print('    sem  : Argumentation semantics. sem={adm|com|prf|stb}')
    print('        adm       : admissible')
    print('        com       : complete')
    print('        prf       : preferred')
    print('        stb       : stable')

if __name__ == '__main__':
    if len(sys.argv) < 4:
        print_usage()
        sys.exit(1)
    filename = sys.argv[1]
    mode = sys.argv[2]
    semantics = sys.argv[3]
    strong = True
    if len(sys.argv) > 4:
        if sys.argv[4] != "strong":
            strong = False
    solver = RC2(WCNF())
    if semantics not in ['adm', 'com', 'prf', 'stb']:
        print('Semantics', semantics, 'not supported.')
        sys.exit(1)
    if (semantics == 'com' or semantics == 'prf') and mode == 'cred':
        semantics = 'adm'
    if mode == 'strict' or mode == 'nonstrict':
        args, atts, enfs = parse_af_and_enfs(filename, mode)
        af = AF(args, atts, mode, semantics, enfs=enfs)
        enforce_extension(af, mode, semantics, solver, strong).print(af.int_to_arg)
    elif mode == 'cred' or mode == 'skept':
        args, atts, pos_enfs, neg_enfs = parse_af_and_enfs(filename, mode)
        af = AF(args, atts, mode, semantics, pos_enfs=pos_enfs, neg_enfs=neg_enfs)
        enforce_status(af, mode, semantics, solver, strong).print(af.int_to_arg)
    else:
        print('Mode', mode, 'not supported.')
        sys.exit(1)

/**************************************************************************************[Solver.cc]
Copyright (c) 2003-2006, Niklas Een, Niklas Sorensson
Copyright (c) 2007-2010, Niklas Sorensson

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
associated documentation files (the "Software"), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge, publish, distribute,
sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or
substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT
OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
**************************************************************************************************/

#include <math.h>

#include "mtl/Sort.h"
#include "core/Solver.h"

// added by nabesima
#include "utils/System.h"

using namespace Minisat;

//=================================================================================================
// Options:


static const char* _cat = "CORE";

static DoubleOption  opt_var_decay          (_cat, "var-decay",   "The variable activity decay factor",            0.95,     DoubleRange(0, false, 1, false));
static DoubleOption  opt_clause_decay       (_cat, "cla-decay",   "The clause activity decay factor",              0.999,    DoubleRange(0, false, 1, false));
static DoubleOption  opt_random_var_freq    (_cat, "rnd-freq",    "The frequency with which the decision heuristic tries to choose a random variable", 0, DoubleRange(0, true, 1, true));
static DoubleOption  opt_random_seed        (_cat, "rnd-seed",    "Used by the random variable selection",         91648253, DoubleRange(0, false, HUGE_VAL, false));
static IntOption     opt_ccmin_mode         (_cat, "ccmin-mode",  "Controls conflict clause minimization (0=none, 1=basic, 2=deep)", 2, IntRange(0, 2));
static IntOption     opt_phase_saving       (_cat, "phase-saving","Controls the level of phase saving (0=none, 1=limited, 2=full)", 2, IntRange(0, 2));
static BoolOption    opt_rnd_init_act       (_cat, "rnd-init",    "Randomize the initial activity", false);
static BoolOption    opt_luby_restart       (_cat, "luby",        "Use the Luby restart sequence", true);
static IntOption     opt_restart_first      (_cat, "rfirst",      "The base restart interval", 100, IntRange(1, INT32_MAX));
static DoubleOption  opt_restart_inc        (_cat, "rinc",        "Restart interval increase factor", 1.3, DoubleRange(1, false, HUGE_VAL, false));
static DoubleOption  opt_garbage_frac       (_cat, "gc-frac",     "The fraction of wasted memory allowed before a garbage collection is triggered",  0.20, DoubleRange(0, false, HUGE_VAL, false));

// added by nabesima
static BoolOption    opt_rup                (_cat, "rup",         "Output a proof of UNSAT in RUP format", false);

// added by nabesima
static const char* _glue = "GLUE";
static IntOption	 opt_learnts_measure    (_glue, "lmeasure",           "The measure of learned clause quality (0=LRU, 1=LBD, 2=strict LBD, 3=pseudo LBD)", 3, IntRange(0, 3));
static IntOption     opt_reduce_db          (_glue, "reduce",             "The measure for reducing learnts (0=LRU, 1=LBD, 2=LBD+LRU", 1, IntRange(0, 2));
static BoolOption    opt_ag_reduce_db       (_glue, "ag-reduce",          "Use the aggressive reduce DB strategy", true);
static BoolOption    opt_lv0_reduce_db      (_glue, "lv0-reduce",         "The reduce DB is applied at DLV 0", false);
static IntOption	 opt_max_lbd            (_glue, "lbd",                "The max LBD of survived learnt clauses in reduce DB (3)", 3, IntRange(2, INT32_MAX));
static IntOption	 opt_max_len            (_glue, "len",                "The max length of survived learnt clauses in reduce DB (3)", 3, IntRange(2, INT32_MAX));
static IntOption     opt_var_decay_strategy (_glue, "var-decay-strategy", "The var-decay parameter strategy (0=const, 1=linear, 2=exp, 3=sigmoid", 0, IntRange(0, 3));
static IntOption     opt_var_decay_period   (_glue, "var-decay-period",   "The parameter for var-decay strategy", 6000, IntRange(0, INT32_MAX));
static DoubleOption  opt_max_var_decay      (_glue, "max-var-decay",      "The maximum var-decay parameter in incremental var-decay strategy", 0.95, DoubleRange(0, false, 1, false));
static BoolOption    opt_lbd_act_bumping    (_glue, "bump",               "Use the LBD based activity bumping strategy", true);
static IntOption     opt_learnts_init       (_glue, "linit",              "The initial size of learnt clauses (20000 in glucose)", 30000, IntRange(1, INT32_MAX));
static IntOption     opt_learnts_inc        (_glue, "linc",               "The factor with which the limit of learnts is added in each reduction (1000 in glucose)", 10000, IntRange(1, INT32_MAX));
static DoubleOption  opt_min_rate_learnts   (_glue, "lmin",               "The min rate of learnts to be preverved at reduction", 0.25, DoubleRange(0, true, 1, true));
static DoubleOption  opt_max_rate_learnts   (_glue, "lmax",               "The max rate of learnts to be preverved at reduction", 0.5, DoubleRange(0, true, 1, true));

// added by nabesima
static const char* _restart = "RESTART";
static IntOption     opt_restart_strategy     (_restart, "restart",              "The restart strategy (0=minisat, 1=LBD avg, 2=DLV avg, 3=LBD+DLV, 4=LBD+DEC)", 4, IntRange(0, 4));
static BoolOption    opt_blocked_restart      (_restart, "blocked-restart",      "Use the blocked restart strategy based on propagations/conflict", true);
static BoolOption	 opt_delayed_restart      (_restart, "delayed-restart",      "Use the delayed restart strategy based on LBD updates", true);
static DoubleOption  opt_lbd_restart_rate     (_restart, "lbd-restart-rate",     "Restarts if local LBD average * this val > global one", 0.8, DoubleRange(0, true, 1, true));
static DoubleOption  opt_dlv_restart_rate     (_restart, "dlv-restart-rate",     "Restarts if local DLV average * this val > global one", 1.0, DoubleRange(0, true, 2, true));
static DoubleOption  opt_dec_restart_rate     (_restart, "dec-restart-rate",     "Restarts if local average of decisions/conflict * this val > global one", 0.95, DoubleRange(0, true, 1, true));

static DoubleOption  opt_blocked_restart_rate (_restart, "blocked-restart-rate", "Blocks if local trail average * this val > global one", 0.5, DoubleRange(0, true, 5, true));
static IntOption     opt_lbd_restart_size     (_restart, "lbd-restart-size",     "The window size of local average for LBDs", 50, IntRange(1, INT32_MAX));
static IntOption     opt_dlv_restart_size     (_restart, "dlv-restart-size",     "The window size of local average for DLVs", 50, IntRange(1, INT32_MAX));
static IntOption     opt_dec_restart_size     (_restart, "dec-restart-size",     "The window size of local average for decisions/conflict", 50, IntRange(1, INT32_MAX));

// added by nabesima
static const char* _simp = "CORE SIMP";
static BoolOption    opt_pure_lit_elim      (_simp, "pure",               "Use pure literal elimination", true);
static BoolOption    opt_self_subsump       (_simp, "ss",                 "Use self-subsumption checking for each resolvent", false);
static BoolOption    opt_rm_fixed_vars      (_simp, "rm-fixed-vars",      "Removes fixded variables from clauses", true);
static BoolOption    opt_rm_dup_bins        (_simp, "rm-dup-bins",        "Removes duplicated binary learnts", false);
static IntOption     opt_premise_source     (_simp, "premise-src",        "Source of premise literals (0=decision literal, 1=last uip, 2=either one)", 0, IntRange(0, 2));
static IntOption     opt_premise_size       (_simp, "premise-size",       "The number of preserved premise literals", 13, IntRange(1, INT_MAX));
static IntOption     opt_lazy_checking      (_simp, "lazy-check",         "Type of lazy checking (0=changed, 1=always)", 0, IntRange(0, 1));
static BoolOption    opt_lazy_fls_probing   (_simp, "lazy-fls-probing",   "Apply lazy false-literal probing", true);
static BoolOption    opt_lazy_pol_probing   (_simp, "lazy-pol-probing",   "Apply lazy polarity probing", true);
static BoolOption    opt_lazy_eqv_probing   (_simp, "lazy-eqv-probing",   "Apply lazy equivalent variable probing", true);
static BoolOption    opt_lazy_bin_probing   (_simp, "lazy-bin-probing",   "Apply lazy binary clause probing", true);
static BoolOption    opt_lazy_bin_shrink    (_simp, "lazy-bin-shrink",    "Apply lazy clause shrinking by binary resolvents", true);
static BoolOption    opt_lazy_bin_subsump   (_simp, "lazy-bin-subsump",   "Apply lazy subsumption checking with binary resolvents", false);
static IntOption     opt_lazy_lrn_skip      (_simp, "lazy-lrn-skip",      "Apply lazy learned clause generation with skip", 1, IntRange(0, 2));
static IntOption     opt_shrink_min_size    (_simp, "shrink-min-size",    "Min size of clauses to be shrinked with detach/attach", 0, IntRange(0, INT_MAX));
static IntOption     opt_rewrite_min_lits   (_simp, "rw-min-lits",        "The minimum number of equivalent literals", 15, IntRange(1, INT32_MAX));
static BoolOption    opt_rand_attach        (_simp, "rand-attach",        "Attach clauses by randomized order", false);
static BoolOption    opt_wl_sorting         (_simp, "wl-sorting",         "Sort watched list before solving", false);

// added by nabesima
static const char* _solver = "SOLVER";
static BoolOption    opt_minisat		    (_solver, "minisat",          "Behaves as MiniSat", false);
static BoolOption    opt_glucose		    (_solver, "glucose",          "Behaves as glucose", false);

// added by nabesima
static double luby(double y, int x);

// Learned clause measure
#define MS_LRU		        0
#define MS_LBD		        1
#define MS_STRICT_LBD       2
#define MS_PSEUDO_LBD       3

// Reduce DB measure
#define RD_LRU              0
#define RD_LBD              1
#define RD_LBD_LRU          2

// Restart strategy
#define RS_MINISAT          0
#define RS_LBD_AVG          1
#define RS_DLV_AVG          2
#define RS_LBD_DLV          3
#define RS_LBD_DEC          4

// Light-weight restart
#define LW_UNUSE            0
#define LW_CALC_ONLY        1
#define LW_USE              2

// Var-decay strategy
#define VD_CONST            0
#define VD_LINEAR           1
#define VD_EXP              2
#define VD_SIGMOID          3

// added by nabesima
void Solver::writeLit(Lit p) const {
    assert(output && rup);
    fprintf(output, "%i 0\n" , (var(p) + 1) * (-2 * sign(p) + 1) );
}
void Solver::writeLits(Lit p, Lit q) const {
    assert(output && rup);
    fprintf(output, "%i %i 0\n" , (var(p) + 1) * (-2 * sign(p) + 1), (var(q) + 1) * (-2 * sign(q) + 1));
}
void Solver::writeLits(vec<Lit>& lits) const {
    assert(output && rup);
    for (int i = 0; i < lits.size(); i++)
      fprintf(output, "%i " , (var(lits[i]) + 1) * (-2 * sign(lits[i]) + 1) );
    fprintf(output, "0\n");
}
void Solver::writeClause(Clause& c) const {
    assert(output && rup);
    for (int i = 0; i < c.size(); i++)
      fprintf(output, "%i " , (var(c[i]) + 1) * (-2 * sign(c[i]) + 1) );
    fprintf(output, "0\n");
}
void Solver::writeDeletedLits(vec<Lit>& lits) const {
    assert(output && rup);
    fprintf(output, "d ");
    for (int i = 0; i < lits.size(); i++)
        fprintf(output, "%i " , (var(lits[i]) + 1) * (-2 * sign(lits[i]) + 1) );
    fprintf(output, "0\n");
}
void Solver::writeDeletedClause(Clause& c) const {
    assert(output && rup);
    fprintf(output, "d ");
    for (int i = 0; i < c.size(); i++)
        fprintf(output, "%i " , (var(c[i]) + 1) * (-2 * sign(c[i]) + 1) );
    fprintf(output, "0\n");
}
void Solver::printLit(Lit l) const {
    if (l != lit_Undef) {
        printf("%s%d:%c", sign(l) ? "-" : "", var(l)+1, value(l) == l_True ? '1' : (value(l) == l_False ? '0' : 'X'));
        if (value(l) != l_Undef) printf("@%d", level(var(l)));
    }
    else
        printf("undef");
}
void Solver::printLits(vec<Lit>& lits) const {
    for (int i=0; i < lits.size(); i++) {
        printLit(lits[i]);
        printf(" ");
    }
}
void Solver::printClause(const Clause& c) const {
    printf("{ ");
    for (int i=0; i < c.size(); i++) {
        printLit(c[i]);
        printf(" ");
    }
    printf("} ");
    if (c.learnt()) printf("lbd=%d ", c.lbd());
    if (c.has_extra()) printf("act=%f", c.activity());
    printf("\n");
}
void Solver::printDecisionStack(int from=0) const {
    int idx = (from == 0) ? 0 : trail_lim[from-1];
    for (int i=from; i < decisionLevel(); i++) {
        printf("DLV%3d: ", i);
        for (; idx < trail_lim[i]; idx++)
            printf("%s%d ", sign(trail[idx]) ? "-" : "", var(trail[idx])+1);
        printf("\n");
    }
    printf("DLV%3d: ", decisionLevel());
    for (; idx < trail.size(); idx++)
        printf("%s%d ", sign(trail[idx]) ? "-" : "", var(trail[idx])+1);
    printf("\n");
}
void Solver::printLog() const {
    printf("| %9d | %7d %8d | %8d %6.0f %6.1f | %7.1f %6.1f %6.2f |\n",
           (int)conflicts,
           (int)dec_vars - (trail_lim.size() == 0 ? trail.size() : trail_lim[0]), nClauses(),
           nLearnts(), (double)learnts_literals/nLearnts(), (double)tot_lbds/nLearnts(),
           (double)propagations / conflicts, wholeDLVs/conflicts, progressEstimate()*100);
    fflush(stdout);
}

//=================================================================================================
// Constructor/Destructor:

Solver::Solver() :

    // Parameters (user settable):
    //
    output           (NULL)               // added by nabesima
  , verbosity        (0)
  , var_decay        (opt_var_decay)
  , clause_decay     (opt_clause_decay)
  , random_var_freq  (opt_random_var_freq)
  , random_seed      (opt_random_seed)
  , luby_restart     (opt_luby_restart)
  , ccmin_mode       (opt_ccmin_mode)
  , phase_saving     (opt_phase_saving)
  , rnd_pol          (false)
  , rnd_init_act     (opt_rnd_init_act)
  , garbage_frac     (opt_garbage_frac)
  , restart_first    (opt_restart_first)
  , restart_inc      (opt_restart_inc)

    // Parameters (the rest):
    //
  , learntsize_factor((double)1/(double)3), learntsize_inc(1.1)

    // Parameters (experimental):
    //
  , learntsize_adjust_start_confl (100)
  , learntsize_adjust_inc         (1.5)

    // Statistics: (formerly in 'SolverStats')
    //
  , solves(0), starts(0), decisions(0), rnd_decisions(0), propagations(0), conflicts(0)
  , dec_vars(0), clauses_literals(0), learnts_literals(0), max_literals(0), tot_literals(0)

    // added by nabesima
  , init_vars(0), simp_vars(0), init_clauses(0), simp_clauses(0)
  , core_clauses_literals(0), tot_lbds(0), curr_restarts(0), lbd_restarts(0), dlv_restarts(0), dec_restarts(0), bcp_restarts(0)
  , blocked_restarts(0), max_confs(0), min_confs(UINT32_MAX)
  , wholeLBDs(0.0), wholeDLVs(0.0)
  , simplify_dbs(0), rewrite_dbs(0), reduce_dbs(0), removed_decisions(0), removed_propagations(0), glue_clauses(0)
  , probings(0), prb_false_lits(0), prb_equiv_vars(0), prb_polarity_lits(0), prb_cla_total_lits(0), pure_lits(0), removed_fixed_vars(0)
  , ind_propagations(0), premise_updates(0), lazy_rm_cla_lits(0), lazy_rm_lrn_lits(0)
  , lazy_subsumed_clauses(0), lazy_subsumed_leartns(0), lazy_skipped_lits(0), lazy_lost_cops(0)
  , ss_rm_cla_lits(0), ss_rm_lrn_lits(0), dup_bin_learnts(0)

  , ok                 (true)
  , cla_inc            (1)
  , var_inc            (1)
  , watches            (WatcherDeleted(ca))
  , qhead              (0)
  , simpDB_assigns     (-1)
  , simpDB_props       (0)
  , order_heap         (VarOrderLt(activity))
  , progress_estimate  (0)
  , remove_satisfied   (true)

    // Resource constraints:
    //
  , conflict_budget    (-1)
  , propagation_budget (-1)
  , asynch_interrupt   (false)

  // added by nabesima
  , rup                  (opt_rup)
  , learnts_measure      (opt_learnts_measure)
  , reduce_db            (opt_reduce_db)
  , ag_reduce_db         (opt_ag_reduce_db)
  , lv0_reduce_db        (opt_lv0_reduce_db)
  , max_lbd              (opt_max_lbd)
  , max_len              (opt_max_len)
  , restart_strategy     (opt_restart_strategy)
  , blocked_restart      (opt_blocked_restart)
  , delayed_restart      (opt_delayed_restart)
  , lbd_restart_rate     (opt_lbd_restart_rate)
  , dlv_restart_rate     (opt_dlv_restart_rate)
  , dec_restart_rate     (opt_dec_restart_rate)
  , blocked_restart_rate (opt_blocked_restart_rate)
  , lbd_restart_size     (opt_lbd_restart_size)
  , dlv_restart_size     (opt_dlv_restart_size)
  , dec_restart_size     (opt_dec_restart_size)

  , var_decay_strategy   (opt_var_decay_strategy)
  , var_decay_period     (opt_var_decay_period)
  , init_var_decay       (opt_var_decay)
  , max_var_decay        (opt_max_var_decay)
  , lbd_act_bumping      (opt_lbd_act_bumping)

  , init_rdb_param       (true)
  , reduce_db_base       (opt_learnts_init)
  , reduce_db_inc	     (opt_learnts_inc)
  , min_rate_learnts     (opt_min_rate_learnts)
  , max_rate_learnts     (opt_max_rate_learnts)
  , reduce_db_limit      (reduce_db_base)
  , rewrited_lits        (0)
  , rewrite_min_lits     (opt_rewrite_min_lits)
  , pure_lit_elim        (opt_pure_lit_elim)
  , self_subsump         (opt_self_subsump)
  , rm_fixed_vars        (opt_rm_fixed_vars)
  , rm_dup_bins          (opt_rm_dup_bins)
  , premise_src          (opt_premise_source)
  , premise_size         (opt_premise_size)
  , lazy_checking        (opt_lazy_checking)
  , lazy_fls_probing     (opt_lazy_fls_probing)
  , lazy_pol_probing     (opt_lazy_pol_probing)
  , lazy_eqv_probing     (opt_lazy_eqv_probing)
  , lazy_bin_probing     (opt_lazy_bin_probing)
  , lazy_bin_shrink      (opt_lazy_bin_shrink)
  , lazy_bin_subsump     (opt_lazy_bin_subsump)
  , lazy_lrn_skip        (opt_lazy_lrn_skip)
  , shrink_min_size      (opt_shrink_min_size)
  , rand_attach          (opt_rand_attach)
  , wl_sorting           (opt_wl_sorting)
  , num_equivalent_lits  (0)

  , lbd_comps            (0)
  , lbd_updates          (0)
{
    // added by nabesima
    if (opt_minisat) {
        restart_inc = 2;
        learnts_measure = MS_LRU;
        reduce_db = RD_LRU;
        ag_reduce_db = false;
        max_len = 2;
        restart_strategy = RS_MINISAT;
        lbd_act_bumping = false;
        delayed_restart = false;
        pure_lit_elim = false;
        rm_fixed_vars = false;
        lazy_fls_probing = false;
        lazy_pol_probing = false;
        lazy_eqv_probing = false;
        lazy_bin_probing = false;
        lazy_bin_shrink = false;
        lazy_lrn_skip = 0;
    }
    else if (opt_glucose) {
        restart_inc = 2;
        learnts_measure = MS_LBD;
        reduce_db = RD_LBD;
        max_lbd = 2;
        max_len = 2;
        restart_strategy = RS_LBD_AVG;
        lbd_restart_rate = 0.7;
        lbd_restart_size = 100;
        reduce_db_base = 20000;
        reduce_db_inc = 1000;
        delayed_restart = false;
        pure_lit_elim = false;
        rm_fixed_vars = false;
        lazy_fls_probing = false;
        lazy_pol_probing = false;
        lazy_eqv_probing = false;
        lazy_bin_probing = false;
        lazy_bin_shrink = false;
        lazy_lrn_skip = 0;
    }
    if (lazy_fls_probing || lazy_pol_probing || lazy_eqv_probing || lazy_bin_probing || lazy_bin_shrink || lazy_bin_subsump || lazy_lrn_skip)
        lazy_simp = true;
    lazy_probed_lits.growTo(PRB_NUM_TYPES);
    lazy_probed_dup_lits.growTo(PRB_NUM_TYPES);
}


Solver::~Solver()
{
}


//=================================================================================================
// Minor methods:


// Creates a new SAT variable in the solver. If 'decision' is cleared, variable will not be
// used as a decision variable (NOTE! This has effects on the meaning of a SATISFIABLE result).
//
Var Solver::newVar(bool sign, bool dvar)
{
    int v = nVars();
    watches  .init(mkLit(v, false));
    watches  .init(mkLit(v, true ));
    assigns  .push(l_Undef);
    vardata  .push(mkVarData(CRef_Undef, 0));
    //activity .push(0);
    activity .push(rnd_init_act ? drand(random_seed) * 0.00001 : 0);
    seen     .push(0);
    polarity .push(sign);
    decision .push();
    trail    .capacity(v+1);
    setDecisionVar(v, dvar);

    // added by nabesima
    lbd_time.push(0);
    premise_lit.push(mkLit(v, false));
    premise_lit.push(mkLit(v, true ));
    if (rup) {
        premise_hash.push(); premise_hash.last().init(mkLit(v, false), premise_size);
        premise_hash.push(); premise_hash.last().init(mkLit(v, true ), premise_size);
    }
    if (lazy_eqv_probing)
        equivalent_lit.push(lit_Undef);

    return v;
}


bool Solver::addClause_(vec<Lit>& ps)
{
    assert(decisionLevel() == 0);
    if (!ok) return false;

    // added by nabesima
    if (rup) rup_tmp.clear();

    // Check if clause is satisfied and remove false/duplicate literals:
    sort(ps);
    Lit p; int i, j;
    for (i = j = 0, p = lit_Undef; i < ps.size(); i++) {
        if (rup) rup_tmp.push(ps[i]);
        if (value(ps[i]) == l_True || ps[i] == ~p)
            return true;
        else if (value(ps[i]) != l_False && ps[i] != p)
            ps[j++] = p = ps[i];
    }
    ps.shrink(i - j);

    // added by nabesima
    if (rup && rup_tmp.size() != ps.size()) {
        writeLits(ps);
        writeDeletedLits(rup_tmp);
    }

    if (ps.size() == 0)
        return ok = false;
    else if (ps.size() == 1){
        uncheckedEnqueue(ps[0]);
        return ok = (propagate() == CRef_Undef);
    }else{
        CRef cr = ca.alloc(ps, false);
        clauses.push(cr);
        attachClause(cr);
    }

    return true;
}

void Solver::attachClause(CRef cr) {
    const Clause& c = ca[cr];
    assert(c.size() > 1);
    watches[~c[0]].push(Watcher(cr, c[1]));
    watches[~c[1]].push(Watcher(cr, c[0]));
    if (c.learnt()) learnts_literals += c.size(), tot_lbds += c.lbd();
    else            clauses_literals += c.size();
}

void Solver::detachClause(CRef cr, bool strict) {
    const Clause& c = ca[cr];
    assert(c.size() > 1);

    if (strict){
        remove(watches[~c[0]], Watcher(cr, c[1]));
        remove(watches[~c[1]], Watcher(cr, c[0]));
    }else{
        // Lazy detaching: (NOTE! Must clean all watcher lists before garbage collecting this clause)
        watches.smudge(~c[0]);
        watches.smudge(~c[1]);
    }

    if (c.learnt()) learnts_literals -= c.size();
    else            clauses_literals -= c.size();
    // added by nabesima
    if (c.learnt()) tot_lbds -= c.lbd();
}

// added by nabesima
void Solver::attachAllClauses() {
    if (!rand_attach) {
        for (int i=0; i < clauses.size(); i++)
            attachClause(clauses[i]);
        for (int i=0; i < learnts.size(); i++)
            attachClause(learnts[i]);
    }
    else {
        int i = 0, j = 0;
        while (i < clauses.size() && j < learnts.size()) {
            if (ca[clauses[i]].size() < ca[learnts[j]].size())
                attachClause(clauses[i++]);
            else
                attachClause(learnts[j++]);
        }
        for (; i < clauses.size(); i++)
            attachClause(clauses[i]);
        for (; j < learnts.size(); j++)
            attachClause(learnts[j]);
    }
}
void Solver::detachAllClauses() {
    for (int i=0; i < nVars(); i++) {
        watches[mkLit(i, false)].clear();
        watches[mkLit(i, true )].clear();
    }
    learnts_literals = 0;
    clauses_literals = 0;
    tot_lbds = 0;
}

void Solver::removeClause(CRef cr) {
    Clause& c = ca[cr];

    detachClause(cr);
    // Don't leave pointers to free'd memory!
    if (locked(c)) vardata[var(c[0])].reason = CRef_Undef;
    c.mark(1);

    ca.free(cr);
}

// added by nabesima
void Solver::removeClauseNoDetach(CRef cr) {
    Clause& c = ca[cr];
    // Don't leave pointers to free'd memory!
    if (locked(c)) vardata[var(c[0])].reason = CRef_Undef;
    c.mark(1);
    ca.free(cr);
}

bool Solver::satisfied(const Clause& c) const {
    for (int i = 0; i < c.size(); i++)
        if (value(c[i]) == l_True)
            return true;
    return false; }


// Revert to the state at given level (keeping all assignment at 'level' but not beyond).
//
void Solver::cancelUntil(int level) {
    if (decisionLevel() > level){
        for (int c = trail.size()-1; c >= trail_lim[level]; c--){
            Var      x  = var(trail[c]);
            assigns [x] = l_Undef;
            if (phase_saving > 1 || (phase_saving == 1 && c > trail_lim.last()))   // modified by nabesima
                polarity[x] = sign(trail[c]);
            insertVarOrder(x); }
        qhead = trail_lim[level];
        trail.shrink(trail.size() - trail_lim[level]);
        trail_lim.shrink(trail_lim.size() - level);
    } }


//=================================================================================================
// Major methods:


Lit Solver::pickBranchLit()
{
    Var next = var_Undef;

    // Random decision:
    if (drand(random_seed) < random_var_freq && !order_heap.empty()){
        next = order_heap[irand(random_seed,order_heap.size())];
        if (value(next) == l_Undef && decision[next])
            rnd_decisions++; }

    // Activity based decision:
    while (next == var_Undef || value(next) != l_Undef || !decision[next])
        if (order_heap.empty()){
            next = var_Undef;
            break;
        }else
            next = order_heap.removeMin();

    return next == var_Undef ? lit_Undef : mkLit(next, rnd_pol ? drand(random_seed) < 0.5 : polarity[next]);
}


/*_________________________________________________________________________________________________
|
|  analyze : (confl : Clause*) (out_learnt : vec<Lit>&) (out_btlevel : int&)  ->  [void]
|
|  Description:
|    Analyze conflict and produce a reason clause.
|
|    Pre-conditions:
|      * 'out_learnt' is assumed to be cleared.
|      * Current decision level must be greater than root level.
|
|    Post-conditions:
|      * 'out_learnt[0]' is the asserting literal at level 'out_btlevel'.
|      * If out_learnt.size() > 1 then 'out_learnt[1]' has the greatest decision level of the
|        rest of literals. There may be others from the same level though.
|
|________________________________________________________________________________________________@*/
// modified by nabesima
//void Solver::analyze(CRef confl, vec<Lit>& out_learnt, int& out_btlevel)
void Solver::analyze(CRef confl, vec<Lit>& out_learnt, int& out_btlevel, int& lbd)
{
    int pathC = 0;
    Lit p     = lit_Undef;

    // Generate conflict clause:
    //
    out_learnt.push();      // (leave room for the asserting literal)
    int index = trail.size() - 1;

    // added by nabesima
    implied_by_learnts.clear();
    int num_seen = 0;

    do{
        assert(confl != CRef_Undef); // (otherwise should be UIP)

        Clause& c = ca[confl];
        if (c.learnt())
            claBumpActivity(c);

        // added by nabesima
        bool all_skipped = true;

        int remains = num_seen;
        for (int j = (p == lit_Undef) ? 0 : 1; j < c.size(); j++){
            Lit q = c[j];

            // added by nabesima
            if (level(var(q)) == 0) continue;

            // added by nabesima
            if (lazy_lrn_skip && j > 0 && (premise_lit[toInt(~q)] == ~c[0] || premise_lit[toInt(c[0])] == q)) {
                if (lazy_lrn_skip == 2 && (!c.learnt() || c.lbd() <= 3))
                    lazy_clause_ops.push(COP(COP_LAZY_SHRINK, confl, q, c.size()));
                lazy_skipped_lits++;
                continue;
            }

            // added by nabesima
            all_skipped = false;

            // modified by nabesima
            //if (!seen[var(q)] && level(var(q)) > 0){
            if (!seen[var(q)]) {
                varBumpActivity(var(q));
                seen[var(q)] = 1;
                num_seen++;       // added by nabesima
                if (level(var(q)) >= decisionLevel()) {
                    pathC++;
                    // added by nabesima
                    if(lbd_act_bumping && reason(var(q)) != CRef_Undef && ca[reason(var(q))].learnt())
                        implied_by_learnts.push(q);
                }
                else
                    out_learnt.push(q);
            }
            else
                remains--;
        }

        // added by nabesima
        if (lazy_simp && all_skipped)
            lazy_fixed_lits.push(PrbLit(c[0], PRB_SELF_SUBSUMP));

        if (self_subsump && p != lit_Undef && /* (!c.learnt() || c.lbd() <= 3) && */ remains == 0) {
            if (c.size() == 2) {
                lazy_fixed_lits.push(PrbLit(c[1], PRB_SELF_SUBSUMP));
                c.learnt() ? ss_rm_lrn_lits++ : ss_rm_cla_lits++;
            }
            else {
                //if (implied_by_learnts.size() > 0 && var(implied_by_learnts.last()) == var(c[0]))
                //    implied_by_learnts.pop();
                //lazy_clause_ops.push(COP(COP_SELF_SUBSUMP, confl, c[0], c.size()));
                detachClause(confl, true);
                for (int k=1; k < c.size(); k++)
                    c[k-1] = c[k];
                c.shrink(1);
                attachClause(confl);
                c.learnt() ? ss_rm_lrn_lits++ : ss_rm_cla_lits++;
            }
        }

        // Select next clause to look at:
        while (!seen[var(trail[index--])]);
        p     = trail[index+1];
        confl = reason(var(p));
        seen[var(p)] = 0;
        pathC--;
        num_seen--;    // added by nabesima

    }while (pathC > 0);
    out_learnt[0] = ~p;

    // Simplify conflict clause:
    //
    int i, j;
    out_learnt.copyTo(analyze_toclear);
    if (ccmin_mode == 2){
        uint32_t abstract_level = 0;
        for (i = 1; i < out_learnt.size(); i++)
            abstract_level |= abstractLevel(var(out_learnt[i])); // (maintain an abstraction of levels involved in conflict)

        for (i = j = 1; i < out_learnt.size(); i++)
            if (reason(var(out_learnt[i])) == CRef_Undef || !litRedundant(out_learnt[i], abstract_level))
                out_learnt[j++] = out_learnt[i];

    }else if (ccmin_mode == 1){
        for (i = j = 1; i < out_learnt.size(); i++){
            Var x = var(out_learnt[i]);

            if (reason(x) == CRef_Undef)
                out_learnt[j++] = out_learnt[i];
            else{
                Clause& c = ca[reason(var(out_learnt[i]))];
                for (int k = 1; k < c.size(); k++)
                    if (!seen[var(c[k])] && level(var(c[k])) > 0){
                        out_learnt[j++] = out_learnt[i];
                        break; }
            }
        }
    }else
        i = j = out_learnt.size();

    max_literals += out_learnt.size();
    out_learnt.shrink(i - j);
    tot_literals += out_learnt.size();

    // added by nabesima
    if (lazy_bin_shrink) {
        int i, j;
        for (i = j = 1; i < out_learnt.size(); i++) {
            p = out_learnt[i];
            if (premise_lit[toInt(~p)] == ~out_learnt[0] || premise_lit[toInt(out_learnt[0])] == p)
                continue;
            out_learnt[j++] = p;
        }
        out_learnt.shrink(i - j);
        lazy_rm_lrn_lits += i - j;
    }

    // Find correct backtrack level:
    //
    if (out_learnt.size() == 1)
        out_btlevel = 0;
    else{

        int max_i = 1;
        // Find the first literal assigned at the next-highest level:
        for (int i = 2; i < out_learnt.size(); i++)
            if (level(var(out_learnt[i])) > level(var(out_learnt[max_i])))
                max_i = i;
        // Swap-in this literal at index 1:
        Lit p             = out_learnt[max_i];
        out_learnt[max_i] = out_learnt[1];
        out_learnt[1]     = p;
        out_btlevel       = level(var(p));

        // added by nabesima
        if (!rup) {
            Lit new_pre = premise_lit[toInt(~out_learnt[1])];
            for (int i = 2; i < out_learnt.size(); i++) {
                if (!(~out_learnt[i] == new_pre || premise_lit[toInt(~out_learnt[i])] == new_pre ||premise_lit[toInt(~new_pre)] == out_learnt[i])) {
                    new_pre = lit_Undef;
                    break;
                }
            }

            if (new_pre != lit_Undef) {
                premise_lit[toInt(out_learnt[0])] = new_pre;  // enqueued literal is invoked by the decision variable q.
                premise_updates++;
            }
        }
    }

    // added by nabesima
    {
        lbd = 0;
        lbd_comps++;
        for (int i=0; i < out_learnt.size(); i++) {
            int lv = level(var(out_learnt[i]));
            if (lbd_time[lv] != lbd_comps) {
                lbd_time[lv] = lbd_comps;
                lbd++;
            }
        }

        if (lbd_act_bumping && implied_by_learnts.size() > 0) {
            for(int i=0; i < implied_by_learnts.size(); i++) {
                if (ca[reason(var(implied_by_learnts[i]))].lbd() < lbd)
                    varBumpActivity(var(implied_by_learnts[i]));
            }
            implied_by_learnts.clear();
        }
    }

    for (int j = 0; j < analyze_toclear.size(); j++) seen[var(analyze_toclear[j])] = 0;    // ('seen[]' is now cleared)
}


// Check if 'p' can be removed. 'abstract_levels' is used to abort early if the algorithm is
// visiting literals at levels that cannot be removed later.
bool Solver::litRedundant(Lit p, uint32_t abstract_levels)
{
    analyze_stack.clear(); analyze_stack.push(p);
    int top = analyze_toclear.size();
    while (analyze_stack.size() > 0){
        assert(reason(var(analyze_stack.last())) != CRef_Undef);
        Clause& c = ca[reason(var(analyze_stack.last()))]; analyze_stack.pop();

        for (int i = 1; i < c.size(); i++){
            Lit p  = c[i];
            if (!seen[var(p)] && level(var(p)) > 0){
                if (reason(var(p)) != CRef_Undef && (abstractLevel(var(p)) & abstract_levels) != 0){
                    seen[var(p)] = 1;
                    analyze_stack.push(p);
                    analyze_toclear.push(p);
                }else{
                    for (int j = top; j < analyze_toclear.size(); j++)
                        seen[var(analyze_toclear[j])] = 0;
                    analyze_toclear.shrink(analyze_toclear.size() - top);
                    return false;
                }
            }
        }
    }

    return true;
}


/*_________________________________________________________________________________________________
|
|  analyzeFinal : (p : Lit)  ->  [void]
|
|  Description:
|    Specialized analysis procedure to express the final conflict in terms of assumptions.
|    Calculates the (possibly empty) set of assumptions that led to the assignment of 'p', and
|    stores the result in 'out_conflict'.
|________________________________________________________________________________________________@*/
void Solver::analyzeFinal(Lit p, vec<Lit>& out_conflict)
{
    out_conflict.clear();
    out_conflict.push(p);

    if (decisionLevel() == 0)
        return;

    seen[var(p)] = 1;

    for (int i = trail.size()-1; i >= trail_lim[0]; i--){
        Var x = var(trail[i]);
        if (seen[x]){
            if (reason(x) == CRef_Undef){
                assert(level(x) > 0);
                out_conflict.push(~trail[i]);
            }else{
                Clause& c = ca[reason(x)];
                for (int j = 1; j < c.size(); j++)
                    if (level(var(c[j])) > 0)
                        seen[var(c[j])] = 1;
            }
            seen[x] = 0;
        }
    }

    seen[var(p)] = 0;
}

void Solver::uncheckedEnqueue(Lit p, CRef from) {
    assert(value(p) == l_Undef);
    assigns[var(p)] = lbool(!sign(p));
    vardata[var(p)] = mkVarData(from, decisionLevel());
    trail.push_(p);
}

/*_________________________________________________________________________________________________
|
|  propagate : [void]  ->  [Clause*]
|
|  Description:
|    Propagates all enqueued facts. If a conflict arises, the conflicting clause is returned,
|    otherwise CRef_Undef.
|
|    Post-conditions:
|      * the propagation queue is empty, even if there was a conflict.
|________________________________________________________________________________________________@*/
CRef Solver::propagate()
{
    CRef     confl        = CRef_Undef;
    uint     num_props    = 0;             // modified by nabesima
    uint     ind_props    = 0;             // added by nabesima
    Lit      dec_lit      = decisionLevel() > 0 ? trail[trail_lim.last()] : lit_Undef;    // added by nabesima
    Lit      old_pre      = lit_Undef;
    Lit      opp_pre      = lit_Undef;
    Lit      new_pre      = lit_Undef;

    watches.cleanAll();

    while (qhead < trail.size()){
        Lit           p  = trail[qhead++];     // 'p' is enqueued fact to propagate.
        vec<Watcher>& ws = watches[p];
        Watcher       *i, *j, *end;
        num_props++;
        if (value(premise_lit[toInt(p)]) == l_True && level(var(premise_lit[toInt(p)])) == decisionLevel()) ind_props++;    // added by nabesima

        for (i = j = (Watcher*)ws, end = i + ws.size();  i != end;){
            // Try to avoid inspecting the clause:
            Lit blocker = i->blocker;
            if (value(blocker) == l_True){
                *j++ = *i++; continue; }

            // Make sure the false literal is data[1]:
            CRef     cr        = i->cref;
            Clause&  c         = ca[cr];
            Lit      false_lit = ~p;
            if (c[0] == false_lit)
                c[0] = c[1], c[1] = false_lit;
            assert(c[1] == false_lit);
            i++;

            // If 0th watch is true, then clause is already satisfied.
            Lit     first = c[0];
            Watcher w     = Watcher(cr, first);
            if (first != blocker && value(first) == l_True){
                *j++ = w; continue; }

            // Look for new watch:
            // modified by nabesima
            //for (int k = 2; k < c.size(); k++)
            //    if (value(c[k]) != l_False){
            //        c[1] = c[k]; c[k] = false_lit;
            //        watches[~c[1]].push(w);
            //        goto NextClause; }
            Lit premise = lit_Undef;
            if (!rup) {
                if (premise_src == PSRC_DEC_LIT) {
                    if (~false_lit == dec_lit || premise_lit[toInt(~false_lit)] == dec_lit || (dec_lit != lit_Undef && premise_lit[toInt(~dec_lit)] == false_lit))
                        premise = dec_lit;
                }
                else if (premise_src == PSRC_LAST_UIP) {
                    premise = (~false_lit == dec_lit) ? dec_lit : premise_lit[toInt(~false_lit)];
                    if (level(var(premise)) != decisionLevel())
                        premise = ~false_lit;
                }
                else {
                    if (premise_lit[toInt(first)] != dec_lit) {
                        if (~false_lit == dec_lit || premise_lit[toInt(~false_lit)] == dec_lit || (dec_lit != lit_Undef && premise_lit[toInt(~dec_lit)] == false_lit))
                            premise = dec_lit;
                    }
                    else {
                        premise = (~false_lit == dec_lit) ? dec_lit : premise_lit[toInt(~false_lit)];
                        if (level(var(premise)) != decisionLevel())
                            premise = ~false_lit;
                    }
                }
            }
            else {
                if (~false_lit == dec_lit)
                    premise = dec_lit;
            }
            for (int k = 2; k < c.size(); ) {
                Lit ck = c[k];
                if (value(ck) != l_False) {
                    // binary self-subsumption checking
                    if (lazy_bin_shrink &&
                            (premise_lit[toInt(~ck)] == ~false_lit || premise_lit[toInt(false_lit)] == ck ||
                             premise_lit[toInt(~ck)] == ~first     || premise_lit[toInt(first    )] == ck )) {
                        if (rup) c.copyTo(rup_tmp);
                        c[k] = c.last();
                        c.shrink(1);
                        c.learnt() ? lazy_rm_lrn_lits++ : lazy_rm_cla_lits++;
                        if (rup) {
                            writeClause(c);
                            writeDeletedLits(rup_tmp);
                        }
                        continue;
                    }
                    c[1] = ck; c[k] = false_lit;
                    watches[~c[1]].push(w);
                    goto NextClause;
                }
                if (premise != lit_Undef && level(var(ck)) != 0 && premise_lit[toInt(~ck)] != premise && (rup || premise == lit_Undef || premise_lit[toInt(~premise)] != ck))
                    premise = lit_Undef;
                k++;
            }
            *j++ = w;

            // Did not find watch -- clause is unit under assignment:
            if (value(first) == l_False) {
                confl = cr;
                qhead = trail.size();
                // Copy the remaining watches:
                while (i < end)
                    *j++ = *i++;
            } else {
                // modified by nabesima
                uncheckedEnqueue(first, cr);

                if (lazy_simp && premise != lit_Undef && decisionLevel() > 0) {
                    old_pre = premise_lit[toInt(first)];
                    opp_pre = premise_lit[toInt(~first)];
                    new_pre = premise;
                    if (lazy_checking == LCHK_ALWAYS || old_pre != new_pre) {

                        if (rup) {
                            LazyHash& hash = premise_hash[toInt(first)];
                            if (!hash.contains(new_pre)) {
                                writeLits(first, ~new_pre);
                                hash.put(new_pre);
                            }
                        }

                        if (lazy_fls_probing && (opp_pre == new_pre || opp_pre == old_pre)) {
                            lazy_fixed_lits.push(PrbLit(~opp_pre, PRB_FALSE_LIT));
                        }
                        if (lazy_pol_probing && old_pre == ~new_pre) {
                            lazy_fixed_lits.push(PrbLit(first, PRB_POLALITY));
                        }
                        else if (lazy_bin_probing && (premise_lit[toInt(new_pre)] == ~old_pre || premise_lit[toInt(old_pre)] == ~new_pre)) {
                            lazy_fixed_lits.push(PrbLit(first, PRB_BIN_CLAUSE));
                        }

                        premise_lit[toInt(first)] = new_pre;
                        premise_updates++;

                        if (lazy_eqv_probing && opp_pre != ~first) {
                            if (~new_pre == opp_pre)
                                putEquivLit(first, new_pre);
                            if (~old_pre == opp_pre)
                                putEquivLit(first, old_pre);
                        }
                    }
                }
            }

            // added by nabesima
            if (c.learnt() && c.lbd() > 2) {
                lbd_comps++;
                int lbd = 0;
                if (learnts_measure <= MS_LBD) {  // glucose version
                    for (int i=0; i < c.size(); i++) {
                        int lv = level(var(c[i]));
                        if (lbd_time[lv] != lbd_comps) {
                            lbd_time[lv] = lbd_comps;
                            lbd++;
                        }
                    }
                }
                else if (learnts_measure == MS_STRICT_LBD){
                    int  unit_blocks = 0;
                    uint first_time  = lbd_comps;
                    uint second_time = ++lbd_comps;
                    for (int i=0; i < c.size(); i++) {
                        int lv = level(var(c[i]));
                        if (lbd_time[lv] < first_time) {  // first time
                            lbd_time[lv] = first_time;
                            lbd++;
                            unit_blocks++;
                            if (lbd >= c.lbd()) break;
                        }
                        else if (lbd_time[lv] == first_time) {  // second time
                            unit_blocks--;
                            lbd_time[lv] = second_time;
                        }
                    }
                    if (unit_blocks == 0)
                        lbd = c.lbd();
                }
                else if (learnts_measure == MS_PSEUDO_LBD){
                    lbd = 1 + (rm_fixed_vars ? 1 : 0);
                    for (int i=0; i < c.size(); i++) {
                        int lv = level(var(c[i]));
                        if (lbd_time[lv] != lbd_comps && (lv > 0 || !rm_fixed_vars)) {
                            lbd_time[lv] = lbd_comps;
                            lbd++;
                            if (lbd >= c.lbd()) break;
                        }
                    }
                }
                else
                    assert(false);

                // Update?
                if (lbd < c.lbd()) {   // The difference to the glucose code.
                    tot_lbds = tot_lbds - c.lbd() + lbd;
                    //wholeLBDs = wholeLBDs - c.lbd() + lbd;
                    c.lbd(lbd);
                    lbd_updates++;
                    if (lbd <= 2) glue_clauses++;
                    if (delayed_restart)
                        recentLBDs.push(lbd);    // glueminisat 2.2.6's heuristics
                }
            }

        NextClause:;
        }
        ws.shrink(i - j);
    }
    propagations += num_props;
    simpDB_props -= num_props;
    ind_propagations += ind_props;    // added by nabesima

    return confl;
}
/*_________________________________________________________________________________________________
|
|  reduceDB : ()  ->  [void]
|
|  Description:
|    Remove half of the learnt clauses, minus the clauses locked by the current assignment. Locked
|    clauses are clauses that are reason to some assignment. Binary clauses are never removed.
|________________________________________________________________________________________________@*/
struct reduceDB_lt {
    ClauseAllocator& ca;
    reduceDB_lt(ClauseAllocator& ca_) : ca(ca_) {}
    bool operator () (CRef x, CRef y) {
        return ca[x].size() > 2 && (ca[y].size() == 2 || ca[x].activity() < ca[y].activity()); }
};

// added by nabesima
struct reduceDB_lt_with_lbd {
    ClauseAllocator& ca;
    bool rm_dup_bins;
    reduceDB_lt_with_lbd(ClauseAllocator& ca_, bool rm_dup_bins_) : ca(ca_), rm_dup_bins(rm_dup_bins_) {}
    bool operator () (CRef x, CRef y) {
        // First criteria
        if (ca[x].size() >  2 && ca[y].size() == 2) return true;
        if (ca[y].size() >  2 && ca[x].size() == 2) return false;
        if (ca[x].size() == 2 && ca[y].size() == 2) {
            if (rm_dup_bins) {
                // For removing duplicated binary clauses.
                Lit xmin = (ca[x][0] < ca[x][1]) ? ca[x][0] : ca[x][1];
                Lit ymin = (ca[y][0] < ca[y][1]) ? ca[y][0] : ca[y][1];
                return xmin < ymin;
            }
            else
                return false;
        }

        // Second one
        if (ca[x].lbd() > ca[y].lbd()) return true;
        if (ca[x].lbd() < ca[y].lbd()) return false;

        return ca[x].size() > ca[y].size();   // The difference to the glucose code.
    }
};

// added by nabesima
struct reduceDB_lt_with_lbd_lru {
    ClauseAllocator& ca;
    int max_lbd;
    reduceDB_lt_with_lbd_lru(ClauseAllocator& ca_, int max_lbd_) : ca(ca_), max_lbd(max_lbd_) {}
    bool operator () (CRef x, CRef y) {
        if (ca[x].lbd() <= max_lbd) {
            if (ca[y].lbd() <= max_lbd)
                return ca[x].lbd() > ca[y].lbd();
            return false;
        }
        if (ca[y].lbd() <= max_lbd)
            return true;
        return ca[x].activity() < ca[y].activity();
    }
};

// modified by nabesima
//void Solver::reduceDB()
bool Solver::reduceDB()
{
    // added by nabesima
    if (verbosity >= 1) printLog();
    // added by nabesima
    if (lazy_clause_ops.size() > 0) {
        lazy_lost_cops += lazy_clause_ops.size();
        lazy_clause_ops.clear();
    }

    int     i = 0, j = 0;
    double  extra_lim = cla_inc / learnts.size();    // Remove any clause below this activity

    // added by nabesima
    reduce_dbs++;
    int org_learnts = nLearnts();
    uint num_bin = 0;
    uint num_glue = 0;
    uint num_glue3 = 0;

    // modified by nabesima
    if (reduce_db == RD_LRU) {  // minisat version
        sort(learnts, reduceDB_lt(ca));
        // Don't delete binary or locked clauses. From the rest, delete clauses from the first half
        // and clauses with activity smaller than 'extra_lim':
        for (i = j = 0; i < learnts.size(); i++){
            Clause& c = ca[learnts[i]];

            // added by nabesima
            if (c.size() == 2) num_bin++;
            if (c.lbd()  == 2) num_glue++;
            if (c.lbd()  == 3) num_glue3++;

            if (c.size() > max_len && !locked(c) && (i < learnts.size() / 2 || c.activity() < extra_lim)) {
                removeClause(learnts[i]);
                if (rup) writeDeletedClause(c);
            }
            else
                learnts[j++] = learnts[i];
        }
    }
    else if (reduce_db == RD_LBD){  // glucose version
        sort(learnts, reduceDB_lt_with_lbd(ca, rm_dup_bins));
        for (i = j = 0; i < learnts.size(); i++) {
            Clause& c = ca[learnts[i]];

            // added by nabesima
            if (c.size() == 2) num_bin++;
            if (c.lbd()  == 2) num_glue++;
            if (c.lbd()  == 3) num_glue3++;

            assert(c.lbd() > 0);

            if (c.size() > max_len && !locked(c) && i < learnts.size() * max_rate_learnts && c.lbd() > max_lbd) {
                removeClause(learnts[i]);
                if (rup) writeDeletedClause(c);
                continue;
            }

            // Duplicated binary clause checking
            if (rm_dup_bins && !locked(c) && j > 0 && c.size() == 2 && ca[learnts[j-1]].size() == 2) {
                Clause& d = ca[learnts[j-1]];
                if ((c[0] == d[0] || c[0] == d[1]) && (c[1] == d[0] || c[1] == d[1])) {
                    removeClause(learnts[i]);
                    if (rup) writeDeletedClause(c);
                    dup_bin_learnts++;
                    continue;
                }
            }

            learnts[j++] = learnts[i];
        }
    }
    else if (reduce_db == RD_LBD_LRU) {

        sort(learnts, reduceDB_lt_with_lbd_lru(ca, max_lbd));

        int org_size = learnts.size();
        for (i = j = 0; i < org_size; i++) {

            Clause& c = ca[learnts[i]];
            assert(c.lbd() > 0);

            // added by nabesima
            if (c.size() == 2) num_bin++;
            if (c.lbd()  == 2) num_glue++;
            if (c.lbd()  == 3) num_glue3++;

            if (locked(c))
                learnts[j++] = learnts[i];
            else if (c.lbd() <= max_lbd || c.size() <= max_len)
                learnts[j++] = learnts[i];
            else if ((j + org_size - i) < (org_size * min_rate_learnts))    // (org_size - i) means the number of remained learnts
                learnts[j++] = learnts[i];
            else if (c.activity() < extra_lim) {
                removeClause(learnts[i]);
                if (rup) writeDeletedClause(c);
            }
            else
                learnts[j++] = learnts[i];
        }
        if (j > org_size * max_rate_learnts) {
            learnts.shrink(i - j);
            for (i = j = 0; i < learnts.size(); i++) {
                Clause& c = ca[learnts[i]];
                if (!locked(c) && i < learnts.size() - org_size / 2) {
                    removeClause(learnts[i]);
                    if (rup) writeDeletedClause(c);
                }
                else
                    learnts[j++] = learnts[i];
            }
        }
    }
    else
        assert(false);

    learnts.shrink(i - j);

    checkGarbage();

    // added by nabesima
    if (verbosity >= 1) {
        printf("< RDB %-5d (%6"PRIu64" res,%5.0f confs/res, %5.1f%%, %7db %8dg %8dg3) >\n",
            reduce_dbs, starts, (double)conflicts / starts, (double)nLearnts() / org_learnts * 100.0,
            num_bin, num_glue, num_glue3);
        printLog();
    }

    return true;
}

void Solver::removeSatisfied(vec<CRef>& cs)
{
    if (!rm_fixed_vars) {
        int i, j;
        for (i = j = 0; i < cs.size(); i++){
            Clause& c = ca[cs[i]];
            if (satisfied(c)) {
                removeClause(cs[i]);
                if (rup) writeDeletedClause(c);
            }
            else
                cs[j++] = cs[i];
        }
        cs.shrink(i - j);
    }
    // added by nabesima
    else {
        assert(decisionLevel() == 0);
        vec<Lit> ps;
        int i, j;
        for (i = j = 0; i < cs.size(); i++){
            Clause& c = ca[cs[i]];
            if (rup)
                ps.clear();
            int  k, l;
            for (k = l = 0; k < c.size(); k++) {
                Lit from = c[k];
                Lit to   = from;
                if (rup)
                    ps.push(from);
                if (value(from) == l_True)
                    break;
                if (k >= 2) {
                    if (value(from) == l_False) {
                        removed_fixed_vars++;
                        continue;
                    }
                    if (lazy_bin_shrink &&
                            (premise_lit[toInt(~from)] == ~c[0] || premise_lit[toInt(c[0])] == from ||
                             premise_lit[toInt(~from)] == ~c[1] || premise_lit[toInt(c[1])] == from)) {
                        c.learnt() ? lazy_rm_lrn_lits++ : lazy_rm_cla_lits++;
                        continue;
                    }
                    if (lazy_bin_subsump) {
                        if (premise_lit[toInt(from)] == ~c[0] || premise_lit[toInt(c[0])] == ~from) {
                            lazy_clause_ops.push(COP(COP_MAKE_BIN, cs[i], c[0], from));
                            while (k < c.size())
                                c[l++] = c[k++];
                            break;
                        }
                        else if (premise_lit[toInt(from)] == ~c[1] || premise_lit[toInt(c[1])] == ~from) {
                            lazy_clause_ops.push(COP(COP_MAKE_BIN, cs[i], c[1], from));
                            while (k < c.size())
                                c[l++] = c[k++];
                            break;
                        }
                    }
                }
                c[l++] = to;
            }
            if (k < c.size()) {
                if (rup) {
                    while (++k < c.size())
                        ps.push(c[k]);
                    writeDeletedLits(ps);
                }
                removeClause(cs[i]);
            }
            else {
                c.shrink(k - l);
                cs[j++] = cs[i];
                if (rup && k > l) {
                    writeClause(c);
                    writeDeletedLits(ps);
                }
            }
        }
        cs.shrink(i - j);
    }
}

// added by nabesima
bool Solver::rewriteClauses()
{
    assert(decisionLevel() == 0);
    assert(lazy_fixed_lits.size() == 0);

    if (num_equivalent_lits == rewrited_lits)
        return true;

    if (lazy_clause_ops.size() > 0) {
        lazy_lost_cops += lazy_clause_ops.size();
        lazy_clause_ops.clear();
    }

    rewrite_dbs++;

    // Find minimum elements.
    for (int i=0; i < equivalent_lit.size(); i++) {
        if (equivalent_lit[i] == lit_Undef) continue;
        Lit p = equivalent_lit[i];
        while (equivalent_lit[var(p)] != lit_Undef)
            p = sign(p) ? ~equivalent_lit[var(p)] : equivalent_lit[var(p)];
//        if (rup && equivalent_lit[i] != p) {
//            Lit pp = mkLit(i, false);
//            writeLits(~pp,  equivalent_lit[var(pp)]);
//            writeLits( pp, ~equivalent_lit[var(pp)]);
//        }
        equivalent_lit[i] = p;
        setDecisionVar(i, false);
    }

    uint rw_lits    = 0;
    uint taut_clas  = 0;
    uint fixed_vars = 0;
    detachAllClauses();
    rewriteClauses(clauses, rw_lits, taut_clas, fixed_vars);
    rewriteClauses(learnts, rw_lits, taut_clas, fixed_vars);
    attachAllClauses();

    // The following code is same as the last of simplify().
    checkGarbage();
    rebuildOrderHeap();

    simpDB_assigns = nAssigns();
    simpDB_props   = clauses_literals + learnts_literals;   // (shouldn't depend on stats really, but it will do for now)

    if (verbosity >= 1) {
        printf("[ RWT %-4d (%5d + %7d eq-vars, %7d rw-lits, %6d taut, %5d fixed) ]\n",
                rewrite_dbs, num_equivalent_lits - rewrited_lits, rewrited_lits, rw_lits, taut_clas, fixed_vars);
        fflush(stdout);
    }

    rewrited_lits = num_equivalent_lits;

    return true;
}

bool Solver::rewriteClauses(vec<CRef>& cs, uint& rw_lits, uint& taut_clas, uint& fixed_vars)
{
    // for checking duplicated literals.
    uint time = 0;
    vec<uint> occ(nVars());

    int i, j;
    for (i = j = 0; i < cs.size(); i++) {
        Clause& c = ca[cs[i]];

        time += 2;
        if (time <= 1) {    // to avoid miss detection of duplicated literals or tautological clauses.
            time = 2;
            for (int k=0; k < occ.size(); k++)
                occ[k] = 0;
        }

        if (rup) rup_tmp.clear();

        bool replaced = false;
        int  k, l;
        for (k = l = 0; k < c.size(); k++) {
            Lit from = c[k];
            Lit to   = from;
            if (rup) rup_tmp.push(from);
            if (value(from) == l_True)
                break;
            if (rm_fixed_vars && value(from) == l_False) {
                removed_fixed_vars++;
                continue;
            }

            if (equivalent_lit.size() > 0 && equivalent_lit[var(from)] != lit_Undef) {
                to = sign(from) ? ~equivalent_lit[var(from)] : equivalent_lit[var(from)];
                rw_lits++;
            }
            else if (lazy_bin_shrink && l >= 2 &&
                    (premise_lit[toInt(~from)] == ~c[0] || premise_lit[toInt(c[0])] == from ||
                     premise_lit[toInt(~from)] == ~c[1] || premise_lit[toInt(c[1])] == from )) {
                c.learnt() ? lazy_rm_lrn_lits++ : lazy_rm_cla_lits++;
                continue;
            }

            // duplicated literals.
            if (occ[var(to)] == time + sign( to)) {
                c.learnt() ? lazy_rm_lrn_lits++ : lazy_rm_cla_lits++;
                continue;
            }
            // tautological clause.
            if (occ[var(to)] == time + sign(~to)) {
                taut_clas++;
                break;
            }

            if (from != to) replaced = true;

            c[l++] = to;
            occ[var(to)] = time + sign(to);
        }

        if (k < c.size()) {
            if (rup) {
                while (++k < c.size())
                    rup_tmp.push(c[k]);
                writeDeletedLits(rup_tmp);
            }
            removeClauseNoDetach(cs[i]);
        }
        else {
            c.shrink(k - l);
            if (rup && (k > l || replaced)) {
                writeClause(c);
                writeDeletedLits(rup_tmp);
            }
            if (c.size() > 1)
                cs[j++] = cs[i];
            else if (c.size() == 0)
                ok = false;
            else {
                assert(c.size() == 1);
                if (value(c[0]) == l_False) {
                    return false;
                }
                if (value(c[0]) == l_Undef) {
                    uncheckedEnqueue(c[0]);
                    fixed_vars++;
                }
                removeClauseNoDetach(cs[i]);
            }
        }
    }
    cs.shrink(i - j);

    return true;
}

void Solver::rebuildOrderHeap()
{
    vec<Var> vs;
    for (Var v = 0; v < nVars(); v++)
        if (decision[v] && value(v) == l_Undef)
            vs.push(v);
    order_heap.build(vs);
}


/*_________________________________________________________________________________________________
|
|  simplify : [void]  ->  [bool]
|
|  Description:
|    Simplify the clause database according to the current top-level assigment. Currently, the only
|    thing done here is the removal of satisfied clauses, but more things can be put here.
|________________________________________________________________________________________________@*/
bool Solver::simplify()
{
    assert(decisionLevel() == 0);

    if (!ok || propagate() != CRef_Undef)
        return ok = false;

    // added by nabesima
    if (lazy_clause_ops.size() > 0) {
        lazy_lost_cops += lazy_clause_ops.size();
        lazy_clause_ops.clear();
    }

    if (nAssigns() == simpDB_assigns || (simpDB_props > 0))
        return true;

    // added by nabesima
    if (lazy_simp && rewrited_lits + rewrite_min_lits <= num_equivalent_lits)
        return rewriteClauses();

    // added by nabesima
    simplify_dbs++;

    // Remove satisfied clauses:
    removeSatisfied(learnts);
    if (remove_satisfied)        // Can be turned off.
        removeSatisfied(clauses);

    checkGarbage();
    rebuildOrderHeap();

    simpDB_assigns = nAssigns();
    simpDB_props   = clauses_literals + learnts_literals;   // (shouldn't depend on stats really, but it will do for now)

    return true;
}


/*_________________________________________________________________________________________________
|
|  search : (nof_conflicts : int) (params : const SearchParams&)  ->  [lbool]
|
|  Description:
|    Search for a model the specified number of conflicts.
|    NOTE! Use negative value for 'nof_conflicts' indicate infinity.
|
|  Output:
|    'l_True' if a partial assigment that is consistent with respect to the clauseset is found. If
|    all variables are decision variables, this means that the clause set is satisfiable. 'l_False'
|    if the clause set is unsatisfiable. 'l_Undef' if the bound on number of conflicts is reached.
|________________________________________________________________________________________________@*/
lbool Solver::search(int nof_conflicts)
{
    assert(ok);

    int         backtrack_level;
    int			lbd;                  // added by nabesima
    int         conflictC = 0;
    int         decisionC = 0;        // added by nabesima
    int64_t	    old_propC = 0;        // added by nabesima
    int64_t	    cur_propC = 0;        // added by nabesima
    vec<Lit>    learnt_clause;
    bool        restart;              // added by nabesima
    bool        lbd_restart;          // added by nabesima
    bool        dlv_restart;          // added by nabesima
    bool        dec_restart;          // added by nabesima
    starts++;

    // added by nabesima
    recentLBDs.clear();
    recentDLVs.clear();
    if (starts == 1) {
        switch (restart_strategy) {
        case RS_MINISAT:
            break;
        case RS_LBD_AVG:
        case RS_DLV_AVG:
        case RS_LBD_DLV:
        case RS_LBD_DEC:
            recentLBDs.init(lbd_restart_size);   // glucose1.0
            recentDLVs.init(dlv_restart_size);	 // glucose
            break;
        }
    }

    for (;;){

        old_propC = propagations;                // added by nabesima

        CRef confl = propagate();

        cur_propC += propagations - old_propC;    // added by nabesima

        if (confl != CRef_Undef){
            // CONFLICT
            conflicts++; conflictC++;
            if (decisionLevel() == 0) return l_False;

            learnt_clause.clear();
            // modified by nabesima
            //analyze(confl, learnt_clause, backtrack_level);
            analyze(confl, learnt_clause, backtrack_level, lbd);
            assert(lbd > 0);

            // added by nabesima
            wholeDLVs += decisionLevel();
            wholeLBDs += lbd;
            recentLBDs.push(lbd);
            recentDLVs.push(decisionLevel());

            if (learnts_measure == MS_PSEUDO_LBD) {
                if (lbd <= 3)
                    glue_clauses++;
            }
            else if (lbd <= 2)
                glue_clauses++;

            cancelUntil(backtrack_level);

            // added by nabesima
            if (rup) writeLits(learnt_clause);

            if (learnt_clause.size() == 1){
                uncheckedEnqueue(learnt_clause[0]);
            }else{
                CRef cr = ca.alloc(learnt_clause, true);
                learnts.push(cr);

                // added by nabesima
                assert(lbd > 0);
                ca[cr].lbd(lbd);
                attachClause(cr);
                claBumpActivity(ca[cr]);
                uncheckedEnqueue(learnt_clause[0], cr);
            }

            varDecayActivity();
            claDecayActivity();

            if (--learntsize_adjust_cnt == 0){
                learntsize_adjust_confl *= learntsize_adjust_inc;
                learntsize_adjust_cnt    = (int)learntsize_adjust_confl;
                max_learnts             *= learntsize_inc;

                // modified by nabesima
                if (verbosity >= 1) {
                    /*
                    printf("| %9d | %7d %8d %8d | %8d %8d %6.1f | %6.3f %% |\n",
                           (int)conflicts,
                           (int)dec_vars - (trail_lim.size() == 0 ? trail.size() : trail_lim[0]), nClauses(), (int)clauses_literals,
                           (int)max_learnts, nLearnts(), (double)learnts_literals/nLearnts(), progressEstimate()*100);
                    */
                    printLog();
                }
            }

        }else{
            // NO CONFLICT

            // modified by nabesima
            restart = lbd_restart = dlv_restart = dec_restart = false;
            switch (restart_strategy) {
            // Minisat version
            case RS_MINISAT: {
                if ((nof_conflicts >= 0 && conflictC >= nof_conflicts) || !withinBudget())
                    restart = true;
                break;
            }
            // Glucose version
            case RS_LBD_AVG: {
                if ((recentLBDs.ready() &&
                     recentLBDs.average() * lbd_restart_rate > wholeLBDs / conflicts) ||
                     !withinBudget())
                    restart = lbd_restart = true;
                break;
            }
            // Average of conflicting decision levels
            case RS_DLV_AVG: {
                if ((recentDLVs.ready() &&
                     recentDLVs.average() * dlv_restart_rate > wholeDLVs / conflicts) ||
                     !withinBudget())
                    restart = dlv_restart = true;
                break;
            }

            case RS_LBD_DLV: {
                if (!withinBudget())
                    restart = true;
                else if (recentLBDs.ready() && recentLBDs.average() * lbd_restart_rate > wholeLBDs / conflicts)
                    restart = lbd_restart = true;
                else if (recentDLVs.ready() && recentDLVs.average() * dlv_restart_rate > wholeDLVs / conflicts)
                    restart = dlv_restart = true;
                break;
            }

            case RS_LBD_DEC: {
                if (!withinBudget())
                    restart = true;
                else if (recentLBDs.ready() && recentLBDs.average() * lbd_restart_rate > wholeLBDs / conflicts)
                    restart = lbd_restart = true;
                else if (conflictC > dec_restart_size &&
                         (double)decisionC / conflictC * dec_restart_rate > (double)decisions / conflicts)
                    restart = dec_restart = true;
                break;
            }

            default:
                assert(false);
                /* no break */
            }

            if (restart && blocked_restart) {
                if ((double)cur_propC / conflictC * blocked_restart_rate > (double)propagations / conflicts) {
                    restart = false;
                    recentDLVs.wait(dlv_restart_size);
                    recentLBDs.wait(lbd_restart_size);
                    blocked_restarts++;
                }
            }

            if (restart) {
                progress_estimate = progressEstimate();
                cancelUntil(0);
                lbd_restarts += lbd_restart;
                dlv_restarts += dlv_restart;
                dec_restarts += dec_restart;
                return l_Undef;
            }

            // added by nabesima
            //if (decisionLevel() == 0) {
            if (lazy_fixed_lits.size() > 0) {
                cancelUntil(0);
                if (!lazySimplify())
                    return l_False;
            }

            // Simplify the set of problem clauses:
            if (decisionLevel() == 0 && !simplify())
                return l_False;

            // modified by nabesima
            if (!ag_reduce_db && learnts.size()-nAssigns() >= max_learnts && (!lv0_reduce_db || decisionLevel() == 0)) { // Minisat version
                // Reduce the set of learnt clauses:
                if (!reduceDB()) return l_False;
            }
            else if (ag_reduce_db && conflicts > reduce_db_limit && (!lv0_reduce_db || decisionLevel() == 0)) {  // gluecose version
                if (!reduceDB()) return l_False;
                reduce_db_limit += reduce_db_base + reduce_db_inc * reduce_dbs;
            }

            Lit next = lit_Undef;
            while (decisionLevel() < assumptions.size()){
                // Perform user provided assumption:
                Lit p = assumptions[decisionLevel()];
                if (value(p) == l_True){
                    // Dummy decision level:
                    newDecisionLevel();
                }else if (value(p) == l_False){
                    analyzeFinal(~p, conflict);
                    return l_False;
                }else{
                    next = p;
                    break;
                }
            }

            // modified by nabesima
            while (next == lit_Undef) {
            //if (next == lit_Undef){
                // New variable decision:
                decisions++;
                decisionC++;    // added by nabesima

                next = pickBranchLit();

                if (next == lit_Undef)
                    // Model found:
                    return l_True;
            }

            // Increase decision level and enqueue 'next'
            newDecisionLevel();
            // modified by nabesima
            //uncheckedEnqueue(next);
            uncheckedEnqueue(next);
        }
    }
    return l_Undef;  // added by nabesima for suppressing a warning from eclipse
}

double Solver::progressEstimate() const
{
    double  progress = 0;
    double  F = 1.0 / nVars();

    for (int i = 0; i <= decisionLevel(); i++){
        int beg = i == 0 ? 0 : trail_lim[i - 1];
        int end = i == decisionLevel() ? trail.size() : trail_lim[i];
        progress += pow(F, i) * (end - beg);
    }

    return progress / nVars();
}

/*
  Finite subsequences of the Luby-sequence:

  0: 1
  1: 1 1 2
  2: 1 1 2 1 1 2 4
  3: 1 1 2 1 1 2 4 1 1 2 1 1 2 4 8
  ...


 */

static double luby(double y, int x){

    // Find the finite subsequence that contains index 'x', and the
    // size of that subsequence:
    int size, seq;
    for (size = 1, seq = 0; size < x+1; seq++, size = 2*size+1);

    while (size-1 != x){
        size = (size-1)>>1;
        seq--;
        x = x % size;
    }

    return pow(y, seq);
}

// NOTE: assumptions passed in member-variable 'assumptions'.
lbool Solver::solve_()
{
    // added by nabesima
    if (init_vars == 0 && init_clauses == 0) {
        init_vars    = simp_vars    = nVars();
        init_clauses = simp_clauses = nClauses();
    }

    model.clear();
    conflict.clear();
    if (!ok) return l_False;

    solves++;

    if (init_rdb_param) {  // added by nabesima
        max_learnts               = nClauses() * learntsize_factor;
        learntsize_adjust_confl   = learntsize_adjust_start_confl;
        learntsize_adjust_cnt     = (int)learntsize_adjust_confl;
    }
    lbool   status            = l_Undef;

    // added by nabesima
    if (max_learnts < reduce_db_base && init_rdb_param) {
        reduce_db_base = reduce_db_limit = (uint64_t)((max_learnts/2 < 5000) ? 5000 : max_learnts/2);
        reduce_db_inc  /= 2;
    }
    core_clauses_literals = clauses_literals;

    // added by nabesima
    if (wl_sorting) {
        for (int v=0; v < nVars(); v++) {
            Lit p = mkLit(v, false);
            sort(watches [p], Watcher_lt(ca));
            sort(watches[~p], Watcher_lt(ca));
        }
    }

    if (verbosity >= 1){
        /*
        printf("============================[ Search Statistics ]==============================\n");
        printf("| Conflicts |          ORIGINAL         |          LEARNT          | Progress |\n");
        printf("|           |    Vars  Clauses Literals |    Limit  Clauses Lit/Cl |          |\n");
        printf("===============================================================================\n");
        */
        // modified by nabesima
        printf("==============================[ Search Statistics ]==============================\n");
        printf("| Conflicts |     ORIGINAL     |         LEARNT         | Props/   DLV/  Prgrss |\n");
        printf("|           |   Vars   Clauses |  Clauses Lit/Cl LBD/Cl |   Conf    Conf   [%%]  |\n");
        printf("=================================================================================\n");

        printLog();
    }

    // Search:
    //int curr_restarts = 0;
    curr_restarts = 0;    // modified by nabesima
    while (status == l_Undef){
        double rest_base = luby_restart ? luby(restart_inc, curr_restarts) : pow(restart_inc, curr_restarts);
        uint64_t prev_conflicts = conflicts;    // added by nabesima
        status = search((int)(rest_base * restart_first));

        // added by nabesima
        uint64_t confs = conflicts - prev_conflicts;
        if (confs < min_confs) min_confs = confs;
        if (confs > max_confs) max_confs = confs;

        if (!withinBudget()) break;
        curr_restarts++;

        // added by nabesima
        if (status == l_Undef) {
            if (lazy_clause_ops.size() > 0)
                if (!applyClauseOps())
                    status = l_False;
            switch (var_decay_strategy) {
            case VD_CONST:
                break;
            case VD_LINEAR:
                var_decay += (max_var_decay - init_var_decay) / var_decay_period;
                if (var_decay > max_var_decay) var_decay = max_var_decay;
                break;
            case VD_EXP:
                var_decay += (max_var_decay - var_decay) / var_decay_period;
                break;
            case VD_SIGMOID:
                var_decay = init_var_decay + (max_var_decay - init_var_decay) / (1 + exp(-((double)10 * starts / var_decay_period - 10)));
                break;
            default:
                assert(false);
                break;
            }
        }
    }

    // modified by nabesima
    if (verbosity >= 1) {
        //printf("===============================================================================\n");
        printLog();
        printf("=================================================================================\n");
    }

    if (status == l_True){
        // Extend & copy model:
        model.growTo(nVars());
        // modified by nabesima
        //for (int i = 0; i < nVars(); i++) model[i] = value(i);
        if (equivalent_lit.size() == 0)
            for (int i = 0; i < nVars(); i++) model[i] = value(i);
        else {
            for (int i = 0; i < nVars(); i++) {
                Lit p = equivalent_lit[i];
                if (p == lit_Undef)
                    model[i] = value(i);
                else {
                    while (equivalent_lit[var(p)] != lit_Undef)
                        p = sign(p) ? ~equivalent_lit[var(p)] : equivalent_lit[var(p)];
                    assert(value(p) != l_Undef);
                    model[i] = value(p);
                }
            }
        }
    }else if (status == l_False && conflict.size() == 0)
        ok = false;

    cancelUntil(0);

    return status;
}

//=================================================================================================
// Writing CNF to DIMACS:
//
// FIXME: this needs to be rewritten completely.

static Var mapVar(Var x, vec<Var>& map, Var& max)
{
    if (map.size() <= x || map[x] == -1){
        map.growTo(x+1, -1);
        map[x] = max++;
    }
    return map[x];
}


void Solver::toDimacs(FILE* f, Clause& c, vec<Var>& map, Var& max)
{
    if (satisfied(c)) return;

    for (int i = 0; i < c.size(); i++)
        if (value(c[i]) != l_False)
            fprintf(f, "%s%d ", sign(c[i]) ? "-" : "", mapVar(var(c[i]), map, max)+1);
    fprintf(f, "0\n");
}


void Solver::toDimacs(const char *file, const vec<Lit>& assumps)
{
    FILE* f = fopen(file, "wr");
    if (f == NULL)
        fprintf(stderr, "could not open file %s\n", file), exit(1);
    toDimacs(f, assumps);
    fclose(f);
}


void Solver::toDimacs(FILE* f, const vec<Lit>& assumps)
{
    // Handle case when solver is in contradictory state:
    if (!ok){
        fprintf(f, "p cnf 1 2\n1 0\n-1 0\n");
        return; }

    vec<Var> map; Var max = 0;

    // Cannot use removeClauses here because it is not safe
    // to deallocate them at this point. Could be improved.
    int cnt = 0;
    for (int i = 0; i < clauses.size(); i++)
        if (!satisfied(ca[clauses[i]]))
            cnt++;

    for (int i = 0; i < clauses.size(); i++)
        if (!satisfied(ca[clauses[i]])){
            Clause& c = ca[clauses[i]];
            for (int j = 0; j < c.size(); j++)
                if (value(c[j]) != l_False)
                    mapVar(var(c[j]), map, max);
        }

    // Assumptions are added as unit clauses:
    cnt += assumptions.size();

    fprintf(f, "p cnf %d %d\n", max, cnt);

    for (int i = 0; i < assumptions.size(); i++){
        assert(value(assumptions[i]) != l_False);
        fprintf(f, "%s%d 0\n", sign(assumptions[i]) ? "-" : "", mapVar(var(assumptions[i]), map, max)+1);
    }

    for (int i = 0; i < clauses.size(); i++)
        toDimacs(f, ca[clauses[i]], map, max);

    if (verbosity > 0)
        printf("Wrote %d clauses with %d variables.\n", cnt, max);
}


//=================================================================================================
// Garbage Collection methods:

void Solver::relocAll(ClauseAllocator& to)
{
    // All watchers:
    //
    watches.cleanAll();
    for (int v = 0; v < nVars(); v++)
        for (int s = 0; s < 2; s++){
            Lit p = mkLit(v, s);
            // printf(" >>> RELOCING: %s%d\n", sign(p)?"-":"", var(p)+1);
            vec<Watcher>& ws = watches[p];
            for (int j = 0; j < ws.size(); j++)
                ca.reloc(ws[j].cref, to);
        }


    // All reasons:
    //
    for (int i = 0; i < trail.size(); i++){
        Var v = var(trail[i]);

        if (reason(v) != CRef_Undef && (ca[reason(v)].reloced() || locked(ca[reason(v)])))
            ca.reloc(vardata[v].reason, to);
    }

    // All learnt:
    //
    for (int i = 0; i < learnts.size(); i++)
        ca.reloc(learnts[i], to);

    // All original:
    //
    for (int i = 0; i < clauses.size(); i++)
        ca.reloc(clauses[i], to);

    // added by nabesim
    for (int i = 0; i < lazy_clause_ops.size(); i++)
        ca.reloc(lazy_clause_ops[i].cref, to);
}


void Solver::garbageCollect()
{
    // Initialize the next region to a size corresponding to the estimated utilization degree. This
    // is not precise but should avoid some unnecessary reallocations for the new region:
    ClauseAllocator to(ca.size() - ca.wasted());

    relocAll(to);
    if (verbosity >= 2)
        printf("|  Garbage collection:   %12d bytes => %12d bytes             |\n",
               ca.size()*ClauseAllocator::Unit_Size, to.size()*ClauseAllocator::Unit_Size);
    to.moveTo(ca);
}


//=================================================================================================
// added by nabesima

void Solver::updatePSM() {
    for (int i=0; i < clauses.size(); i++)
        ca[clauses[i]].psm(polarity);
    for (int i=0; i < learnts.size(); i++)
        ca[learnts[i]].psm(polarity);
}

bool Solver::applyClauseOps() {
    assert(false);
    int i, j;
    for (i = j = 0; i < lazy_clause_ops.size(); i++) {
        COP&  cop = lazy_clause_ops[i];
        Clause& c = ca[cop.cref];
        if (cop.type == COP_MAKE_BIN) {
            detachClause(cop.cref, true);
            c[0] = cop.first;
            c[1] = cop.second;
            c.shrink(c.size() - 2);
            attachClause(cop.cref);
            c.learnt() ? lazy_subsumed_leartns++ : lazy_subsumed_clauses++;
        }
        else if ((cop.type == COP_LAZY_SHRINK || cop.type == COP_SELF_SUBSUMP) && cop.size == c.size()) {
            if (cop.size == 2) {
                Lit p = c[0] == cop.removed ? c[1] : c[0];
                if (value(p) == l_Undef)
                    uncheckedEnqueue(p);
                else if (value(p) == l_False)
                    return false;
            }
            else if (shrink_min_size == 0 || cop.size <= shrink_min_size) {
                detachClause(cop.cref, true);
                int k, l;
                for (k = l = 0; k < c.size(); k++) {
                    if (c[k] == cop.removed) {
                        if (cop.type == COP_LAZY_SHRINK)
                            c.learnt() ? lazy_rm_lrn_lits++ : lazy_rm_cla_lits++;
                        else
                            c.learnt() ? ss_rm_lrn_lits++ : ss_rm_cla_lits++;
                        continue;
                    }
                    c[l++] = c[k];
                }
                c.shrink(k - l);
                attachClause(cop.cref);
            }
            else if (c[0] == cop.removed || c[1] == cop.removed) {
                lazy_clause_ops[j++] = lazy_clause_ops[i];
            }
            else {
                int k, l;
                for (k = l = 2; k < c.size(); k++) {
                    if (c[k] == cop.removed) {
                        if (cop.type == COP_LAZY_SHRINK)
                            c.learnt() ? lazy_rm_lrn_lits++ : lazy_rm_cla_lits++;
                        else
                            c.learnt() ? ss_rm_lrn_lits++ : ss_rm_cla_lits++;
                        continue;
                    }
                    c[l++] = c[k];
                }
                c.shrink(k - l);
            }
        }
    }
    lazy_clause_ops.shrink(i - j);
    return true;
}

bool Solver::lazySimplify() {
    assert(decisionLevel() == 0);
    for (int i=0; i < lazy_fixed_lits.size(); i++) {
        PrbLit prb = lazy_fixed_lits[i];
        if (value(prb.p) == l_Undef) {
            uncheckedEnqueue(prb.p);
            lazy_probed_lits[prb.type]++;
            if (rup) writeLit(prb.p);
            if (propagate() != CRef_Undef)
                return false;
        }
        else if (value(prb.p) == l_False)
            return false;
        else
            lazy_probed_dup_lits[prb.type]++;
        Lit q = premise_lit[toInt(~prb.p)];
        if (value(q) == l_Undef)
            lazy_fixed_lits.push(PrbLit(~q, PRB_PREMISE_CHAIN));
        else if (value(q) == l_True)
            return false;
        if (equivalent_lit.size() > 0 && equivalent_lit[var(prb.p)] != lit_Undef) {
            Lit r = sign(prb.p) ? ~equivalent_lit[var(prb.p)] : equivalent_lit[var(prb.p)];
            if (value(r) == l_Undef)
                lazy_fixed_lits.push(PrbLit(r, PRB_EQV_LIT));
            else if (value(r) == l_False)
                return false;
        }
    }
    lazy_fixed_lits.clear();
    return true;
}

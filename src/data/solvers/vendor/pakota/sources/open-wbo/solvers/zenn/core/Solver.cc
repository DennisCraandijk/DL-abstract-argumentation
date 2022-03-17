/***************************************************************************************[Solver.cc]
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

using namespace Minisat;

//=================================================================================================
// Options:


static const char* _cat = "CORE";

static DoubleOption  opt_var_decay         (_cat, "var-decay",   "The variable activity decay factor",            0.95,     DoubleRange(0, false, 1, false));
static DoubleOption  opt_clause_decay      (_cat, "cla-decay",   "The clause activity decay factor",              0.999,    DoubleRange(0, false, 1, false));
static DoubleOption  opt_random_var_freq   (_cat, "rnd-freq",    "The frequency with which the decision heuristic tries to choose a random variable", 0, DoubleRange(0, true, 1, true));
static DoubleOption  opt_random_seed       (_cat, "rnd-seed",    "Used by the random variable selection",         91648253, DoubleRange(0, false, HUGE_VAL, false));
static IntOption     opt_ccmin_mode        (_cat, "ccmin-mode",  "Controls conflict clause minimization (0=none, 1=basic, 2=deep)", 2, IntRange(0, 2));
static IntOption     opt_phase_saving      (_cat, "phase-saving", "Controls the level of phase saving (0=none, 1=limited, 2=full)", 2, IntRange(0, 2));
static BoolOption    opt_rnd_init_act      (_cat, "rnd-init",    "Randomize the initial activity", false);
static BoolOption    opt_luby_restart      (_cat, "luby",        "Use the Luby restart sequence", true);
static IntOption     opt_restart_first     (_cat, "rfirst",      "The base restart interval", 100, IntRange(1, INT32_MAX));
static DoubleOption  opt_restart_inc       (_cat, "rinc",        "Restart interval increase factor", 2, DoubleRange(1, false, HUGE_VAL, false));
static DoubleOption  opt_garbage_frac      (_cat, "gc-frac",     "The fraction of wasted memory allowed before a garbage collection is triggered",  0.20, DoubleRange(0, false, HUGE_VAL, false));

static const char* _sinn = "SINN";

static IntOption     opt_mov_sol           (_sinn, "ms", " 0 : Sinn , 1 : glueminisat , 2 : 0+1", 2, IntRange(0,2));
static IntOption     opt_sinn_len          (_sinn, "sl", "ms=3のときSinnの動作をどれだけ長くやるか", 32, IntRange(0, INT32_MAX));
static IntOption     opt_glue_len          (_sinn, "gml", "ms=3のときglueminisatの動作をどれだけ長くやるか", 128, IntRange(0, INT32_MAX));
static IntOption     opt_agg_db            (_sinn, "ag","0 : minisat , 1 : glue, 3 : ps", 1, IntRange(0,2));
static IntOption     opt_safe_lbd          (_sinn, "safe_lbd", "", 2, IntRange(0, INT32_MAX));
static BoolOption    opt_true_lbd          (_sinn, "true", "", true);
static DoubleOption  opt_red_base          (_sinn, "red", "reduceDBでどれだけの節を残すか。",0.50, DoubleRange(0,false,1,false));
static IntOption     opt_safe_low          (_sinn, "safe_low", "LowestLBDがこれ以下の節は削除しない", 2, IntRange(0, INT32_MAX));
static IntOption     opt_how_safe_low      (_sinn, "h_safeLow", "safe_lowをどうやって使用するのか。 0 : 節削除はしないがウォッチからはずす, 1 : 削除せずウォッチからも外さない", 0, IntRange(0,1));
static IntOption     opt_whi_lt            (_sinn, "which_lt", "reduceDB時にどの構造体を利用するか。 0 : LBD-size , 1 : LBD-lowLBD-size, 2 : SINNフェーズでは0の動き、Uphaseでは1の動き", 0, IntRange(0,2));
static BoolOption    opt_glue_ver          (_sinn, "gmr", "Uphaseでglueminisatのリスタートを行うかどうか。falseなら50回で必ずリスタート", true);
static IntOption     opt_max_glue_conf     (_sinn, "max_glue", "Uphaseの最大コンフリクト数。＃＃この値の50倍が実際の値になる＃＃", 0, IntRange(0, INT32_MAX));
static IntOption     opt_var_luby          (_sinn, "var_luby", "ルビー関数を利用した様々な手法 0 : 通常, 1 : 一巡したら一巡した回数分だけ無視する, 2 : 一巡したら階差数列分だけ無視する, 3 : 一巡したらその巡の半分無視する", 2, IntRange(0,3));
static DoubleOption  opt_blv_restart_rate  (_sinn, "blv-restart-rate",  "Restarts if local BLV average * this val < global one", 1.05, DoubleRange(0, true, 1, true));
static IntOption 	   opt_recover_phase     (_sinn, "recover", "Uphaseにフェイズシフトするときに待機節をウォッチしなおす。reduceDBにまかせる。Uphaseになるときにし直す。Uphaseになるときに直し、SINNphaseで外す", 0, IntRange(0,2));
static BoolOption    opt_half_wait         (_sinn, "hw", "UphaseでのreduceDBで半分以下になるとき半分までの節は待機節にする", false);
// opt_restart_setについては、ミスって 0 : LBD + BLV, 1 : DLV + BLV, 2 : LBD + DLVとなってしまった。(0:LBD+DLVの予定だった)
static IntOption     opt_restart_set       (_sinn, "rs", "Uphaseで用いるリスタート。0 : LBD+BLV , 1 : DLV + BLV, 2 : LBD + DLV, 3 : ALL, 4 : LBD, 5 : DLV, 6 : BLV", 2, IntRange(0,6));
static IntOption     opt_ps_length         (_sinn, "ps_len", "propsDecMA reduceDBにて使用する移動平均の長さ", 1000, IntRange(1,INT32_MAX));
static IntOption     opt_ps_init              (_sinn, "ps_init", "propsDecMA reduceDBで最初のreduce判定を行うコンフリクト数", 15000, IntRange(1,INT32_MAX));
static IntOption     opt_ps_inc               (_sinn, "ps_inc", "propsDecMA reduceDBで判定を始めるコンフリクト数をどれだけ増やすか", 15000, IntRange(1,INT32_MAX));
static DoubleOption   opt_ps_rate         (_sinn, "ps_rate", "propsDecMA reduceDBで判定にしようするrate", 0.8, DoubleRange(0,false, INT32_MAX, true));
static IntOption     opt_ps_init_len       (_sinn, "ps_init_len", "propsDecMA reduceDBで使用する合計と回数を短縮するまでのディシジョン数", 524288, IntRange(1, INT32_MAX));
static IntOption     opt_luby_power		(_sinn, "luby_power", "lubyの返り値を変更する。0 : 2のn乗, 1 : nの2乗, 2 : n+1の2乗", 0, IntRange(0,2));

static IntOption		opt_appear_rank		(_sinn, "appear", "出現数利用のアクティビティ変化を使うか 0 : none, 1 : (学習節のみ)パラメータ/変数数, 2 : (学習節のみ)絶対数, 3 : （オリジナル節こみ）パラメータ/変数, 4 : (オリジナル節こみ）絶対数, 5 : （バイナリ節のみ）パラメータ/変数, 6 : （バイナリ節のみ）絶対数", 0, IntRange(0,6));
static IntOption		opt_appear_freq		(_sinn, "ap_freq", "出現数利用を何Phase毎に行なうか 1だと全てのPhaseで行なう, 2だと交互に行なう, 3だと3Phaseに1回行なう", 3, IntRange(1, INT32_MAX));
static DoubleOption	opt_appear_param	(_sinn, "ap_param", "出現数利用1のパラメータ", 0.333, DoubleRange(0,false,1,false));
static IntOption		opt_appear_num		(_sinn, "ap_num", "出現数利用2の絶対数", 100, IntRange(0, INT32_MAX));
static DoubleOption	opt_appear_boost	(_sinn, "ap_boost", "出現数で上位の変数のアクティビティ上昇の幅", 2.0, DoubleRange(0,true, INT32_MAX, true));
static BoolOption		opt_appear_have		(_sinn, "ap_have", "節削除時に再構成するかどうか", false);

//////		Half Half
static BoolOption		opt_half_half			(_sinn, "half", "", false);
static IntOption		opt_change_res		(_sinn, "cres", "", 8, IntRange(0, 8));

//////		safe high
static IntOption		opt_safe_high			(_sinn, "safe_high", "", 60, IntRange(0,INT32_MAX));

static BoolOption		opt_phase_var_decay			(_sinn, "phase_decay", "", true);
static DoubleOption  opt_var_decay_sinn         (_cat, "sinn_decay",   "The variable activity decay factor phase SINN",            0.990,     DoubleRange(0, false, 1, false));
static DoubleOption  opt_var_decay_glue         (_cat, "glue_decay",   "The variable activity decay factor phase Uphase",          0.800,     DoubleRange(0, false, 1, false));

static const char* _zenn = "ZENN";

static IntOption			opt_clause_imp	(_zenn, "ci", "(SINNフェーズの)節重要度に何を使用するか。0 : Newest LBD, 1 : VANR, 2 : LBD+VANR , 3 : LBD*VANR+WAIT , 4 : VANR like glue", 0, IntRange(0,4));
//////		VANR
static DoubleOption		opt_vanr_rate		(_zenn, "vanr_rate", "VANRの計算に用いるレート",1.10,DoubleRange(0,false,INT32_MAX,false));
static IntOption			opt_safe_vanr		(_zenn, "safe_vanr", "これ以下のVANRを持つ学習節は捨てない。", 1, IntRange(0,INT32_MAX));
//////		LBD+VANR
static DoubleOption		opt_alive_rate		(_zenn, "alive_rate", "LBD+VANRの時に上位何割のものを残すのか", 0.30, DoubleRange(0,false,1,false));

static const char* _glue = "GLUE";

static IntOption     opt_learnts_init      (_glue, "linit", "", 30000, IntRange(1,INT32_MAX));
static IntOption     opt_learnts_inc       (_glue, "linc", "", 10000, IntRange(1,INT32_MAX));
static IntOption     opt_restart_min_confs (_glue, "restart_leng", "The number of conflicts for next restart", 50, IntRange(1, INT32_MAX));
static DoubleOption  opt_lbd_restart_rate  (_glue, "lbd-restart-rate",  "Restarts if local LBD average * this val < global one", 0.8, DoubleRange(0, true, 1, true));
static DoubleOption  opt_dlv_restart_rate  (_glue, "dlv-restart-rate",  "Restarts if local DLV average * this val < global one", 1.0, DoubleRange(0, true, 1, true));
// XXX option end
//=================================================================================================
// Constructor/Destructor:

#define SINN	0
#define NABE	1



Solver::Solver() :

    		// Parameters (user settable):
    		//
    		verbosity        (0)
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
, dec_vars(0), clauses_literals(0), learnts_literals(0), max_literals(0), tot_literals(0), tot_change_watch(0)

// variables for Sinn

// variables for Glue
, reduce_dbs (0), tot_lbds (0)

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
// options
, movement_of_solver (opt_mov_sol)
, length_of_sinn (opt_sinn_len)
, length_of_glueminisat (opt_glue_len)
, aggressive_reduce_db (opt_agg_db)
, safeLBD (opt_safe_lbd)
, use_true_lbd (opt_true_lbd)
, reduce_base(opt_red_base)
, rest_reduce(1.00 - reduce_base)
, safeLowLBD (opt_safe_low)
, usage_safe_low (opt_how_safe_low)
, which_reduce_lt (opt_whi_lt)
, glue_version (opt_glue_ver)
, max_glue_conflicts (opt_max_glue_conf)
, variety_of_luby (opt_var_luby)
, recover_Uphase (opt_recover_phase)
, half_waiting_uphase (opt_half_wait)
, restart_set (opt_restart_set)

// variables for Sinn
, which_method (SINN)
, sinn_starts (0)
, method_starts (0)
, num_watched_clauses (0)
, current_phase_confls (0)
, luby_circle_num (0)
, luby_circle_key (-1)
, luby_circle_key_inc (0)
, luby_circle_inc (0)

// variables for Glue
, reduce_db_base (opt_learnts_init)
, reduce_db_inc (opt_learnts_inc)
, reduce_db_limit (reduce_db_base)
, restart_min_confs (opt_restart_min_confs)
, lbd_restart_rate (opt_lbd_restart_rate)
, dlv_restart_rate (opt_dlv_restart_rate)
, blv_restart_rate (opt_blv_restart_rate)

, lbd_updates (0)

, wholeLBDs (0.0)
, wholeDLVs (0.0)
, wholeBLVs (0.0)

, psMA_length (opt_ps_length)
, disturb (true)
, ps_reduce_db_limit (opt_ps_init)
, ps_reduce_db_inc (0)
, ps_reduce_db_inc_sub (opt_ps_inc)
, ps_rate (opt_ps_rate)
, ps_init_length (opt_ps_init_len)
// SQUARE Luby
, luby_power (opt_luby_power)
// Half Half
, sinn_phase_num (1)
, uphase_num (0)
, use_halfhalf (opt_half_half)
, default_restart_strategy (restart_set)
, change_restart_strategy (opt_change_res)

, use_appear (opt_appear_rank)
, frequence_appear (opt_appear_freq-1)
, parameter_appear (opt_appear_param)
, number_appear (opt_appear_num)
, booster (opt_appear_boost)
, use_having_num (opt_appear_have)
, phase_num (0)
, upper_num (0)

//////////////////////////////////////////////		ZENN
, clause_importance (opt_clause_imp)

/////		VANR
, vanr_rate (opt_vanr_rate)
, safe_vanr (opt_safe_vanr)
, tot_assigns (0)

////		LBD+VANR
, alive_rate (opt_alive_rate)

, safe_high (opt_safe_high)

, phase_var_decay (opt_phase_var_decay)
, var_decay_sinn (opt_var_decay_sinn)
, var_decay_glue (opt_var_decay_glue)
{
  output = NULL;
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

	// added for LBD
	lbd_time.push(0);
	// VANR
//	var_order.push(0);

	return v;
}


bool Solver::addClause_(vec<Lit>& ps)
{
	assert(decisionLevel() == 0);
	if (!ok) return false;

	// Check if clause is satisfied and remove false/duplicate literals:
	sort(ps);
	Lit p; int i, j;
	for (i = j = 0, p = lit_Undef; i < ps.size(); i++)
		if (value(ps[i]) == l_True || ps[i] == ~p)
			return true;
		else if (value(ps[i]) != l_False && ps[i] != p)
			ps[j++] = p = ps[i];
	ps.shrink(i - j);

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
//	const Clause& c = ca[cr];
	Clause& c = ca[cr];
	assert(c.size() > 1);
	watches[~c[0]].push(Watcher(cr, c[1]));
	watches[~c[1]].push(Watcher(cr, c[0]));
	if (c.learnt()){
		learnts_literals += c.size();
		num_watched_clauses++;
	}
	else            clauses_literals += c.size();
	assert(!c.attached());
	c.attach();
}


void Solver::detachClause(CRef cr, bool strict) {
//	const Clause& c = ca[cr];
	Clause& c = ca[cr];
	assert(c.size() > 1);

	if (strict){
		remove(watches[~c[0]], Watcher(cr, c[1]));
		remove(watches[~c[1]], Watcher(cr, c[0]));
	}else{
		// Lazy detaching: (NOTE! Must clean all watcher lists before garbage collecting this clause)
		watches.smudge(~c[0]);
		watches.smudge(~c[1]);
	}

	if (c.learnt()) {
		learnts_literals -= c.size();
		num_watched_clauses--;
	}
	else            clauses_literals -= c.size();
	assert(c.attached());
	c.detach();
}


void Solver::removeClause(CRef cr) {
	Clause& c = ca[cr];
	if(c.attached())
		detachClause(cr);
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
			if (phase_saving > 1 || (phase_saving == 1) && c > trail_lim.last())
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
// XXX analyze
void Solver::analyze(CRef confl, vec<Lit>& out_learnt, int& out_btlevel, int& lbd)
{
	int pathC = 0;
	Lit p     = lit_Undef;

	// Generate conflict clause:
	//
	out_learnt.push();      // (leave room for the asserting literal)
	int index   = trail.size() - 1;

	implied_by_learnts.clear();

	do{
		assert(confl != CRef_Undef); // (otherwise should be UIP)
		Clause& c = ca[confl];

//		if (c.learnt())
//			claBumpActivity(c);

		for (int j = (p == lit_Undef) ? 0 : 1; j < c.size(); j++){
			Lit q = c[j];

			if (!seen[var(q)] && level(var(q)) > 0){
//				if(reason(var(q)) != CRef_Undef && ca[reason(var(q))].learnt() && ca[reason(var(q))].lbd() < 3)
//					varBumpActivity(var(q), var_inc * (1/var_decay));
//				else
				if(use_appear != APP_NONE && phase_num == frequence_appear && appearRank.upper(var(q)))
					varBumpActivity(var(q), var_inc * booster);
				else
					varBumpActivity(var(q));
				seen[var(q)] = 1;
				if (level(var(q)) >= decisionLevel()){
					if(which_method == NABE && reason(var(q)) != CRef_Undef && ca[reason(var(q))].learnt())
						implied_by_learnts.push(q);
					pathC++;
				}
				else
					out_learnt.push(q);
			}
		}

		// Select next clause to look at:
		while (!seen[var(trail[index--])]);
		p     = trail[index+1];
		confl = reason(var(p));
		seen[var(p)] = 0;
		pathC--;

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
	}

	// added for LBD
	lbd = 0;
	lbd_updates++;
	for(int i=0;i < out_learnt.size();i++){
		// 出現数利用
			if((use_appear == APP_BIN_PARAM || use_appear == APP_BIN_NUM) && out_learnt.size() == 2)
				appearRank.add(var(out_learnt[i]));
			else if(use_appear != APP_NONE)
				appearRank.add(var(out_learnt[i]));
		// 出現数利用終わり
		int lv = level(var(out_learnt[i]));
		if(lbd_time[lv] != lbd_updates){
			lbd_time[lv] = lbd_updates;
			lbd++;
		}
	}

	tot_lbds += lbd;
	if(implied_by_learnts.size() > 0){
		for(int i=0;i<implied_by_learnts.size();i++){
			if(ca[reason(var(implied_by_learnts[i]))].lbd() < lbd)
//				varBumpActivity(var(implied_by_learnts[i]), var_inc *(1/var_decay) - var_inc);
				varBumpActivity(var(implied_by_learnts[i]));
		}
		implied_by_learnts.clear();
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


void Solver::uncheckedEnqueue(Lit p, CRef from)
{
	assert(value(p) == l_Undef);
	assigns[var(p)] = lbool(!sign(p));
	vardata[var(p)] = mkVarData(from, decisionLevel());
	trail.push_(p);
	tot_assigns++;
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
	CRef    confl     = CRef_Undef;
	int     num_props = 0;
	watches.cleanAll();

	while (qhead < trail.size()){
		Lit            p   = trail[qhead++];     // 'p' is enqueued fact to propagate.
		vec<Watcher>&  ws  = watches[p];
		Watcher        *i, *j, *end;
		num_props++;
		int k;

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
			for (k = 2; k < c.size(); k++)
				if (value(c[k]) != l_False){
					tot_change_watch++;
					c[1] = c[k]; c[k] = false_lit;
					watches[~c[1]].push(w);
					goto NextClause; }

			// Did not find watch -- clause is unit under assignment:
			*j++ = w;
			if (value(first) == l_False){
				confl = cr;
				qhead = trail.size();
				// Copy the remaining watches:
				while (i < end)
					*j++ = *i++;
			}else
				uncheckedEnqueue(first, cr);

			// update LBD
			if(c.learnt() && clause_importance != CLA_VANR){
				int lbd = 0;
				lbd_updates++;
				for(k=0;k<c.size();k++){
					if(k == 1)			// c[0]とc[1]の割り当てられたレベルは同一なので
						continue;
					int lv = level(var(c[k]));
					if(use_true_lbd && lv == 0)
						continue;
					if(lbd_time[lv] != lbd_updates){
						lbd_time[lv] = lbd_updates;
						lbd++;
					}
				}
//				assert(lbd > 0);
				tot_lbds += lbd - c.lbd();
				c.set_lbd(lbd);
				if(c.safe_high() && lbd > safe_high)
					c.isNotSafeHigh();
			}

			NextClause:;
		}
		ws.shrink(i - j);
	}
	propagations += num_props;
	simpDB_props -= num_props;

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
////		LBDで並び替えているが、実際はVANRもこれで並び替えが可能である。
struct reduceDB_lt {
	ClauseAllocator& ca;
	reduceDB_lt(ClauseAllocator& ca_) : ca(ca_) {}
	bool operator () (CRef x, CRef y){
		// size 2 基準
		//size2基準
		if(ca[x].size() > 2 && ca[y].size() == 2) return true;
		if(ca[y].size() > 2 && ca[x].size() == 2) return false;
		if(ca[x].size() == 2 && ca[y].size() == 2) return false;

		if(ca[x].lbd() > ca[y].lbd()) return true;
		if(ca[x].lbd() < ca[y].lbd()) return false;

		//最終的な評価は節の長さ
		return ca[x].size() > ca[y].size();
	}

//	bool operator () (CRef x, CRef y) {
//		return ca[x].size() > 2 && (ca[y].size() == 2 || ca[x].activity() < ca[y].activity()); }
};
// LBDが同じならLowLBDを利用するversion
struct reduceDB_low_lt {
	ClauseAllocator& ca;
	reduceDB_low_lt(ClauseAllocator& ca_) : ca(ca_) {}
	bool operator () (CRef x, CRef y){
		// size 2 基準
		//size2基準
		if(ca[x].size() > 2 && ca[y].size() == 2) return true;
		if(ca[y].size() > 2 && ca[x].size() == 2) return false;
		if(ca[x].size() == 2 && ca[y].size() == 2) return false;

		if(ca[x].lbd() > ca[y].lbd()) return true;
		if(ca[x].lbd() < ca[y].lbd()) return false;

		if(ca[x].lowLBD() > ca[y].lowLBD()) return true;
		if(ca[x].lowLBD() < ca[y].lowLBD()) return false;

		//最終的な評価は節の長さ
		return ca[x].size() > ca[y].size();
	}

//	bool operator () (CRef x, CRef y) {
//		return ca[x].size() > 2 && (ca[y].size() == 2 || ca[x].activity() < ca[y].activity()); }
};
struct reduceDB_lt_vanr {
    ClauseAllocator& ca;
    reduceDB_lt_vanr(ClauseAllocator& ca_) : ca(ca_) {}
    bool operator () (CRef x, CRef y) {
		//size2基準
		if(ca[x].size() > 2 && ca[y].size() == 2) return true;
		if(ca[y].size() > 2 && ca[x].size() == 2) return false;
		if(ca[x].size() == 2 && ca[y].size() == 2) return false;

		if(ca[x].vanr() > ca[y].vanr()) return true;
		if(ca[x].vanr() < ca[y].vanr()) return false;

		//最終的な評価は節の長さ
		return ca[x].size() > ca[y].size();
    }
};
// XXX reduceDB
void Solver::reduceDB()
{
	int     i, j;

//	int total_lbd=0;
//	for(i=0;i<learnts.size();i++){
//		total_lbd += ca[learnts[i]].lbd();
//	}
//
//	int total = (int)tot_lbds;
//	int confls = (int)conflicts;
//	printf("conflicts             : %-12"PRIu64"\n", conflicts);
//	printf("tot_lbds             : %-12"PRIu64"\n", tot_lbds);
//	printf("decLevel : %d , conflicts : %d , tot_lbd : %d , total_lbd : %d\n",decisionLevel(),confls, total, total_lbd);
	if(aggressive_reduce_db == 1) reduce_dbs++;
//	if(which_reduce_lt == 0 || (which_reduce_lt == 2 && which_method == SINN))
//		sort(learnts, reduceDB_lt(ca));
//	else if(which_reduce_lt == 1 || (which_reduce_lt == 2 && which_method == NABE))
//		sort(learnts, reduceDB_low_lt(ca));

	double beforeDelete = learnts.size();
	if(verbosity > 2)
		  printf("c confl : %u | leants : %d , watched : %d, waitings : %d", (unsigned int)conflicts, learnts.size(), num_watched_clauses, waitings.size());

	if(which_method == SINN){
		if(verbosity > 2)
			printf("c SINN ");

		// どの基準で並び替えるか選択する
		if(clause_importance == CLA_LBD){
			if(which_reduce_lt == 0 || which_reduce_lt == 2)
				sort(learnts, reduceDB_lt(ca));
			else if(which_reduce_lt == 1)
				sort(learnts, reduceDB_low_lt(ca));

			int org_size = (int)(rest_reduce * num_watched_clauses) + nLearnts() - num_watched_clauses;
			for (i = j = 0; i < learnts.size(); i++){
				Clause& c = ca[learnts[i]];
				if (c.size() > 2 && !locked(c) && i < org_size && c.lbd() > safeLBD){
					if(c.lowLBD() > safeLowLBD || !c.safe_high())
						removeClause(learnts[i]);
					else if(usage_safe_low == 0){		//lowestLBDがsafe_lowより小さいので削除しない
						// 削除はしないが、ウォッチからは外す
						if(c.attached())
							detachClause(learnts[i],true);
						learnts[j++] = learnts[i];
					}
					else												// 削除しないし、ウォッチからも外さない
						learnts[j++] = learnts[i];
				}
				else{
					if(!c.attached())
						attachClause(learnts[i]);
					learnts[j++] = learnts[i];
				}
			}
		}
		else if(clause_importance == CLA_VANR){
			if(verbosity > 2)
				printf("c | VANR | ");
	    	int vanr_clause_base = (int) ((tot_assigns/conflicts)*vanr_rate);
//			double extra_lim = var_inc * vanr_rate;												// VANR Second
//	    	if(verbosity > 2)
//	    		printf("vanr_base : %d", vanr_clause_base);
			for(i=0;i<learnts.size();i++){
				Clause& c = ca[learnts[i]];
				int gap = 0;
				int psm = 0;
				for(j=0;j<c.size();j++){
					if(order_heap.order(var(c[j])) < 0)
						assert(value(c[j]) != l_Undef);
					if(order_heap.order(var(c[j])) >= vanr_clause_base){
//					else if(activity[var(c[j])] < extra_lim){
						gap++;
					}
					// psm
					if(polarity[var(c[j])] == sign(c[j])) psm++;
				}
				c.vanr() = gap*psm;
			}
			if(verbosity > 2){
				double literals_in_learnts = 0;
				double vanr_learnts = 0;
				for(i = 0;i<learnts.size();i++){
					Clause& c = ca[learnts[i]];
					literals_in_learnts += c.size();
					vanr_learnts += c.vanr();
				}
				printf("c \nc vanrs/lits : %f	literals : %f	vanrs : %f\n", vanr_learnts/literals_in_learnts, literals_in_learnts, vanr_learnts);
			}
			// VANRで並べ替える（LBDの時と同じ構造体を使える）
			sort(learnts, reduceDB_lt_vanr(ca));
			int org_size = (int)(rest_reduce * num_watched_clauses) + nLearnts() - num_watched_clauses;
			for (i = j = 0; i < learnts.size(); i++){
				Clause& c = ca[learnts[i]];
				if (c.size() > 2 && !locked(c) && i < org_size && c.vanr() > safe_vanr){
					if(c.lowLBD() > safeLowLBD)
						removeClause(learnts[i]);
					else if(usage_safe_low == 0){		//lowestLBDがsafe_lowより小さいので削除しない
						// 削除はしないが、ウォッチからは外す
						if(c.attached())
							detachClause(learnts[i],true);
						learnts[j++] = learnts[i];
					}
					else												// 削除しないし、ウォッチからも外さない
						learnts[j++] = learnts[i];
				}
				else{
					if(!c.attached())
						attachClause(learnts[i]);
					learnts[j++] = learnts[i];
				}
			}
		}
		else if(clause_importance == CLA_LBDVANR){
	    	if(verbosity > 2)
	    		printf("c | LBD+VANR | ");
			int limit = (int)((1.0 - alive_rate) * learnts.size());			// 節の3/4までは削除対象

	    	temporary_saveLBD.clear();
	    	temporary_saveLBD.growTo(nLearnts());

	    	int vanr_clause_base = (int) ((tot_assigns/conflicts)*vanr_rate);
	    	if(verbosity > 2)
	    		printf("c vanr_base : %d", vanr_clause_base);
	    	for(i=0;i<learnts.size();i++){
	    		Clause& c = ca[learnts[i]];
	    		int gap = 0;
	    		for(j=0;j<c.size();j++){
	    			if(order_heap.order(var(c[j])) < 0)
	    				assert(value(c[j]) != l_Undef);
	    			if(order_heap.order(var(c[j])) >= vanr_clause_base){
	    				gap++;
	    			}
	    		}
	    		temporary_saveLBD[i].learnt = learnts[i];			// 節のLBDを別の場所に記憶
	    		temporary_saveLBD[i].LBD = c.lbd();
	    		c.vanr() = gap;
	    	}
	    	sort(learnts, reduceDB_lt(ca));
	    	for (i = j = 0; i < learnts.size(); i++){
	    		Clause& c = ca[learnts[i]];
	    		if (c.size() > 2 && !locked(c) && i < limit && c.vanr() > safe_vanr)
	    			c.keep(false);
	    		else
	    			c.keep(true);
	    	}
	    	// LBDを節に戻す
	    	for(i=0;i<temporary_saveLBD.size();i++)
	    		ca[temporary_saveLBD[i].learnt].set_lbd((uint32_t) temporary_saveLBD[i].LBD);
	    	temporary_saveLBD.clear();

	    	sort(learnts, reduceDB_lt(ca));
//			int org_size = (int)(rest_reduce * num_watched_clauses) + nLearnts() - num_watched_clauses;
			for (i = j = 0; i < learnts.size(); i++){
				Clause& c = ca[learnts[i]];
				if (c.size() > 2 && !locked(c) && !c.keep() && i < limit && c.lbd() > safeLBD){
					if(c.lowLBD() > safeLowLBD)
						removeClause(learnts[i]);
					else if(usage_safe_low == 0){		//lowestLBDがsafe_lowより小さいので削除しない
						// 削除はしないが、ウォッチからは外す
						if(c.attached())
							detachClause(learnts[i],true);
						learnts[j++] = learnts[i];
					}
					else												// 削除しないし、ウォッチからも外さない
						learnts[j++] = learnts[i];
				}
				else{
					if(!c.attached())
						attachClause(learnts[i]);
					learnts[j++] = learnts[i];
				}
			}

		}
		else if(clause_importance == CLA_LBDVANRW){
	    	if(verbosity > 2)
	    		printf("c | LBD*VANR+WAIT | ");
			int limit = (int)((1.0 - alive_rate) * learnts.size());			// 節の3/4までは削除対象

	    	temporary_saveLBD.clear();
	    	temporary_saveLBD.growTo(nLearnts());

	    	int vanr_clause_base = (int) ((tot_assigns/conflicts)*vanr_rate);
	    	if(verbosity > 2)
	    		printf("c vanr_base : %d", vanr_clause_base);
	    	for(i=0;i<learnts.size();i++){
	    		Clause& c = ca[learnts[i]];
	    		int gap = 0;
	    		for(j=0;j<c.size();j++){
	    			if(order_heap.order(var(c[j])) < 0)
	    				assert(value(c[j]) != l_Undef);
	    			if(order_heap.order(var(c[j])) >= vanr_clause_base){
	    				gap++;
	    			}
	    		}
	    		temporary_saveLBD[i].learnt = learnts[i];			// 節のLBDを別の場所に記憶
	    		temporary_saveLBD[i].LBD = c.lbd();
	    		c.vanr() = gap;
	    	}
	    	sort(learnts, reduceDB_lt(ca));
	    	for (i = j = 0; i < learnts.size(); i++){
	    		Clause& c = ca[learnts[i]];
	    		if (c.size() > 2 && !locked(c) && i < limit && c.vanr() > safe_vanr)
	    			c.keep(false);
	    		else
	    			c.keep(true);
	    	}
	    	// LBDを節に戻す
	    	for(i=0;i<temporary_saveLBD.size();i++)
	    		ca[temporary_saveLBD[i].learnt].set_lbd((uint32_t) temporary_saveLBD[i].LBD);
	    	temporary_saveLBD.clear();

	    	sort(learnts, reduceDB_lt(ca));
//			int org_size = (int)(rest_reduce * num_watched_clauses) + nLearnts() - num_watched_clauses;
	    	int deleteLimit = (int)(learnts.size() * 0.75);
//	    	printf("deleteLimit : %d limit : %d\n", deleteLimit, limit);
			for (i = j = 0; i < learnts.size(); i++){
				Clause& c = ca[learnts[i]];
				if(locked(c) || c.size() <= 2 || c.lbd() <= safeLBD || i > deleteLimit  || (i >= limit && c.keep())){
					// 必ず残す節
					if(!c.attached())
						attachClause(learnts[i]);
					learnts[j++] = learnts[i];
				}
				else if(i >= limit || c.lowLBD() <= safeLowLBD){
					// 待機する節
					if(c.attached())
						detachClause(learnts[i],true);
					learnts[j++] = learnts[i];
				}
				else
					removeClause(learnts[i]);
			}

//			printf("i : %d , j : %d\n", i, j);
		}
//		else if(clause_importance == CLA_VANRU){
		else{
	    	if(verbosity > 2)
	    		printf("c | VANR like U | ");

//			int limit = (int)((1.0 - alive_rate) * learnts.size());			// 節の3/4までは削除対象

	    	// VANRの計算とLBD値の保存
	    	temporary_saveLBD.clear();
	    	temporary_saveLBD.growTo(nLearnts());
	    	int vanr_clause_base = (int) ((tot_assigns/conflicts)*vanr_rate);
	    	if(verbosity > 2)
	    		printf("c vanr_base : %d", vanr_clause_base);
	    	for(i=0;i<learnts.size();i++){
	    		Clause& c = ca[learnts[i]];
	    		int gap = 0;
	    		for(j=0;j<c.size();j++){
	    			if(order_heap.order(var(c[j])) < 0)
	    				assert(value(c[j]) != l_Undef);
	    			if(order_heap.order(var(c[j])) >= vanr_clause_base){
	    				gap++;
	    			}
	    		}
	    		temporary_saveLBD[i].learnt = learnts[i];			// 節のLBDを別の場所に記憶
	    		temporary_saveLBD[i].LBD = c.lbd();
	    		c.vanr() = gap;
	    	}
	    	sort(learnts, reduceDB_lt(ca));
	    	// LBDを節に戻す
	    	for(i=0;i<temporary_saveLBD.size();i++)
	    		ca[temporary_saveLBD[i].learnt].set_lbd((uint32_t) temporary_saveLBD[i].LBD);
	    	temporary_saveLBD.clear();

	    	int org_size = learnts.size();
	    	for (i = j = 0; i < org_size; i++){
	    		Clause& c = ca[learnts[i]];
	    		if (locked(c)){
	    			if(!c.attached())
	    				attachClause(learnts[i]);
	    			learnts[j++] = learnts[i];
	    		}
	    		//else if (c.activity() == 0 && c.lbd() > 2)
	    		//	removeClause(learnts[i]);
	    		else if (c.lbd() <= 3 || c.lowLBD() <= safeLowLBD){									//ここはsafeLBDのように変数作ったほうが良いかも
	    			if(!c.attached())
	    				attachClause(learnts[i]);
	    			learnts[j++] = learnts[i];
	    		}
	    		else if ((j + org_size - i) < (org_size / 4)){// At least 25% learnts are preversed.
	    			if(!c.attached())
	    				attachClause(learnts[i]);
	    			learnts[j++] = learnts[i];
	    		}
	    		else if(half_waiting_uphase && i > org_size / 2){
					if(c.attached())
						detachClause(learnts[i],true);
					learnts[j++] = learnts[i];
	    		}
	    		else
	    			removeClause(learnts[i]);
	    	}
		}
	}
//	else if(which_method == NABE){
	else{
		if(verbosity > 2)
			printf("c Uphase ");

		// どの基準で並び替えるか選択する
		if(which_reduce_lt == 0)
			sort(learnts, reduceDB_lt(ca));
		else if(which_reduce_lt == 1 || which_reduce_lt == 2)
			sort(learnts, reduceDB_low_lt(ca));

    	int org_size = learnts.size();
        for (i = j = 0; i < org_size; i++) {
        	Clause& c = ca[learnts[i]];

    		if (locked(c)){
    			if(!c.attached())
    				attachClause(learnts[i]);
    			learnts[j++] = learnts[i];
    		}
    		//else if (c.activity() == 0 && c.lbd() > 2)
    		//	removeClause(learnts[i]);
    		else if (c.lbd() <= 3 || (c.lowLBD() <= safeLowLBD && c.safe_high())){									//ここはsafeLBDのように変数作ったほうが良いかも
    			if(!c.attached())
    				attachClause(learnts[i]);
    			learnts[j++] = learnts[i];
    		}
    		else if ((j + org_size - i) < (org_size / 4)){// At least 25% learnts are preversed.
    			if(!c.attached())
    				attachClause(learnts[i]);
    			learnts[j++] = learnts[i];
    		}
    		else if(half_waiting_uphase && i > org_size / 2){
				if(c.attached())
					detachClause(learnts[i],true);
				learnts[j++] = learnts[i];
    		}
    		else
    			removeClause(learnts[i]);
        }
        if (num_watched_clauses > org_size / 2) {
			learnts.shrink(i - j);
			for (i = j = 0; i < learnts.size(); i++) {
	        	Clause& c = ca[learnts[i]];
	    		if (!locked(c) && i < learnts.size() - org_size / 2)
	    			removeClause(learnts[i]);
	    		else{
	    			learnts[j++] = learnts[i];
	    		}
			}
        }
	}
	learnts.shrink(i - j);
	checkGarbage();
	  if(verbosity > 2)
		  printf("c - > learnts : %d, watched : %d, waitings : %d	remainRate : %f\n",nLearnts(), num_watched_clauses, waitings.size(), (nLearnts()/beforeDelete) * 100);

	  if(use_having_num && use_appear != APP_NONE){
		  if(use_appear == APP_BIN_NUM || use_appear == APP_BIN_PARAM){
			  appearRank.init(nVars(),upper_num);
			  for(i = 0;i<clauses.size();i++){
				  Clause& c = ca[clauses[i]];
				  if(c.size() == 2){
					  for(j=0;j<c.size();j++)
						  appearRank.add(var(c[j]));
				  }
			  }
			  for(i = 0;i<learnts.size();i++){
				  Clause& c = ca[learnts[i]];
				  if(c.size() == 2){
					  for(j=0;j<c.size();j++)
						  appearRank.add(var(c[j]));
				  }
			  }
		  }
		  else{
			  appearRank.init(nVars(),upper_num);
			  if(use_appear == APP_ALL_NUM || APP_ALL_PARAM){
				  for(i = 0;i<clauses.size();i++){
					  Clause& c = ca[clauses[i]];
					  for(j=0;j<c.size();j++)
						  appearRank.add(var(c[j]));
				  }
			  }
			  for(i = 0;i<learnts.size();i++){
				  Clause& c = ca[learnts[i]];
				  for(j=0;j<c.size();j++)
					  appearRank.add(var(c[j]));
			  }
		  }
	  }
}


void Solver::removeSatisfied(vec<CRef>& cs)
{
	int i, j;
	for (i = j = 0; i < cs.size(); i++){
		Clause& c = ca[cs[i]];
		if (satisfied(c)){
			removeClause(cs[i]);
			c.satisfy();
		}
		else
			cs[j++] = cs[i];
	}
	cs.shrink(i - j);
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

	if (nAssigns() == simpDB_assigns || (simpDB_props > 0))
		return true;

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
	int         lbd;
	int         conflictC = 0;
	vec<Lit>    learnt_clause;
	starts++;

	if(method_starts == 0) current_phase_confls = 0;	//現在のフェーズのコンフリクト数を数える。
	if(movement_of_solver == 2){
		if(which_method == SINN && method_starts >= length_of_sinn){	// SINN -> Uphase
			which_method = NABE;
			phase_num++;		// 出現数用
			uphase_num++;
			if(use_halfhalf){		// Half Half
				if(uphase_num & 1)
					restart_set = default_restart_strategy;
				else
					restart_set = change_restart_strategy;
				if(verbosity > 2)
					printf("c restart set : %d	", restart_set);
			}
			if(phase_num > frequence_appear)
				phase_num = 0;
			method_starts = 0;
			if(recover_Uphase != RECOVER_NONE){
				int added = 0;
				for(int i=0;i<waitings.size();i++){
					if(!ca[waitings[i]].satisfied() && !ca[waitings[i]].attached() && ca[waitings[i]].lowLBD() <= safeLowLBD){
						added++;
						attachClause(waitings[i]);
					}
				}
//				assert(num_watched_clauses == learnts.size());
				if(verbosity > 2)
					printf("c Added %d		| ",added);
			}
			if(recover_Uphase == RECOVER_UPHASE)
				waitings.clear();
			if(verbosity > 2)
				printf("c PhaseShift -> Uphase    learnts : %d  watched : %d wait : %d\n",learnts.size(),num_watched_clauses, waitings.size());
		}
		else if(which_method == NABE && (method_starts >= length_of_glueminisat || current_phase_confls >= max_glue_conflicts)){
			// Uphase -> SINN
			which_method = SINN;
			sinn_phase_num++;
			phase_num++;		// 出現数用
			if(phase_num > frequence_appear)
				phase_num = 0;
			method_starts  = 0;
			if(recover_Uphase == RECOVER_US){
				// 待機節のウォッチを外す
				for(int i=0;i<waitings.size();i++){
					if(ca[waitings[i]].attached() && !ca[waitings[i]].satisfied())
						detachClause(waitings[i], true);
				}
			}
			if(verbosity > 2)
				printf("c PhaseShift -> SINN    learnts : %d  watched : %d wait : %d\n",learnts.size(), num_watched_clauses, waitings.size());
		}
		else
			method_starts++;

		if(use_halfhalf && which_method == NABE && method_starts == length_of_glueminisat/2){
			if(uphase_num & 1)
				restart_set = change_restart_strategy;
			else
				restart_set = default_restart_strategy;
			if(verbosity > 2)
				printf("c change to restart set : %d\n", restart_set);
		}
	}

	if(which_method == SINN)
		sinn_starts++;
	else if(which_method == NABE){
		recentLBDs.init(restart_min_confs);
		recentDLVs.init(restart_min_confs);
		recentBLVs.init(restart_min_confs);
	}

	if(aggressive_reduce_db == 2){
		disturb = true;
		propsDecMA.clear();
		propsDecMA.init(psMA_length);
	}

	for (;;){
//		XXX CONFLICT
		CRef confl = propagate();
		if (confl != CRef_Undef){
			// CONFLICT
			conflicts++; conflictC++; current_phase_confls++;
			if (decisionLevel() == 0) return l_False;
			if(aggressive_reduce_db == 2)
				disturb = true;

			learnt_clause.clear();

			analyze(confl, learnt_clause, backtrack_level, lbd);
			assert(lbd > 0 || learnt_clause.size() == 1);

			wholeLBDs += lbd;					// for GLUE
			wholeDLVs += decisionLevel();
			wholeBLVs += backtrack_level;

			if(which_method == NABE){
				recentLBDs.push(lbd);
				recentDLVs.push(decisionLevel());
				recentBLVs.push(backtrack_level);
			}

			cancelUntil(backtrack_level);

			assert(learnt_clause.size() < 65536);
			if (learnt_clause.size() == 1){
				uncheckedEnqueue(learnt_clause[0]);
			}else{
				CRef cr = ca.alloc(learnt_clause, true);
				learnts.push(cr);
				assert(lbd > 0);
				if(clause_importance != CLA_VANR)
					ca[cr].set_lbd(lbd);
				if(lbd > safe_high)
					ca[cr].isNotSafeHigh();
				attachClause(cr);
//				claBumpActivity(ca[cr]);
				uncheckedEnqueue(learnt_clause[0], cr);
			}

			if(phase_var_decay){
				if(which_method == SINN)
					varDecayActivitySinn();
				else
					varDecayActivityGlue();
			}
			else
				varDecayActivity();
			claDecayActivity();

			if (--learntsize_adjust_cnt == 0){
				learntsize_adjust_confl *= learntsize_adjust_inc;
				learntsize_adjust_cnt    = (int)learntsize_adjust_confl;
				max_learnts             *= learntsize_inc;

				if (verbosity >= 1)
					printf("c | %9d | %7d %8d %8d | %8d %8d %6.0f | %6.3f %% |\n",
							(int)conflicts,
							(int)dec_vars - (trail_lim.size() == 0 ? trail.size() : trail_lim[0]), nClauses(), (int)clauses_literals,
							(int)max_learnts, nLearnts(), (double)learnts_literals/nLearnts(), progressEstimate()*100);
			}

		}else{

			if(which_method == SINN){
				// NO CONFLICT
				if (nof_conflicts >= 0 && conflictC >= nof_conflicts || !withinBudget()){
					// Reached bound on number of conflicts:
					progress_estimate = progressEstimate();
					cancelUntil(0);
					return l_Undef; }
			}
			else if(which_method == NABE){
				bool restart = false;

				if((!glue_version || restart_set == RES_CONST) && recentLBDs.ready())
					restart = true;

				if (!withinBudget())
					restart = true;
				else if ((restart_set == RES_LBDBLV || restart_set == RES_LBDDLV || restart_set == RES_ALL || restart_set == RES_LBD) &&
						recentLBDs.ready() && recentLBDs.average() * lbd_restart_rate > wholeLBDs / conflicts)
					restart = true;
				else if ((restart_set == RES_DLVBLV || restart_set == RES_LBDDLV || restart_set == RES_ALL || restart_set == RES_DLV) &&
						recentDLVs.ready() && recentDLVs.average() * dlv_restart_rate > wholeDLVs / conflicts)
					restart = true;
				else if ((restart_set == RES_LBDBLV || restart_set == RES_DLVBLV || restart_set == RES_ALL || restart_set == RES_BLV) &&
						recentBLVs.ready() && recentBLVs.average() * blv_restart_rate > wholeBLVs / conflicts)
					restart = true;
				else if(current_phase_confls >= max_glue_conflicts)
					restart = true;
				if (restart) {
					progress_estimate = progressEstimate();
					cancelUntil(0);
					recentLBDs.clear();
					return l_Undef;
				}
			}

			// Simplify the set of problem clauses:
			if (decisionLevel() == 0 && !simplify())
				return l_False;

			if (aggressive_reduce_db == 0 && learnts.size()-nAssigns() >= max_learnts)
				// Reduce the set of learnt clauses:
				reduceDB();
			else if(aggressive_reduce_db == 1 && conflicts > reduce_db_limit){
            	if(clause_importance == CLA_VANR || clause_importance == CLA_LBDVANR){
            		// VANRの計算のためレベル0に戻る必要がある
    				progress_estimate = progressEstimate();
    				cancelUntil(0);
    				simplify();
            	}
				reduceDB();
				reduce_db_limit += reduce_db_base + reduce_db_inc * reduce_dbs;
			}
			else if(aggressive_reduce_db == 2 && conflicts > ps_reduce_db_limit
					&& propsDecMA.ready() && propsDecMA.average() * ps_rate < wholeprops/totaldecs){
				reduceDB();
				ps_reduce_db_inc += ps_reduce_db_inc_sub;
				ps_reduce_db_limit = conflicts + ps_reduce_db_inc;
				if(totaldecs > ps_init_length){
					wholeprops /= 10;
					totaldecs /= 10;
				}
				if(verbosity > 2)
					printf("c MAave : %f , TotalAve : %f\n", propsDecMA.average(), wholeprops/totaldecs);
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

			if (next == lit_Undef){
				// New variable decision:
				decisions++;
				next = pickBranchLit();

				if (next == lit_Undef)
					// Model found:
					return l_True;
			}

			if(aggressive_reduce_db == 2){
				if(disturb)
					disturb = false;
				else{
					wholeprops += trail.size() - trail_lim[decisionLevel()-1];
					totaldecs++;
					propsDecMA.push(trail.size() - trail_lim[decisionLevel()-1]);
				}
			}

			// Increase decision level and enqueue 'next'
			newDecisionLevel();
			uncheckedEnqueue(next);
		}
	}
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

static double luby(double y, int x,int power){

	// Find the finite subsequence that contains index 'x', and the
	// size of that subsequence:
	int size, seq;
	for (size = 1, seq = 0; size < x+1; seq++, size = 2*size+1);

	while (size-1 != x){
		size = (size-1)>>1;
		seq--;
		x = x % size;
	}

	if(power == LUBY_EXP)
		return pow(y, seq);
	else if(power == LUBY_SQU)
		return (seq == 0) ? 1 : pow(seq, y);
//	else if(power == LUBY_SQP)
	else
		return pow(seq+1, y);
}

// NOTE: assumptions passed in member-variable 'assumptions'.
// XXX solve
lbool Solver::solve_()
{
	model.clear();
	conflict.clear();
	if (!ok) return l_False;

	solves++;

	max_learnts               = nClauses() * learntsize_factor;
	learntsize_adjust_confl   = learntsize_adjust_start_confl;
	learntsize_adjust_cnt     = (int)learntsize_adjust_confl;
	lbool   status            = l_Undef;

	if(aggressive_reduce_db == 1 && max_learnts < reduce_db_base){
		reduce_db_base = reduce_db_limit = (max_learnts/2 < 5000) ? 5000 : (uint64_t)(max_learnts/2);
		reduce_db_inc /= 2;
	}
	if(aggressive_reduce_db == 2 && max_learnts < reduce_db_base){
		ps_reduce_db_limit = ps_reduce_db_inc_sub = (max_learnts/2 < 5000) ? 5000 : (uint64_t)(max_learnts/2);
	}

	if(movement_of_solver == 1)
		which_method = NABE;

	max_glue_conflicts = (max_glue_conflicts == 0) ? INT32_MAX : 50 * max_glue_conflicts;

	if(variety_of_luby > 0){
		luby_circle_key = 1;
		luby_circle_key_inc = 1;
		luby_circle_inc = 1;
	}

	if(use_appear == APP_LEARNT_PARAM || use_appear == APP_ALL_PARAM || use_appear == APP_BIN_PARAM)
		upper_num = (int)(nVars() * parameter_appear);
	else if(use_appear == APP_LEARNT_NUM || use_appear == APP_ALL_NUM || use_appear == APP_BIN_NUM)
		upper_num = number_appear;

	if(use_appear != APP_NONE)
		appearRank.init(nVars(), upper_num);

	if(use_appear == APP_ALL_PARAM || use_appear == APP_ALL_NUM){
		for(int i=0;i<nClauses();i++){
			Clause& c = ca[clauses[i]];
			for(int j=0;j<c.size();j++)
				appearRank.add(var(c[j]));
		}
	}

	if(use_appear == APP_BIN_PARAM || use_appear == APP_BIN_NUM){
		for(int i=0;i<nClauses();i++){
			Clause& c = ca[clauses[i]];
			if(c.size() <= 2){
				for(int j=0;j<c.size();j++)
					appearRank.add(var(c[j]));
			}
		}
	}

	if(use_appear != APP_NONE && verbosity > 2)
		printf("c upper_size is %d\n",appearRank.size());
//	appearRank.printALL();
//	exit(0);

	if (verbosity >= 1){
		printf("c ============================[ Search Statistics ]==============================\n");
		printf("c | Conflicts |          ORIGINAL         |          LEARNT          | Progress |\n");
		printf("c |           |    Vars  Clauses Literals |    Limit  Clauses Lit/Cl |          |\n");
		printf("c ===============================================================================\n");
	}

	// Search:
//	int curr_restarts = 0;
	while (status == l_Undef){
		if(sinn_starts == luby_circle_key){
			// ルビー関数のバリエーション
			sinn_starts += luby_circle_inc;
			luby_circle_num++;

			if(variety_of_luby == 1) luby_circle_inc++;		// 一巡ごとに飛ばす数が1増えていく
			else if(variety_of_luby == 2) luby_circle_inc += luby_circle_num;			// 一巡ごとに飛ばす数が回った数だけ増えていく
			else if(variety_of_luby == 3) luby_circle_inc *= 2;;			// その順の半分を飛ばす。

			luby_circle_key_inc *= 2;					// 次の周期に合わせる
			luby_circle_key += luby_circle_key_inc;		// 次の周期に合わせる
		}
//		double rest_base = luby_restart ? luby(restart_inc, curr_restarts) : pow(restart_inc, curr_restarts);
		double rest_base = luby_restart ? luby(restart_inc, sinn_starts, luby_power) : pow(restart_inc, sinn_starts);
		status = search((int)(rest_base * restart_first));

		if (!withinBudget()) break;
//		curr_restarts++;
	}

	if (verbosity >= 1)
		printf("c ===============================================================================\n");


	if (status == l_True){
		// Extend & copy model:
		model.growTo(nVars());
		for (int i = 0; i < nVars(); i++) model[i] = value(i);
	}else if (status == l_False && conflict.size() == 0)
		ok = false;

	cancelUntil(0);
	return status;
}

//=================================================================================================
// Writing CNF to DIMACS:
//
//  this needs to be rewritten completely.

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
		printf("c Wrote %d clauses with %d variables.\n", cnt, max);
}


//=================================================================================================
// Garbage Collection methods:

void Solver::relocAll(ClauseAllocator& to)
{
	// All watchers:
	//
	// for (int i = 0; i < watches.size(); i++)
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

	waitings.clear();
	// All learnt:
	//
	for (int i = 0; i < learnts.size(); i++){
		ca.reloc(learnts[i], to);
		if(!to[learnts[i]].attached())
			waitings.push(learnts[i]);
	}
	if(verbosity > 2)
		printf("c remake waiting : %d\n", waitings.size());

	// All original:
	//
	for (int i = 0; i < clauses.size(); i++)
		ca.reloc(clauses[i], to);
}


void Solver::garbageCollect()
{
	// Initialize the next region to a size corresponding to the estimated utilization degree. This
	// is not precise but should avoid some unnecessary reallocations for the new region:
	ClauseAllocator to(ca.size() - ca.wasted());

	relocAll(to);
	if (verbosity == 2)
		printf("c |  Garbage collection:   %12d bytes => %12d bytes             |\n",
				ca.size()*ClauseAllocator::Unit_Size, to.size()*ClauseAllocator::Unit_Size);
	to.moveTo(ca);
}

void Solver::printStatus(){
	double cpu_time = cpuTime();
	double mem_used = memUsedPeak();

	if(verbosity == 0){
		printf("	CPU_time	%g	", cpu_time);
		printf("restarts	%-12"PRIu64"	", starts);
		printf("conflicts	%-12"PRIu64"	", conflicts);
		printf("decisions	%-12"PRIu64"	", decisions);
		printf("propagations	%-12"PRIu64"	", propagations);
		printf("res/confl	%f	",(double)starts/conflicts);
		printf("confl/sec	%f	",(double)conflicts/cpu_time);
		printf("confl/dec	%f	",conflicts  /(double)decisions);
		printf("prop/sec	%.0f	", propagations/cpu_time);
		printf("prop/dec	%.2f	",propagations/(double)decisions);
		printf("chan/sec	%.0f	",tot_change_watch/cpu_time);
		printf("chan/dec	%.2f	",tot_change_watch/(double)decisions);
		printf("LearntSize	%.2f	",(double)learnts_literals/nLearnts());
//		if(lbd_measure > 0) printf("LBD/clause	%.2f	",(double)tot_lbds/conflicts);
		printf("conflict_literals	%-12"PRIu64"	", tot_literals);
		if (mem_used != 0) printf("Memory_used(MG)	%.2f	", mem_used);
		if (movement_of_solver == 2){
			printf("Phase	");
			if(which_method == SINN)
				printf("SINN");
			else
				printf("Uphase");
			printf("	");
		}
		printf("\n");
	}
	else{
		printf("c restarts              : %-12"PRIu64"   (%f / confl)\n", starts, (double)starts/conflicts);
		printf("c conflicts             : %-12"PRIu64"   (%.0f /sec)  (%f / dec) \n", conflicts   , conflicts   /cpu_time, conflicts/(double)decisions);
		printf("c decisions             : %-12"PRIu64"   (%4.2f %% random) (%.0f /sec)\n", decisions, (float)rnd_decisions*100 / (float)decisions, decisions   /cpu_time);
		printf("c propagations          : %-12"PRIu64"   (%.0f /sec) (%.2f / dec) \n", propagations, propagations/cpu_time, propagations/(double)decisions);
		printf("c change literals       : %-12"PRIu64"   (%.0f / sec) (%.2f / dec) \n",tot_change_watch, tot_change_watch/cpu_time, tot_change_watch/(double)decisions);
//		if(lbd_measure > 0) printf("LBD/clause            : %.2f\n",(double)tot_lbds/conflicts);
		printf("c conflict literals     : %-12"PRIu64"   (%4.2f %% deleted)\n", tot_literals, (max_literals - tot_literals)*100 / (double)max_literals);
		if (mem_used != 0) printf("Memory used           : %.2f MB\n", mem_used);
		printf("c CPU time              : %g s\n", cpu_time);
		if (movement_of_solver == 2){
			printf("c Phase                 : ");
			if(which_method == SINN)
				printf("SINN");
			else
				printf("Uphase");
			printf("\n");
		}
		printf("c \n");
	}
}


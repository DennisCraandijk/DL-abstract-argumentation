/*
 * Stat.cc
 *
 *  Created on: 2012/07/06
 *      Author: nabesima
 */
#include "Stat.h"
#include "utils/System.h"

using namespace Minisat;

void Minisat::printStats(Solver& s)
{
    double cpu_time = cpuTime();
    double mem_used = memUsedPeak();
    // modified by nabesima
    printf("variables         : %-12d   (init %d, after simp %d)\n", s.nFreeVars(), s.init_vars, s.simp_vars);
    printf("clauses           : %-12d   (init %d, after simp %d)\n", s.nClauses(), s.init_clauses, s.simp_clauses);
    //printf("restarts              : %"PRIu64"\n", solver.starts);
    printf("restarts          : %-12"PRIu64"   (%.2f confs/res, %"PRIu64" ~ %"PRIu64" confs, %"PRIu64" lbd, %"PRIu64" dec, %"PRIu64" bcp, blocked %"PRIu64")\n", s.starts - 1, (double)s.conflicts / s.starts, s.min_confs, s.max_confs, s.lbd_restarts, s.dec_restarts, s.bcp_restarts, s.blocked_restarts);
    printf("conflicts         : %-12"PRIu64"   (%.0f /sec)\n", s.conflicts, s.conflicts / cpu_time);
    printf("decisions         : %-12"PRIu64"   (%4.2f %% random, %.0f /sec, %4.2f %% deleted)\n", s.decisions, (float)s.rnd_decisions*100 / (float)s.decisions, s.decisions / cpu_time, (double)s.removed_decisions*100 / (s.decisions + s.removed_decisions));
    printf("propagations      : %-12"PRIu64"   (%.0f /sec, %4.2f %% deleted)\n", s.propagations, s.propagations/cpu_time, (double)s.removed_propagations*100 / (s.propagations + s.removed_propagations));
    printf("conflict literals : %-12"PRIu64"   (%4.2f %% deleted)\n", s.tot_literals, (s.max_literals - s.tot_literals)*100 / (double)s.max_literals);
    // added by nabesima
    printf("simplifications   : %-12d   (simp %d, rewrite %d)\n", s.simplify_dbs + s.rewrite_dbs, s.simplify_dbs, s.rewrite_dbs);
    printf("glue clauses      : %-12d   (%4.2f %%)\n", s.glue_clauses, (double)s.glue_clauses * 100 / s.conflicts);
    printf("avg lbd / avg len : %4.2f / %4.2f\n", (double)s.wholeLBDs / s.conflicts, (double)s.tot_literals / s.conflicts);
    printf("closed props      : %-12"PRIu64"   (%4.2f %% of propagations)\n", s.ind_propagations, (double)s.ind_propagations * 100 / s.propagations);
    printf("premise updates   : %-12"PRIu64"   (%4.2f /var)\n", s.premise_updates, (double)s.premise_updates / s.simp_vars);
    printf("probed variables  : %-12d   (%d chains, %d/%d false-lits, %d/%d polarity, %d eq-lits, %d/%d bin, %d/%d ss)\n", s.lazy_probed_lits[PRB_PREMISE_CHAIN] + s.lazy_probed_lits[PRB_FALSE_LIT] + s.lazy_probed_lits[PRB_POLALITY] + s.lazy_probed_lits[PRB_EQV_LIT] + s.lazy_probed_lits[PRB_BIN_CLAUSE] + s.lazy_probed_lits[PRB_SELF_SUBSUMP], s.lazy_probed_lits[PRB_PREMISE_CHAIN], s.lazy_probed_lits[PRB_FALSE_LIT], s.lazy_probed_lits[PRB_FALSE_LIT] + s.lazy_probed_dup_lits[PRB_FALSE_LIT], s.lazy_probed_lits[PRB_POLALITY], s.lazy_probed_lits[PRB_POLALITY] + s.lazy_probed_dup_lits[PRB_POLALITY], s.lazy_probed_lits[PRB_EQV_LIT], s.lazy_probed_lits[PRB_BIN_CLAUSE], s.lazy_probed_lits[PRB_BIN_CLAUSE] + s.lazy_probed_dup_lits[PRB_BIN_CLAUSE], s.lazy_probed_lits[PRB_SELF_SUBSUMP], s.lazy_probed_lits[PRB_SELF_SUBSUMP] + s.lazy_probed_dup_lits[PRB_SELF_SUBSUMP]);
    printf("removed cla lits  : %-12d   (%4.2f %% of %"PRIu64" deleted)\n", s.lazy_rm_cla_lits, (double)s.lazy_rm_cla_lits * 100 / s.core_clauses_literals, s.core_clauses_literals);
    printf("removed lrn lits  : %-12d   (%4.2f %% of %"PRIu64" deleted)\n", s.lazy_rm_lrn_lits, (double)s.lazy_rm_lrn_lits * 100 / s.tot_literals, s.tot_literals);
    printf("self-subsumptions : %-12d   (%d cla-lits, %d lrn-lits)\n", s.ss_rm_cla_lits + s.ss_rm_lrn_lits, s.ss_rm_cla_lits, s.ss_rm_lrn_lits);
    printf("bin subsumed      : %-12d   (%d clauses, %d learnts, %d lost ops)\n", s.lazy_subsumed_clauses + s.lazy_subsumed_leartns, s.lazy_subsumed_clauses, s.lazy_subsumed_leartns, s.lazy_lost_cops);
    printf("dup bin learnts   : %-12d\n", s.dup_bin_learnts);
    printf("skipped lits      : %-12d\n", s.lazy_skipped_lits);
    printf("pure lits         : %-12d\n", s.pure_lits);
    printf("removed fixed vars: %-12d\n", s.removed_fixed_vars);
    if (mem_used != 0)
        printf("Memory used       : %.2f MB\n", mem_used);
    printf("CPU time          : %g s\n", cpu_time);
}



/***********************************************************************************[SimpSolver.cc]
Copyright (c) 2006,      Niklas Een, Niklas Sorensson
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

#include "mtl/Sort.h"
#include "simp/SimpSolver.h"
#include "utils/System.h"

using namespace Minisat;

//=================================================================================================
// Options:

static const char* _cat = "SIMP";

static BoolOption   opt_use_asymm          (_cat, "asymm",        "Shrink clauses by asymmetric branching.", false);
static BoolOption   opt_use_rcheck         (_cat, "rcheck",       "Check if a clause is already implied. (costly)", false);
static BoolOption   opt_use_elim           (_cat, "elim",         "Perform variable elimination.", true);
static IntOption    opt_grow               (_cat, "grow",         "Allow a variable elimination step to grow by a number of clauses.", 0);
static IntOption    opt_clause_lim         (_cat, "cl-lim",       "Variables are not eliminated if it produces a resolvent with a length above this limit. -1 means no limit", 20,   IntRange(-1, INT32_MAX));
static IntOption    opt_subsumption_lim    (_cat, "sub-lim",      "Do not check if subsumption against a clause larger than this. -1 means no limit.", 1000, IntRange(-1, INT32_MAX));
static DoubleOption opt_simp_garbage_frac  (_cat, "simp-gc-frac", "The fraction of wasted memory allowed before a garbage collection is triggered during simplification.",  0.5, DoubleRange(0, false, HUGE_VAL, false));

static const char* _probing = "PROBING";
static BoolOption   opt_pol_probing        (_probing, "pol-probing", "Apply polarity probing", true);
static BoolOption   opt_eqv_probing        (_probing, "eqv-probing", "Apply equivalent variable probing", true);
static BoolOption   opt_bin_probing        (_probing, "bin-probing", "Apply binary clause probing", true);

//=================================================================================================
// Constructor/Destructor:


SimpSolver::SimpSolver() :
    parsing            (false)
  , grow               (opt_grow)
  , clause_lim         (opt_clause_lim)
  , subsumption_lim    (opt_subsumption_lim)
  , simp_garbage_frac  (opt_simp_garbage_frac)
  , use_asymm          (opt_use_asymm)
  , use_rcheck         (opt_use_rcheck)
  , use_elim           (opt_use_elim)

  // added by nabesima
  , pol_probing        (opt_pol_probing)
  , eqv_probing        (opt_eqv_probing)
  , bin_probing        (opt_bin_probing)

  , merges             (0)
  , asymm_lits         (0)
  , eliminated_vars    (0)
  , elimorder          (1)
  , use_simplification (true)
  , occurs             (ClauseDeleted(ca))
  , elim_heap          (ElimLt(n_occ))
  , bwdsub_assigns     (0)
  , n_touched          (0)
{
    vec<Lit> dummy(1,lit_Undef);
    ca.extra_clause_field = true; // NOTE: must happen before allocating the dummy clause below.
    bwdsub_tmpunit        = ca.alloc(dummy);
    remove_satisfied      = false;

    // added by nabesima
    if (bin_probing)
        prb_cla_lits.growTo(3);

}

SimpSolver::~SimpSolver()
{
}

Var SimpSolver::newVar(bool sign, bool dvar) {
    Var v = Solver::newVar(sign, dvar);

    frozen    .push((char)false);
    eliminated.push((char)false);

    if (use_simplification){
        n_occ     .push(0);
        n_occ     .push(0);
        occurs    .init(v);
        touched   .push(0);
        elim_heap .insert(v);
    }

    // added by nabesima
    if (eqv_probing && !lazy_eqv_probing)
        equivalent_lit.push(lit_Undef);

    return v;
}

lbool SimpSolver::solve_(bool do_simp, bool turn_off_simp)
{
    vec<Var> extra_frozen;
    lbool    result = l_True;

    do_simp &= use_simplification;

    if (do_simp){
        // Assumptions must be temporarily frozen to run variable elimination:
        for (int i = 0; i < assumptions.size(); i++){
            Var v = var(assumptions[i]);

            // If an assumption has been eliminated, remember it.
            assert(!isEliminated(v));

            if (!frozen[v]){
                // Freeze and store.
                setFrozen(v, true);
                extra_frozen.push(v);
            } }

        result = lbool(eliminate(turn_off_simp));
    }

    // added by nabesima
    simp_vars    = nFreeVars();
    simp_clauses = nClauses();

    if (result == l_True)
        result = Solver::solve_();
    else if (verbosity >= 1)
        printf("===============================================================================\n");

    if (result == l_True)
        extendModel();

    if (do_simp)
        // Unfreeze the assumptions that were frozen:
        for (int i = 0; i < extra_frozen.size(); i++)
            setFrozen(extra_frozen[i], false);

    return result;
}

bool SimpSolver::addClause_(vec<Lit>& ps)
{
#ifndef NDEBUG
    for (int i = 0; i < ps.size(); i++)
        assert(!isEliminated(var(ps[i])));
#endif

    int nclauses = clauses.size();

    if (use_rcheck && implied(ps))
        return true;

    if (!Solver::addClause_(ps))
        return false;

    // added by nabesima
    if(!parsing && rup)
        writeLits(ps);

    if (use_simplification && clauses.size() == nclauses + 1){
        CRef          cr = clauses.last();
        const Clause& c  = ca[cr];

        // NOTE: the clause is added to the queue immediately and then
        // again during 'gatherTouchedClauses()'. If nothing happens
        // in between, it will only be checked once. Otherwise, it may
        // be checked twice unnecessarily. This is an unfortunate
        // consequence of how backward subsumption is used to mimic
        // forward subsumption.
        subsumption_queue.insert(cr);
        for (int i = 0; i < c.size(); i++){
            occurs[var(c[i])].push(cr);
            n_occ[toInt(c[i])]++;
            touched[var(c[i])] = 1;
            n_touched++;
            if (elim_heap.inHeap(var(c[i])))
                elim_heap.increase(var(c[i]));
        }
    }

    return true;
}

void SimpSolver::removeClause(CRef cr)
{
    const Clause& c = ca[cr];

    if (use_simplification)
        for (int i = 0; i < c.size(); i++){
            n_occ[toInt(c[i])]--;
            updateElimHeap(var(c[i]));
            occurs.smudge(var(c[i]));
        }

    Solver::removeClause(cr);

    // added by nabesima
    if (rup) writeDeletedClause(ca[cr]);
}

bool SimpSolver::strengthenClause(CRef cr, Lit l)
{
    Clause& c = ca[cr];
    assert(decisionLevel() == 0);
    assert(use_simplification);

    // FIX: this is too inefficient but would be nice to have (properly implemented)
    // if (!find(subsumption_queue, &c))
    subsumption_queue.insert(cr);

    // added by nabesima
    if (rup) {
        assert(output != NULL);
        for (int i = 0; i < c.size(); i++)
            if (c[i] != l) fprintf(output, "%i " , (var(c[i]) + 1) * (-2 * sign(c[i]) + 1) );
        fprintf(output, "0\n");
    }

    if (c.size() == 2){
        removeClause(cr);
        c.strengthen(l);
    }else{

        // added by nabesima
        if (rup) writeDeletedClause(c);

        detachClause(cr, true);
        c.strengthen(l);
        attachClause(cr);
        remove(occurs[var(l)], cr);
        n_occ[toInt(l)]--;
        updateElimHeap(var(l));
    }

    return c.size() == 1 ? enqueue(c[0]) && propagate() == CRef_Undef : true;
}

// Returns FALSE if clause is always satisfied ('out_clause' should not be used).
bool SimpSolver::merge(const Clause& _ps, const Clause& _qs, Var v, vec<Lit>& out_clause)
{
    merges++;
    out_clause.clear();

    bool  ps_smallest = _ps.size() < _qs.size();
    const Clause& ps  =  ps_smallest ? _qs : _ps;
    const Clause& qs  =  ps_smallest ? _ps : _qs;

    for (int i = 0; i < qs.size(); i++){
        if (var(qs[i]) != v){
            for (int j = 0; j < ps.size(); j++)
                if (var(ps[j]) == var(qs[i]))
                    if (ps[j] == ~qs[i])
                        return false;
                    else
                        goto next;
            out_clause.push(qs[i]);
        }
        next:;
    }

    for (int i = 0; i < ps.size(); i++)
        if (var(ps[i]) != v)
            out_clause.push(ps[i]);

    return true;
}

// Returns FALSE if clause is always satisfied.
bool SimpSolver::merge(const Clause& _ps, const Clause& _qs, Var v, int& size)
{
    merges++;

    bool  ps_smallest = _ps.size() < _qs.size();
    const Clause& ps  =  ps_smallest ? _qs : _ps;
    const Clause& qs  =  ps_smallest ? _ps : _qs;
    const Lit*  __ps  = (const Lit*)ps;
    const Lit*  __qs  = (const Lit*)qs;

    size = ps.size()-1;

    for (int i = 0; i < qs.size(); i++){
        if (var(__qs[i]) != v){
            for (int j = 0; j < ps.size(); j++)
                if (var(__ps[j]) == var(__qs[i]))
                    if (__ps[j] == ~__qs[i])
                        return false;
                    else
                        goto next;
            size++;
        }
        next:;
    }

    return true;
}

void SimpSolver::gatherTouchedClauses()
{
    if (n_touched == 0) return;

    int i,j;
    for (i = j = 0; i < subsumption_queue.size(); i++)
        if (ca[subsumption_queue[i]].mark() == 0)
            ca[subsumption_queue[i]].mark(2);

    for (i = 0; i < touched.size(); i++)
        if (touched[i]){
            const vec<CRef>& cs = occurs.lookup(i);
            for (j = 0; j < cs.size(); j++)
                if (ca[cs[j]].mark() == 0){
                    subsumption_queue.insert(cs[j]);
                    ca[cs[j]].mark(2);
                }
            touched[i] = 0;
        }

    for (i = 0; i < subsumption_queue.size(); i++)
        if (ca[subsumption_queue[i]].mark() == 2)
            ca[subsumption_queue[i]].mark(0);

    n_touched = 0;
}

bool SimpSolver::implied(const vec<Lit>& c)
{
    assert(decisionLevel() == 0);

    trail_lim.push(trail.size());
    for (int i = 0; i < c.size(); i++)
        if (value(c[i]) == l_True){
            cancelUntil(0);
            return false;
        }else if (value(c[i]) != l_False){
            assert(value(c[i]) == l_Undef);
            uncheckedEnqueue(~c[i]);
        }

    bool result = propagate() != CRef_Undef;
    cancelUntil(0);
    return result;
}

// Backward subsumption + backward subsumption resolution
bool SimpSolver::backwardSubsumptionCheck(bool verbose)
{
    int cnt = 0;
    int subsumed = 0;
    int deleted_literals = 0;
    assert(decisionLevel() == 0);

    while (subsumption_queue.size() > 0 || bwdsub_assigns < trail.size()){

        // Empty subsumption queue and return immediately on user-interrupt:
        if (asynch_interrupt){
            subsumption_queue.clear();
            bwdsub_assigns = trail.size();
            break; }

        // Check top-level assignments by creating a dummy clause and placing it in the queue:
        if (subsumption_queue.size() == 0 && bwdsub_assigns < trail.size()){
            Lit l = trail[bwdsub_assigns++];
            ca[bwdsub_tmpunit][0] = l;
            ca[bwdsub_tmpunit].calcAbstraction();
            subsumption_queue.insert(bwdsub_tmpunit); }

        CRef    cr = subsumption_queue.peek(); subsumption_queue.pop();
        Clause& c  = ca[cr];

        if (c.mark()) continue;

        if (verbose && verbosity >= 2 && cnt++ % 1000 == 0)
            printf("subsumption left: %10d (%10d subsumed, %10d deleted literals)\r", subsumption_queue.size(), subsumed, deleted_literals);

        assert(c.size() > 1 || value(c[0]) == l_True);    // Unit-clauses should have been propagated before this point.

        // Find best variable to scan:
        Var best = var(c[0]);
        for (int i = 1; i < c.size(); i++)
            if (occurs[var(c[i])].size() < occurs[best].size())
                best = var(c[i]);

        // Search all candidates:
        vec<CRef>& _cs = occurs.lookup(best);
        CRef*       cs = (CRef*)_cs;

        for (int j = 0; j < _cs.size(); j++)
            if (c.mark())
                break;
            else if (!ca[cs[j]].mark() &&  cs[j] != cr && (subsumption_lim == -1 || ca[cs[j]].size() < subsumption_lim)){
                Lit l = c.subsumes(ca[cs[j]]);

                if (l == lit_Undef)
                    subsumed++, removeClause(cs[j]);
                else if (l != lit_Error){
                    deleted_literals++;

                    if (!strengthenClause(cs[j], ~l))
                        return false;

                    // Did current candidate get deleted from cs? Then check candidate at index j again:
                    if (var(l) == best)
                        j--;
                }
            }
    }

    return true;
}

bool SimpSolver::asymm(Var v, CRef cr)
{
    Clause& c = ca[cr];
    assert(decisionLevel() == 0);

    if (c.mark() || satisfied(c)) return true;

    trail_lim.push(trail.size());
    Lit l = lit_Undef;
    for (int i = 0; i < c.size(); i++)
        if (var(c[i]) != v && value(c[i]) != l_False)
            uncheckedEnqueue(~c[i]);
        else
            l = c[i];

    if (propagate() != CRef_Undef){
        cancelUntil(0);
        asymm_lits++;
        if (!strengthenClause(cr, l))
            return false;
    }else
        cancelUntil(0);

    return true;
}

bool SimpSolver::asymmVar(Var v)
{
    assert(use_simplification);

    const vec<CRef>& cls = occurs.lookup(v);

    if (value(v) != l_Undef || cls.size() == 0)
        return true;

    for (int i = 0; i < cls.size(); i++)
        if (!asymm(v, cls[i]))
            return false;

    return backwardSubsumptionCheck();
}

static void mkElimClause(vec<uint32_t>& elimclauses, Lit x)
{
    elimclauses.push(toInt(x));
    elimclauses.push(1);
}

static void mkElimClause(vec<uint32_t>& elimclauses, Var v, Clause& c)
{
    int first = elimclauses.size();
    int v_pos = -1;

    // Copy clause to elimclauses-vector. Remember position where the
    // variable 'v' occurs:
    for (int i = 0; i < c.size(); i++){
        elimclauses.push(toInt(c[i]));
        if (var(c[i]) == v)
            v_pos = i + first;
    }
    assert(v_pos != -1);

    // Swap the first literal with the 'v' literal, so that the literal
    // containing 'v' will occur first in the clause:
    uint32_t tmp = elimclauses[v_pos];
    elimclauses[v_pos] = elimclauses[first];
    elimclauses[first] = tmp;

    // Store the length of the clause last:
    elimclauses.push(c.size());
}

bool SimpSolver::eliminateVar(Var v)
{
    assert(!frozen[v]);
    assert(!isEliminated(v));
    assert(value(v) == l_Undef);

    // Split the occurrences into positive and negative:
    //
    const vec<CRef>& cls = occurs.lookup(v);
    vec<CRef>        pos, neg;
    for (int i = 0; i < cls.size(); i++)
        (find(ca[cls[i]], mkLit(v)) ? pos : neg).push(cls[i]);

    // Check wether the increase in number of clauses stays within the allowed ('grow'). Moreover, no
    // clause must exceed the limit on the maximal clause size (if it is set):
    //
    int cnt         = 0;
    int clause_size = 0;

    for (int i = 0; i < pos.size(); i++)
        for (int j = 0; j < neg.size(); j++)
            if (merge(ca[pos[i]], ca[neg[j]], v, clause_size) &&
                (++cnt > cls.size() + grow || (clause_lim != -1 && clause_size > clause_lim)))
                return true;

    // Delete and store old clauses:
    eliminated[v] = true;
    setDecisionVar(v, false);
    eliminated_vars++;

    if (pos.size() > neg.size()){
        for (int i = 0; i < neg.size(); i++)
            mkElimClause(elimclauses, v, ca[neg[i]]);
        mkElimClause(elimclauses, mkLit(v));
    }else{
        for (int i = 0; i < pos.size(); i++)
            mkElimClause(elimclauses, v, ca[pos[i]]);
        mkElimClause(elimclauses, ~mkLit(v));
    }

    // modified by nabesima (to make same behaviour as minisat)
    if (!rup)
    for (int i = 0; i < cls.size(); i++)
        removeClause(cls[i]);

    // Produce clauses in cross product:
    vec<Lit>& resolvent = add_tmp;
    for (int i = 0; i < pos.size(); i++)
        for (int j = 0; j < neg.size(); j++)
            if (merge(ca[pos[i]], ca[neg[j]], v, resolvent) && !addClause_(resolvent))
                return false;

    // added by nabesima
    if (rup)
        for (int i = 0; i < cls.size(); i++)
            removeClause(cls[i]);

    // Free occurs list for this variable:
    occurs[v].clear(true);

    // Free watchers lists for this variable, if possible:
    if (watches[ mkLit(v)].size() == 0) watches[ mkLit(v)].clear(true);
    if (watches[~mkLit(v)].size() == 0) watches[~mkLit(v)].clear(true);

    return backwardSubsumptionCheck();
}

bool SimpSolver::substitute(Var v, Lit x)
{
    assert(!frozen[v]);
    assert(!isEliminated(v));
    assert(value(v) == l_Undef);

    if (!ok) return false;

    eliminated[v] = true;
    setDecisionVar(v, false);
    const vec<CRef>& cls = occurs.lookup(v);

    vec<Lit>& subst_clause = add_tmp;
    for (int i = 0; i < cls.size(); i++){
        Clause& c = ca[cls[i]];

        subst_clause.clear();
        for (int j = 0; j < c.size(); j++){
            Lit p = c[j];
            subst_clause.push(var(p) == v ? x ^ sign(p) : p);
        }

        // modified by nabesima (to make same behaviour as minisat)
        if (!rup) removeClause(cls[i]);

        if (!addClause_(subst_clause))
            return ok = false;

        // added by nabesima
        if (rup) removeClause(cls[i]);
    }

    return true;
}

void SimpSolver::extendModel()
{
    int i, j;
    Lit x;

    for (i = elimclauses.size()-1; i > 0; i -= j){
        for (j = elimclauses[i--]; j > 1; j--, i--)
            if (modelValue(toLit(elimclauses[i])) != l_False)
                goto next;

        x = toLit(elimclauses[i]);
        model[var(x)] = lbool(!sign(x));
    next:;
    }
}

bool SimpSolver::eliminate(bool turn_off_elim)
{
    // added by nabesima
    if (init_vars == 0 && init_clauses == 0) {
        init_vars    = simp_vars    = nVars();
        init_clauses = simp_clauses = nClauses();
    }

    if (!simplify())
        return false;
    else if (!use_simplification)
        return true;

    // Main simplification loop:
    //
    while (n_touched > 0 || bwdsub_assigns < trail.size() || elim_heap.size() > 0){

        gatherTouchedClauses();
        // printf("  ## (time = %6.2f s) BWD-SUB: queue = %d, trail = %d\n", cpuTime(), subsumption_queue.size(), trail.size() - bwdsub_assigns);
        if ((subsumption_queue.size() > 0 || bwdsub_assigns < trail.size()) &&
            !backwardSubsumptionCheck(true)){
            ok = false; goto cleanup; }

        // Empty elim_heap and return immediately on user-interrupt:
        if (asynch_interrupt){
            assert(bwdsub_assigns == trail.size());
            assert(subsumption_queue.size() == 0);
            assert(n_touched == 0);
            elim_heap.clear();
            goto cleanup; }

        // printf("  ## (time = %6.2f s) ELIM: vars = %d\n", cpuTime(), elim_heap.size());
        for (int cnt = 0; !elim_heap.empty(); cnt++){
            Var elim = elim_heap.removeMin();

            if (asynch_interrupt) break;

            if (isEliminated(elim) || value(elim) != l_Undef) continue;

            if (verbosity >= 2 && cnt % 100 == 0)
                printf("elimination left: %10d\r", elim_heap.size());

            if (use_asymm){
                // Temporarily freeze variable. Otherwise, it would immediately end up on the queue again:
                bool was_frozen = frozen[elim];
                frozen[elim] = true;
                if (!asymmVar(elim)){
                    ok = false; goto cleanup; }
                frozen[elim] = was_frozen; }

            // At this point, the variable may have been set by assymetric branching, so check it
            // again. Also, don't eliminate frozen variables:
            if (use_elim && value(elim) == l_Undef && !frozen[elim] && !eliminateVar(elim)){
                ok = false; goto cleanup; }

            checkGarbage(simp_garbage_frac);
        }

        assert(subsumption_queue.size() == 0);
    }

 cleanup:

    // If no more simplification is needed, free all simplification-related data structures:
    if (turn_off_elim){
        touched  .clear(true);
        occurs   .clear(true);
        n_occ    .clear(true);
        elim_heap.clear(true);
        subsumption_queue.clear(true);

        use_simplification    = false;
        remove_satisfied      = true;
        ca.extra_clause_field = false;

        // Force full cleanup (this is safe and desirable since it only happens once):
        rebuildOrderHeap();
        garbageCollect();
    }else{
        // Cheaper cleanup:
        cleanUpClauses(); // TODO: can we make 'cleanUpClauses()' not be linear in the problem size somehow?
        checkGarbage();
    }

    if (verbosity >= 1 && elimclauses.size() > 0)
        printf("|  Eliminated clauses:     %10.2f Mb (%12d remains)                 |\n",
               double(elimclauses.size() * sizeof(uint32_t)) / (1024*1024), clauses.size());

    // added by nabesima
    if (!lazySimplify())
        return false;
    if (pol_probing || eqv_probing || bin_probing)
        if (!probe())
            return ok = false;
    if (!simplifyClauses())
        return ok = false;

    return ok;
}

void SimpSolver::cleanUpClauses()
{
    occurs.cleanAll();
    int i,j;
    for (i = j = 0; i < clauses.size(); i++)
        if (ca[clauses[i]].mark() == 0)
            clauses[j++] = clauses[i];
    clauses.shrink(i - j);
}


//=================================================================================================
// Garbage Collection methods:

void SimpSolver::relocAll(ClauseAllocator& to)
{
    if (!use_simplification) return;

    // All occurs lists:
    //
    for (int i = 0; i < nVars(); i++){
        vec<CRef>& cs = occurs[i];
        for (int j = 0; j < cs.size(); j++)
            ca.reloc(cs[j], to);
    }

    // Subsumption queue:
    //
    for (int i = 0; i < subsumption_queue.size(); i++)
        ca.reloc(subsumption_queue[i], to);

    // Temporary clause:
    //
    ca.reloc(bwdsub_tmpunit, to);
}

void SimpSolver::garbageCollect()
{
    // Initialize the next region to a size corresponding to the estimated utilization degree. This
    // is not precise but should avoid some unnecessary reallocations for the new region:
    ClauseAllocator to(ca.size() - ca.wasted());

    cleanUpClauses();
    to.extra_clause_field = ca.extra_clause_field; // NOTE: this is important to keep (or lose) the extra fields.
    relocAll(to);
    Solver::relocAll(to);
    if (verbosity >= 2)
        printf("|  Garbage collection:   %12d bytes => %12d bytes                     |\n",
               ca.size()*ClauseAllocator::Unit_Size, to.size()*ClauseAllocator::Unit_Size);
    to.moveTo(ca);
}

//
// added by nabesima
//
bool SimpSolver::simplifyClauses()
{
    if (!rm_fixed_vars) return simplify();

    assert(decisionLevel() == 0);
    if (!ok || propagate() != CRef_Undef)
        return false;

     // Rewrite equivalent literals with minimal ones.
    if (!rewriteClauses())
        return false;

    // Remove satisfied clauses:
    int org_free_vars = -1;
    while (org_free_vars != nFreeVars()) {
        org_free_vars = nFreeVars();
        detachAllClauses();
        vec<Lit> ps;
        int i, j;
        for (i = j = 0; i < clauses.size(); i++) {
            Clause& c = ca[clauses[i]];
            if (rup)
                rup_tmp.clear();
            int  k, l;
            for (k = l = 0; k < c.size(); k++) {
                Lit p = c[k];
                if (rup)
                    rup_tmp.push(p);
                if (value(p) == l_True)
                    break;
                if (value(p) == l_False) {
                    removed_fixed_vars++;
                    continue;
                }
                c[l++] = p;
            }
            if (k < c.size()) {
                Solver::removeClause(clauses[i]);
                if (rup) {
                    while (++k < c.size())
                        rup_tmp.push(c[k]);
                    writeDeletedLits(rup_tmp);
                }
                continue;
            }
            if (l == 1) {
                uncheckedEnqueue(c[0]);
                if (rup) {
                    writeLit(c[0]);
                    writeDeletedLits(ps);
                }
                Solver::removeClause(clauses[i]);
                continue;
            }
            c.shrink(k - l);
            clauses[j++] = clauses[i];
            if (rup && k > l) {
                writeClause(c);
                writeDeletedLits(ps);
            }
        }
        clauses.shrink(i - j);
        attachAllClauses();

        // Weak pure literal elimination
        if (pure_lit_elim) {
            vec<int> num_lits(nVars() * 2);
            for (int i=0; i < clauses.size(); i++) {
                Clause& c = ca[clauses[i]];
                for (int j=0; j < c.size(); j++)
                    num_lits[toInt(c[j])]++;
            }
            for (int v=0; v < nVars(); v++) {
                Lit p = mkLit(v, false);
                if (value(p) != l_Undef)
                    continue;
                if (!decision[v])
                    continue;
                if (num_lits[toInt(p)] == 0 && num_lits[toInt(~p)] == 0)
                    continue;
                assert(!isEliminated(v));
                assert(equivalent_lit.size() == 0 || equivalent_lit[v] == lit_Undef);
                if (num_lits[toInt(p)] == 0) {
                    polarity[v] = false;    // NOTE: if use uncheckedEnqueue(~p), then sometimes output invalid model.
                    pure_lits++;
                }
                else if (num_lits[toInt(~p)] == 0) {
                    polarity[v] = true;    // NOTE: if use uncheckedEnqueue(p), then sometimes output invalid model.
                    pure_lits++;
                }
            }
        }

        if (propagate() != CRef_Undef)
            return ok = false;
    }

    checkGarbage();
    rebuildOrderHeap();
    return true;
}

// Determinates values of variables by probing
bool SimpSolver::probe()
{
    vec<bool> tested(nVars());
    while (true) {

        if (asynch_interrupt) break;

        // Collects variables which are contained in binary clauses with both polarities.
        vec<bool> cands(nVars() * 2);
        vec<Lit>  undef;
        for (int i=0; i < clauses.size(); i++) {
            Clause &c = ca[clauses[i]];
            undef.clear();
            for (int j=0; j < c.size(); j++) {
                if (value(c[j]) == l_False) continue;
                if (value(c[j]) == l_True || undef.size() >= 2) {
                    undef.clear();
                    break;
                }
                undef.push(c[j]);
            }
            for (int j=0; j < undef.size(); j++)
                cands[toInt(undef[j])] = true;
        }

        vec<Var> vars;
        for (int i=0; i < nVars(); i++) {
            Lit lit = mkLit(i, false);
            if (cands[toInt(lit)] && cands[toInt(~lit)] && !tested[var(lit)]) {
                vars.push(var(lit));
                tested[var(lit)] = true;  // heuristics
            }
        }

        int org_num_vars = nFreeVars();
        if (verbosity >= 1)
            printf("|  Probing:              %12d vars                                      |\n", vars.size());
        if (!probeVars(vars))
            return false;
        if (nFreeVars() == org_num_vars)
            break;

        int org_num_clauses = clauses.size();
        simpDB_props = 0;
        simplify();
        if (!lazySimplify())
            return false;
        if (verbosity >= 1)
            printf("|  Eliminated:           %12d vars, %7d clauses                     |\n",
                    org_num_vars - nFreeVars(), org_num_clauses - clauses.size());

            if (prb_equiv_vars > 0) {
                removeEqChains();
                rewriteClauses();
                if (verbosity >= 1)
                    printf("|  Equivalent vars:      %12d vars                                      |\n", prb_equiv_vars);
                prb_equiv_vars = 0;
            }
    }

    return true;
}

bool SimpSolver::probeVars(vec<Var>& vars)
{
    vec<bool> occ(nVars() * 2);
    vec<int>  occ_idx;
    vec<Lit>  decided;
    for (int i=0; i < vars.size(); i++) {

        if (asynch_interrupt) break;

        Var v = vars[i];
        if (value(v) != l_Undef) continue;
        Lit neg = mkLit(v, true);
        Lit pos = ~neg;
        probings++;

        // Tests the false assignment
        newDecisionLevel();
        uncheckedEnqueue(neg);
        if (propagate() != CRef_Undef) {
            cancelUntil(0);
            uncheckedEnqueue(pos);
            prb_false_lits++;
            if (rup) writeLit(pos);
            if (propagate() != CRef_Undef)
                return false;
            continue;
        }

        // Records implied variables
        for (int j=0; j < occ_idx.size(); j++) occ[occ_idx[j]] = false;
        occ_idx.clear();
        for (int j=trail_lim[0] + 1; j < trail.size(); j++) {
            occ_idx.push(toInt(trail[j]));
            occ[toInt(trail[j])] = true;
        }
        cancelUntil(0);
        if (!lazySimplify()) return false;

        if (bin_probing)
            if (!probeBinClauses(neg, occ))
                return false;

        // Tests the true assignment
        if (value(pos) != l_Undef) continue;
        newDecisionLevel();
        uncheckedEnqueue(pos);
        if (propagate() != CRef_Undef) {
            cancelUntil(0);
            uncheckedEnqueue(neg);
            prb_false_lits++;
            if (rup) writeLit(neg);
            if (propagate() != CRef_Undef)
                return false;
            continue;
        }

        // Applies probed results.
        decided.clear();
        for (int j=trail_lim[0] + 1; j < trail.size(); j++) {
            if (occ[toInt(trail[j])])
                decided.push(trail[j]);
            else if (eqv_probing && occ[toInt(~trail[j])])
                putEqLit(trail[j], pos);
        }

        // Records implied variables
        for (int j=0; j < occ_idx.size(); j++) occ[occ_idx[j]] = false;
        occ_idx.clear();
        for (int j=trail_lim[0] + 1; j < trail.size(); j++) {
            occ_idx.push(toInt(trail[j]));
            occ[toInt(trail[j])] = true;
        }
        cancelUntil(0);
        if (!lazySimplify()) return false;

        if (decided.size() > 0) {
            for (int j=0; j < decided.size(); j++) {
                if (value(decided[j]) != l_Undef)
                    continue;
                uncheckedEnqueue(decided[j]);
                prb_polarity_lits++;
                if (rup) {
                    writeLits(~pos, decided[j]);
                    writeLits(~neg, decided[j]);
                    writeLit(decided[j]);
                }
            }
            if (propagate() != CRef_Undef)
                return false;
        }
        if (bin_probing)
            if (!probeBinClauses(pos, occ))
                return false;
    }

    return lazySimplify();
}

bool SimpSolver::probeBinClauses(Lit l, vec<bool>& occ)
{
    if (value(l) != l_Undef) return true;

    vec<Lit> undef;
    vec<Lit> decided;
    vec<Watcher>& ws = watches[~l];
    for (int i=0; i < ws.size(); i++) {
        Watcher& w = ws[i];
        // Try to avoid inspecting the clause:
        Lit blocker = w.blocker;
        if (value(blocker) == l_True)
            continue;

        Clause& c = ca[w.cref];
        // Finds bin clauses
        undef.clear();
        for (int j=0; j < c.size(); j++) {
            if (value(c[j]) == l_False) continue;
            if (value(c[j]) == l_True || undef.size() >= 2) {
                undef.clear();
                break;
            }
            undef.push(c[j]);
        }
        if (undef.size() == 0) continue;
        if (undef.size() == 1) {
            uncheckedEnqueue(undef[0]);
            if (rup) writeLit(undef[0]);
            if (propagate() != CRef_Undef)
                return ok = false;
            continue;
        }
        assert(undef.size() == 2);
        assert(l == undef[0] || l == undef[1]);
        Lit other = (undef[0] == l) ? undef[1] : undef[0];

        if (toInt(var(l)) > toInt(var(other))) continue;
        probings++;

        // Tests the assignment for other
        newDecisionLevel();
        uncheckedEnqueue(other);
        if (propagate() != CRef_Undef) {
            cancelUntil(0);
            uncheckedEnqueue(~other);
            prb_false_lits++;
            if (rup) writeLit(~other);
            if (propagate() != CRef_Undef)
                return ok = false;
            continue;
        }

        // Applies probed results.
        decided.clear();
        for (int j=trail_lim[0] + 1; j < trail.size(); j++) {
            if (occ[toInt(trail[j])])
                decided.push(trail[j]);
        }
        cancelUntil(0);
        if (decided.size() > 0) {
            for (int j=0; j < decided.size(); j++) {
                uncheckedEnqueue(decided[j]);
                prb_cla_lits[2]++;
                prb_cla_total_lits++;
                if (rup) {
                    writeLits(~l, decided[j]);
                    writeLits(~other, decided[j]);
                    writeLit(decided[j]);
                }
            }
            if (propagate() != CRef_Undef)
                return ok = false;
        }
    }
    return true;
}

bool SimpSolver::putEqLit(Lit p, Lit q) {
    assert(eqv_probing);
    assert(equivalent_lit.size() > 0);
    while (true) {
        assert(p != q);
        assert(p != lit_Undef && q != lit_Undef);
        // UNSAT?
        if (p == ~q) return false;
        if (var(p) < var(q)) {
            Lit tmp = p;
            p = q;
            q = tmp;
        }
        if (equivalent_lit[var(p)] == lit_Undef) {
            equivalent_lit[var(p)] = sign(p) ? ~q : q;
            num_equivalent_lits++;
            prb_equiv_vars++;
            if (rup) {
                Lit pp = mkLit(var(p), false);
                writeLits(~pp,  equivalent_lit[var(pp)]);
                writeLits( pp, ~equivalent_lit[var(pp)]);
            }
            return true;
        }
        Lit r = sign(p) ? ~equivalent_lit[var(p)] : equivalent_lit[var(p)];
        if (r == q)
            return true;
        p = r;
    }
    assert(false);
    return true;
}
bool SimpSolver::removeEqChains() {
    assert(decisionLevel() == 0);
    // Remove chains.
    for (int i=0; i < equivalent_lit.size(); i++) {
        if (equivalent_lit[i] == lit_Undef) continue;
        Lit p = equivalent_lit[i];
        while (equivalent_lit[var(p)] != lit_Undef)
            p = sign(p) ? ~equivalent_lit[var(p)] : equivalent_lit[var(p)];
        equivalent_lit[i] = p;
    }
    // Assigns equivalent literals if one of those has value.
    for (int i=0; i < equivalent_lit.size(); i++) {
        if (equivalent_lit[i] == lit_Undef) continue;
        setDecisionVar(i, false);
        // If src or dst have a value, then propagate it to the unassinged side.
        if (value(i) != value(equivalent_lit[i])) {
            if (value(i) == l_Undef)
                uncheckedEnqueue(value(equivalent_lit[i]) == l_True ? mkLit(i, false) : mkLit(i, true));
            else if (value(equivalent_lit[i]) == l_Undef)
                uncheckedEnqueue(value(i) == l_True ? equivalent_lit[i] : ~equivalent_lit[i]);
            else
                return ok = false;
        }
    }
    return true;
}


/* MARCH Satisfiability Solver

   Copyright (C) 2001-2009 M.J.H. Heule, J.E. van Zwieten, and M. Dufour.
   [marijn@heule.nl, jevanzwieten@gmail.com, mark.dufour@gmail.com]

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA

*/

//#define ADD_CONFLICT
//#define BACKJUMP

//#define LOCAL_AUTARKY
//#define DETECT_COMPONENTS
#define COMPENSATION_RESOLVENTS

#define SAT_INCREASE	1000

#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>

#include "common.h"
#include "distribution.h"
#include "solver.h"
#include "equivalence.h"
#include "preselect.h"
#include "memory.h"
#include "lookahead.h"
#include "progressBar.h"
#include "parser.h"
//#include "localbranch.h"

#define CONTINUE	1
#define FINISHED	0

#define IS_REDUCED_TIMP( __a, __b )	((timeAssignments[__a] < NARY_MAX) && (timeAssignments[__b] == (NARY_MAX+1)) )

#ifdef DISTRIBUTION
  #define LEFT_CHILD	 records[record_index].child[branch_literal > 0]
  #define RIGHT_CHILD	 records[record_index].child[branch_literal < 0]
#endif

#ifdef SUPER_LINEAR
int sl_depth;
#endif

int *tmpEqImpSize;

int *TernaryImpTable, *TernaryImpLast;
int current_bImp_stamp, *bImp_stamps;

int *newbistack, *newbistackp, newbistackSize;
int *substack,   *substackp,   substackSize;

int analyze_autarky();

int nrofforced;

unsigned long long solution_bin = 0;
unsigned int solution_bits = 63;

#ifdef DISTRIBUTION
float first_time;
int skip_flag = 0;
int first_depth = 20;
#endif
#ifdef SUBTREE_SIZE
int path_length;
#endif
#ifdef PLOT
int plotCount = 0;
#endif
#ifdef CUT_OFF
int last_SAT_bin = -1;
#endif
#ifdef BACKJUMP
int backjump_literal = 0;
#endif

int currentNodeNumber = 1; 
int UNSATflag = 0;

#define STAMP_IMPLICATIONS( _nrval ) \
{ \
    bImp = BIMP_START(_nrval); \
    current_bImp_stamp++; \
    bImp_stamps[ _nrval ] = current_bImp_stamp; \
    for (i = BIMP_ELEMENTS; --i; ) \
	bImp_stamps[ *(bImp++) ] = current_bImp_stamp; \
}

inline void push_stack_blocks( )
{ 
	PUSH( r, STACK_BLOCK ); 
	PUSH( imp, STACK_BLOCK ); 
        PUSH( bieq, STACK_BLOCK ); 
        PUSH( subsume, STACK_BLOCK ); 
	current_node_stamp++;
}

#ifdef PROGRESS_BAR
#define NODE_START( ) \
{ \
	push_stack_blocks( ); \
	pb_descend( ); \
}
#define NODE_END( ) \
{ \
	backtrack(); \
	pb_climb(); \
}
#else
#define NODE_START( ) \
{ \
	push_stack_blocks( ); \
}
#define NODE_END( ) \
{ \
	backtrack(); \
}
#endif \

inline int recursive_solve( ) 
{ 
	int _result = 0;

	depth++; 
	PUSH( r, STACK_BLOCK ); 
	_result = march_solve_rec(); 
	depth--;

	return _result;
}

#define UPDATE_BIN( ) \
{\
    if( depth <= solution_bits ) solution_bin ^= (unsigned long long) 1 << (solution_bits - depth); \
}

/*
	----------------------------------------------------
	------------[ initializing and freeing ]------------
	----------------------------------------------------
*/
void clearTernaryImpReduction( )
{
	int i;
	for( i = 1; i <= nrofvars; i++ )
	{
	    TernaryImpReduction[  i ] = 0;
	    TernaryImpReduction[ -i ] = 0;
	}
}

inline void fill_ternary_implication_arrays( )
{
	int i, j, lit;

        for( i = 0; i < nrofclauses; i++ )
	    if( Clength[ i ] == 3 )
	    {
                for( j = 0; j < 3; j++ )
                {
                   lit = Cv[i][j];
                   TernaryImp[lit][ tmpTernaryImpSize[lit]++ ] = Cv[i][(j+1)%3 ];
                   TernaryImp[lit][ tmpTernaryImpSize[lit]++ ] = Cv[i][(j+2)%3 ];
                }
	    }

        for( i = -nrofvars; i <= nrofvars; i++ )
	    TernaryImpSize[ i ] = tmpTernaryImpSize[ i ] / 2;	    
}

int initSolver()
{
	int i, j, _tmp;

	/* initialise global counters */

	current_node_stamp = 1;
	lookDead 	   = 0;
	mainDead 	   = 0;
#ifdef COUNT_SAT
	count_sat	   = 0;
#endif
       solution_bin = 0;
       solution_bits = 63;
#ifdef DISTRIBUTION
       first_time = 0;
       skip_flag = 0;
       first_depth = 20;
#endif
       currentNodeNumber = 1;
       UNSATflag = 0;

	/* allocate recursion stack */
	/* tree is max. nrofvars deep and we thus have max. nrofvars STACK_BLOCKS
		 -> 2 * nrofvars should be enough for everyone :)
	*/
	INIT_ARRAY( r       , 3 * nrofvars + 1   );

	INIT_ARRAY( imp     , INITIAL_ARRAY_SIZE );
	INIT_ARRAY( subsume , INITIAL_ARRAY_SIZE );
	INIT_ARRAY( bieq    , INITIAL_ARRAY_SIZE );
	INIT_ARRAY( newbi   , INITIAL_ARRAY_SIZE );
	INIT_ARRAY( sub     , INITIAL_ARRAY_SIZE );

	MALLOC_OFFSET( bImp_satisfied, int, nrofvars, 2 );
	MALLOC_OFFSET( bImp_start,     int, nrofvars, 2 );
	MALLOC_OFFSET( bImp_stamps,    int, nrofvars, 0 );
	MALLOC_OFFSET( node_stamps, tstamp, nrofvars, 0 );

	tmpEqImpSize  = (int*) malloc( sizeof(int) * (nrofvars+1) );

	init_lookahead();
	init_preselection();
#ifdef DISTRIBUTION
	init_direction();
#endif
	tmpTernaryImpSize = (int* ) malloc( sizeof(int ) * ( 2*nrofvars+1 ) );
#ifdef TERNARYLOOK
        TernaryImp 	  = (int**) malloc( sizeof(int*) * ( 2*nrofvars+1 ) );
        TernaryImpSize 	  = (int* ) malloc( sizeof(int ) * ( 2*nrofvars+1 ) );

        for( i = 0; i <= 2 * nrofvars; i++ )
	{
	    tmpTernaryImpSize[ i ] = 0;
	    TernaryImpSize   [ i ] = 0;
	}

	for( i = 0; i < nrofclauses; i++ )
	    if( Clength[ i ] == 3 )
		for( j = 0; j < 3; j++ )
		    TernaryImpSize[ Cv[ i ][ j ] + nrofvars ]++;

        for( i = 0; i <= 2 * nrofvars; i++ )
	    TernaryImp[ i ] = (int*) malloc(sizeof(int)*(4*TernaryImpSize[i]+4));

        tmpTernaryImpSize 	+= nrofvars;
        TernaryImp 		+= nrofvars;
	TernaryImpSize 		+= nrofvars;

	fill_ternary_implication_arrays();

        for( i = -nrofvars; i <= nrofvars; i++ )
//	    tmpTernaryImpSize[ i ] = 2 * tmpTernaryImpSize[ i ] + 2;	
	    tmpTernaryImpSize[ i ] = 4 * TernaryImpSize[ i ] + 2;	

//	branch_on_dummies_from_long_clauses();

	while( AddTernaryResolvents() );

        for( i = -nrofvars; i <= nrofvars; i++ )
	    free( TernaryImp[ i ] );

	FREE_OFFSET( TernaryImp     );
	FREE_OFFSET( TernaryImpSize );
#else
        tmpTernaryImpSize 	+= nrofvars;
#endif
	/* initialise global datastructures */

#ifdef GLOBAL_AUTARKY
	MALLOC_OFFSET( TernaryImpReduction, int, nrofvars, 0 );

	if( kSAT_flag )
	{
	    int _nrofliterals = 0;

	    for( i = 0; i < nrofbigclauses; ++i )
		_nrofliterals += clause_length[ i ];

	    clause_reduction = (int*)  malloc( sizeof(int ) * nrofbigclauses );
	    clause_red_depth = (int*)  malloc( sizeof(int ) * nrofbigclauses );
	    big_global_table = (int*)  malloc( sizeof(int ) * _nrofliterals );
	    clause_SAT_flag  = (int*)  malloc( sizeof(int ) * nrofbigclauses );

	    MALLOC_OFFSET( big_to_binary, int*, nrofvars, NULL );
	    MALLOC_OFFSET( btb_size,      int , nrofvars,    0 );

	    for( i = 0; i < nrofbigclauses; ++i )
	    {
		clause_reduction[ i ] =  0;
		clause_red_depth[ i ] =  nrofvars;
		clause_SAT_flag [ i ] =  0;
	    }

	    int tmp = 0;
            for( i = 1; i <= nrofvars; i++ )
            {
                big_to_binary[  i ] = (int*) &big_global_table[ tmp ];
                tmp += big_occ[  i ];

                big_to_binary[ -i ] = (int*) &big_global_table[ tmp ];
                tmp += big_occ[ -i ];

            }
	    assert( tmp == _nrofliterals );
	}
#endif
        TernaryImp 	= (int**) malloc( sizeof(int*) * ( 2*nrofvars+1 ) );
        TernaryImpSize 	= (int* ) malloc( sizeof(int ) * ( 2*nrofvars+1 ) );
        TernaryImpLast 	= (int* ) malloc( sizeof(int ) * ( 2*nrofvars+1 ) );
        TernaryImpTable	= (int* ) malloc( sizeof(int ) * ( 6*nrofclauses+1 ) );

	TernaryImp	       += nrofvars;
	TernaryImpSize	       += nrofvars;
	TernaryImpLast	       += nrofvars;

	if( simplify_formula() == UNSAT ) return UNSAT;

	for( i = -nrofvars; i <= nrofvars; i++ )
	{
	    tmpTernaryImpSize[ i ] = 0;
	    TernaryImpSize   [ i ] = 0;
	    bImp_satisfied   [ i ] = 2;	//waarom staat dit hier?
	}

	for( i = 0; i < nrofclauses; i++ )
	    if( Clength[ i ] == 3 )
		for( j = 0; j < 3; j++ )
		    TernaryImpSize[ Cv[ i ][ j ] ]++;

	_tmp = 0;
        for( i = -nrofvars; i <= nrofvars; i++ )
        {
	    TernaryImp[ i ]     = TernaryImpTable + 2 * _tmp;
            _tmp               += TernaryImpSize[ i ];
	    TernaryImpLast[ i ] = TernaryImpSize[ i ];
        }

	fill_ternary_implication_arrays();

	rebuild_BinaryImp();

	init_freevars();

	for( i = 0; i < nrofceq ; i++ )
	    assert( CeqSizes[ i ] != 1 );
#ifdef EQ
	for( i = 0; i < nrofceq ; i++ )	
	    if( CeqSizes[ i ] == 2 )
            	DPLL_propagate_binary_equivalence( i );
#endif
#ifdef DETECT_COMPONENTS
	init_localbranching();
#endif
#ifdef CUT_OFF
	solution_bits = CUT_OFF - 1;
#endif

	push_stack_blocks();

	return 1;
}

void disposeSolver()
{
	if( kSAT_flag )
	{
		free_big_clauses_datastructures();
          	FREE( clause_reduction );
           	FREE( big_global_table );
           	FREE( clause_red_depth );
           	FREE( clause_SAT_flag );

           	FREE_OFFSET( big_to_binary );
           	FREE_OFFSET( btb_size );
	}

        free_BinaryImp();

        dispose_lookahead();
        dispose_preselection();
        dispose_freevars();

        FREE_OFFSET( TernaryImp        );
        FREE_OFFSET( TernaryImpSize    );
        FREE_OFFSET( tmpTernaryImpSize );
        FREE_OFFSET( bImp_stamps       );
        FREE_OFFSET( bImp_satisfied    );
        FREE_OFFSET( node_stamps       );

	FREE( tmpEqImpSize );
        FREE( impstack );
        FREE( rstack );
}

int propagate_forced_literals()
{
	int _freevars = freevars;

	if( IFIUP( 0, FIX_FORCED_LITERALS ) == UNSAT )
	    return UNSAT;

	nrofforced = _freevars - freevars;
	percentage_forced = 100.0 * nrofforced / (double) _freevars;

//	printf("c lookahead forced %.1f \% variables at depth %i\n", percentage_forced, depth ); 

	return SAT;
}


#ifdef SUPER_LINEAR
int super_linear_branching()
{
	int _result;

	sl_depth = 1;

	do
	{
	    _result = march_solve_rec();
	    if( _result != UNSAT )
		return _result;
#ifdef TIMEOUT
	    if( (int) clock() > CLOCKS_PER_SEC * TIMEOUT )
		return UNKNOWN;
#endif
	    pb_reset();
	
	    sl_depth++;

	}
	while(1);

	return UNSAT;

}
#endif

#ifdef DISTRIBUTION // naar distribution.c
int distribution_branching()
{
	int _result;

	target_rights  = 0;
	current_rights = 0;
	current_record = 0;

	first_time = (float) clock();

	do
	{
	    assert( target_rights <= jump_depth );

#ifdef SUBTREE_SIZE
	    path_length = 1;
#endif
	    _result = march_solve_rec();
	    if( _result != UNSAT )
		return _result;
#ifdef TIMEOUT
	    if( (int) clock() > CLOCKS_PER_SEC * TIMEOUT )
		return UNKNOWN;
#endif
#ifndef CUT_OFF
	    pb_reset();
#endif
	    target_rights++;

	    current_record = 1;
	
//	    print_records();
	}
	while( records[1].UNSAT_flag == 0 );

	return UNSAT;

}
#endif

#ifdef DISTRIBUTION
int skip_node( )
{
	if( depth < jump_depth )
	    if( (target_rights < current_rights) || 
		(target_rights >= (current_rights + jump_depth - depth)) ) 
		return 1;
	return 0;
}
#endif

void printConflict()
{
    if( kSAT_flag )
    {
	int i, *_rstackp = rstack;

	printf("c at depth %i add conflict: ", depth );
	while( _rstackp < rstackp )
	{
	    i = *(_rstackp++);

	    if( timeAssignments[ i ] >= NARY_MAX ) // is dit nodig???
	    {
		if( (TernaryImpReduction[ i ] + TernaryImpReduction[ -i ]) != 0 )
		{
		    if( timeAssignments[ i ] & 1 ) 	printf("%i ",  i );
		    else 				printf("%i ", -i );	
		}
		else
		{
		    printf("* ");
//		    if( timeAssignments[ i ] & 1 ) 	printf("(%i) ",  i );
//		    else 				printf("(%i) ", -i );	
		}
	    }
	}
	printf("\n\n");
    }
}

int march_solve_rec()
{
	int branch_literal = 0, _result, _percentage_forced;
	int skip_left = 0, skip_right = 0, top_flag = 0;
#ifdef DISTRIBUTION
	int record_index = current_record;

	top_flag = TOP_OF_TREE;

	if( fix_recorded_literals(record_index) == UNSAT )
	    return UNSAT;

	if( record_index == 0 ) record_index = init_new_record( );
#endif
#ifdef SUPER_LINEAR
	if( depth < sl_depth ) subtree_size = 0;
	else if( subtree_size == SL_MAX ) return UNSAT;
	else	subtree_size++;
#endif
#ifdef TIMEOUT
	if( (int) clock() > CLOCKS_PER_SEC * TIMEOUT )
	    return UNKNOWN;
#endif
#ifdef SUBTREE_SIZE
	path_length++;
#endif
#ifdef CUT_OFF
	if( depth <= CUT_OFF ) last_SAT_bin = -1;

        if( solution_bin == last_SAT_bin )
	{
#ifdef DISTRIBUTION
	    records[ record_index ].UNSAT_flag = 1;
#endif
            return UNSAT;
	}
#endif
#ifdef DETECT_COMPONENTS
	determine_components();
#endif
#ifdef DISTRIBUTION
	branch_literal = records[record_index].branch_literal;

	if( branch_literal != 0 ) dist_acc_flag = 1;
	else
//	if( branch_literal == 0 )
#endif
	do
	{
#ifdef LOCAL_AUTARKY
	    int _depth;
	
	    _depth = analyze_autarky();
	    if( _depth == 0 )
	        printf("c global autarky found at depth %i\n", depth );
	    else if( _depth != depth )
	        printf("c autarky found at depth %i (from %i)\n", depth, _depth );
#endif
	    nodeCount++;

//	    printf("node %i @ depth %i\n", nodeCount, depth );

	    if( ConstructCandidatesSet() == 0 )
	    { 
	        if( depth > 0 )
	        {
		    if( checkSolution() == SAT ) return verifySolution();
	        }
	    	if( PreselectAll() == 0 )
		    return verifySolution();
	    }
	    else	ConstructPreselectedSet();

	    if( lookahead() == UNSAT )
	    {
	    	lookDead++;
	    	return UNSAT;
	    }

	    if( propagate_forced_literals() == UNSAT )
		return UNSAT;

	    branch_literal = get_signedBranchVariable();

//	    printf("c branch literal %i", branch_literal );
	}
	while( (percentage_forced > 50.0) || (branch_literal == 0) );
#ifdef PLOT
	if( plotCount++ >= 10000 )
	    return UNSAT;

	printf("\t%i\t%.3f\t%i\n", plotCount, sum_plot / count_plot, depth );
#endif
	_percentage_forced = percentage_forced;

#ifdef PARALLEL
	if( depth < para_depth )
	    if( para_bin & (1 << (para_depth - depth - 1) ) )
	    	branch_literal *= -1;
#endif
//	printf("branch_literal = %i at depth %i\n", branch_literal, depth );
#ifdef GLOBAL_AUTARKY
	if( depth == 0 )
	{
	    int i;
	    for( i = 1; i <= nrofvars; i++ )
	    {
		TernaryImpReduction[  i ] = 0;
		TernaryImpReduction[ -i ] = 0;
		
		if( kSAT_flag )
		{
		    btb_size[  i ] = 0;
		    btb_size[ -i ] = 0;
		}
	    }

	    if( kSAT_flag )
		for( i = 0; i < nrofbigclauses; ++i )
		    clause_reduction[ i ] = 0;

//	    branch_literal = 10;
	}
#endif
	NODE_START();
#ifdef BLOCK_PRESELECT
	set_block_stamps( branch_literal );
#endif

#ifdef DISTRIBUTION
	if( top_flag )
	{	
	    branch_literal *= -1;
	    current_rights++;
	    UPDATE_BIN();
	}
	skip_left = skip_node();
#endif
	if( (skip_left==0) && IFIUP( branch_literal, FIX_BRANCH_VARIABLE ) )
	{ 
#ifdef DISTRIBUTION
	    current_record = LEFT_CHILD;
#endif
	    _result = recursive_solve();
#ifdef DISTRIBUTION
	    LEFT_CHILD = current_record;
#endif
	    if( _result == SAT || _result == UNKNOWN ) return _result; }
	else {  
#ifdef DISTRIBUTION
		if( (LEFT_CHILD != 0)  && records[ LEFT_CHILD ].UNSAT_flag == 0 )
		{
		    records[ LEFT_CHILD ].UNSAT_flag = 1; 
//		    printf("c left child %i UNSAT by parent!\n", LEFT_CHILD );
		}
#endif
		PUSH( r, STACK_BLOCK );}

	NODE_END();

#ifdef BACKJUMP
	if( backjump_literal != 0 )
	  if( timeAssignments[ backjump_literal ] >= NARY_MAX )
	  {
//		printf("backjumping at depth %i due to literal %i\n", depth, backjump_literal );
		return UNSAT;
	  }
	backjump_literal = 0;
#endif

#ifdef DISTRIBUTION
	if( top_flag )
	{
	    current_rights--;
	    UPDATE_BIN();
	}
#endif
	percentage_forced = _percentage_forced;

#ifdef PARALLEL
	if( depth < para_depth ) goto parallel_skip_node;
#endif

#ifdef GLOBAL_AUTARKY
	if( depth == 0 )
	  if( kSAT_flag )
	  {
	    int i;
	    for( i = 1; i <= nrofvars; ++i )
	    {
		assert( TernaryImpReduction[  i ] == 0 );
		assert( TernaryImpReduction[ -i ] == 0 );

		assert( btb_size[  i ] == 0 );
		assert( btb_size[ -i ] == 0 );
	    }

	    for( i = 0; i < nrofbigclauses; ++i )
	    {
		clause_red_depth[ i ] = nrofvars;
		clause_SAT_flag[ i ]  = 0;
	    }
	  }

	  if( kSAT_flag )
	  {
	    int i;
	    for( i = 1; i <= nrofvars; ++i )
	    {
		assert( TernaryImpReduction[  i ] >= 0 );
		assert( TernaryImpReduction[ -i ] >= 0 );

		assert( btb_size[  i ] >= 0 );
		assert( btb_size[ -i ] >= 0 );
	    }
	  }

#endif
	NODE_START();
#ifdef BLOCK_PRESELECT
	set_block_stamps( branch_literal );
#endif
        if( top_flag == 0 )
	{
#ifdef DISTRIBUTION
	    current_rights++;
#endif
	    UPDATE_BIN();
	}
#ifdef DISTRIBUTION
	skip_right = skip_node();
#endif
	if( (skip_right == 0) && IFIUP( -branch_literal, FIX_BRANCH_VARIABLE ) )
	{
#ifdef DISTRIBUTION
	    current_record = RIGHT_CHILD;
#endif
	    _result = recursive_solve();
#ifdef DISTRIBUTION
	    RIGHT_CHILD = current_record;
#endif
	    if( _result == SAT || _result == UNKNOWN ) return _result;}
	else {  
#ifdef DISTRIBUTION
		if( (RIGHT_CHILD != 0) && records[ RIGHT_CHILD ].UNSAT_flag == 0 )
		{
		    records[ RIGHT_CHILD ].UNSAT_flag = 1; 
//		    printf("c right child %i UNSAT by parent!\n", RIGHT_CHILD );
		}
#endif
		PUSH( r, STACK_BLOCK );}
	NODE_END();

	if( top_flag == 0 )
	{	
#ifdef DISTRIBUTION
	    current_rights--;
#endif
	    UPDATE_BIN();
	}

#ifdef PARALLEL
	parallel_skip_node:;
#endif
//	printf("ended node %i at depth %i\n", nodeCount, depth );

	

#ifdef SUBTREE_SIZE
	if( (skip_flag == 0) && (jump_depth == 0)  && (current_rights == 0) )
	{
	    int subtree = path_length - depth;

//	    float cost = nodeCount * (CLOCKS_PER_SEC /  ((float)(clock() + 10 - first_time)));
//	    printf("c nodes per second = %.3f at level %i\n", cost, depth );

	    if( jump_depth >= 30 ) jump_depth = 999;

	    if( subtree >     SUBTREE_SIZE )
	    {
	        jump_depth = depth;

	        while( subtree > 2*SUBTREE_SIZE )
	        {
		   jump_depth++; 
		   subtree = subtree / 2;
	        }

	        if( jump_depth >= 20 ) jump_depth = 999;

	        skip_flag = 1;
	    }
	}
#endif
#ifdef DISTRIBUTION
	record_node( record_index, branch_literal, skip_left, skip_right );

	current_record = record_index;
#endif

#ifdef BACKJUMP
	if( kSAT_flag )
	{
	    int *_rstackp = rstackp, nrval;
	
	    while( --_rstackp > rstack )
	    {
		nrval = *_rstackp;
		if( (TernaryImpReduction[ nrval ] + TernaryImpReduction[ -nrval ]) != 0 )
		{
		    backjump_literal = nrval;
		    break;
		}
	    }
	}
#endif
#ifdef ADD_CONFLICT
	printConflict();
#endif
	return UNSAT;
}

int IFIUP( const int nrval, const int forced_or_branch_flag )
{
	int i, *_forced_literal_array, _forced_literals, *local_fixstackp;

	local_fixstackp = rstackp;
	end_fixstackp   = rstackp;

	currentTimeStamp = BARY_MAX;
	
	current_bImp_stamp = 1;

	for( i = nrofvars; i >= 1;  i-- )
	{
	    bImp_stamps[  i ] = 0;
	    bImp_stamps[ -i ] = 0;
	}

	if( forced_or_branch_flag == FIX_FORCED_LITERALS )
	{
	   get_forced_literals( &_forced_literal_array, &_forced_literals );
	   for( i = 0; i < _forced_literals; i++ )
	      	if( look_fix_binary_implications(*(_forced_literal_array++)) == UNSAT ) 
		    { MainDead( local_fixstackp ); return UNSAT; }
	}
#ifdef DISTRIBUTION
	else if( forced_or_branch_flag == FIX_RECORDED_LITERALS )
	{
	   get_recorded_literals( &_forced_literal_array, &_forced_literals );
	   for( i = 0; i < _forced_literals; i++ )
	      	if( look_fix_binary_implications(*(_forced_literal_array++)) == UNSAT ) 
		    { MainDead( local_fixstackp ); return UNSAT; }
	}
#endif
	else
	{
	 	if( look_fix_binary_implications( nrval ) == UNSAT ) 	
    		    { MainDead( local_fixstackp ); return UNSAT; }
	}

	while( local_fixstackp < end_fixstackp )
		if( DPLL_update_datastructures(*(local_fixstackp++)) == UNSAT ) 	
		    { MainDead( local_fixstackp ); return UNSAT; }

	rstackp = end_fixstackp;

	return SAT;
}

inline void reduce_big_occurences( const int clause_index, const int nrval )
{
#ifdef HIDIFF
	HiRemoveClause( clause_index );
#endif
#ifdef NO_TRANSLATOR
	int *clauseSet, index;
	int *literals = clause_list[ clause_index ], lit;
	while( *literals != LAST_LITERAL )
	{
	    lit =  *(literals++);
	    if( lit != nrval )
	    {
	        clauseSet = clause_set[ lit ];
	        index = 0; //wellicht niet nodig
	        while( 1 )
	    	{
		    if( *(clauseSet++) == clause_index )
		    {
		        clauseSet[ -1 ] = clause_set[ lit ][ big_occ[ lit ] - 1 ];
		        clause_set[ lit ][ big_occ[ lit ] - 1 ] = LAST_CLAUSE;
		        break;
		    }
		    index++;
		}
	    }
	    big_occ[ lit ]--;
	}
#endif
}

inline int DPLL_update_datastructures( const int nrval )
{
	int i, *bImp;
#ifdef EQ
	int nr, ceqidx;
	nr = NR( nrval );
        PUSH( sub, STACK_BLOCK );
#endif
//	printf("FIXING %i\n", nrval ); 

	FIX( nrval, NARY_MAX );

//	diff[  nrval ] = 0;
//	diff[ -nrval ] = 0;

#ifdef TIMEOUT
	if( (int) clock() > CLOCKS_PER_SEC * TIMEOUT )
	    return SAT;
#endif
	unitResolveCount++;
	reduce_freevars( nrval );


        bImp = BIMP_START(-nrval);
        for( i = BIMP_ELEMENTS; --i; )
            bImp_satisfied[ -(*(bImp++)) ]++;

	// Update eager datastructures
	if( kSAT_flag == 0 )
	{
#ifdef GLOBAL_AUTARKY
	    int lit1, lit2;

            int *tImp = TernaryImp[ nrval ] + 2 * TernaryImpSize[ nrval ];
            for( i = TernaryImpLast[ nrval ] - TernaryImpSize[ nrval ]; i--; )
            {
	    	lit1 = *(tImp++);
	    	lit2 = *(tImp++);

	    	if( IS_REDUCED_TIMP( lit1, lit2 ) )
		    TernaryImpReduction[ lit1 ]--;
	    	else if( IS_REDUCED_TIMP( lit2, lit1 ) )
                    TernaryImpReduction[ lit2 ]--;
            }
#endif
	    remove_satisfied_implications(  nrval );
	    remove_satisfied_implications( -nrval );

#ifdef GLOBAL_AUTARKY
            tImp = TernaryImp[ -nrval ];
            for( i = tmpTernaryImpSize[ -nrval ]; i--; )
	    {
	        TernaryImpReduction[ *(tImp++) ]++;
	        TernaryImpReduction[ *(tImp++) ]++;
	    }
#endif
	}
	else
	{
	  int *clauseSet, clause_index; 

	  // REMOVE SATISFIED CLAUSES
	  clauseSet = clause_set[ nrval ];
	  while( *clauseSet != LAST_CLAUSE )
	  {
	    clause_index = *(clauseSet++);

	    // if clause is not satisfied
	    if( clause_length[ clause_index ] < SAT_INCREASE - 2 )	
	    {
#ifdef GLOBAL_AUTARKY
		// if clause is already been reduced
		if( clause_reduction[ clause_index ] > 0 )
		{
                    int *literals = clause_list[ clause_index ];
                    while( *literals != LAST_LITERAL )
			TernaryImpReduction[ *(literals++) ]--;
		}
		clause_SAT_flag[ clause_index ] = 1;
#endif
		reduce_big_occurences( clause_index, nrval );
	    }
	    clause_length[ clause_index ] += SAT_INCREASE;
	  }
#ifdef GLOBAL_AUTARKY
	  for( i = 0; i < btb_size[ nrval ]; ++i )
	  {
	    // decrease literal reduction
            int *literals = clause_list[ big_to_binary[ nrval ][ i ] ], flag = 0;
            while( *literals != LAST_LITERAL )
            {
		if( timeAssignments[ *(literals++) ] == NARY_MAX )
		{
		    if( flag == 1 ) { flag = 0; break; }
		    flag = 1;
		}
            }

	    if( flag == 1 )
	    {
		clause_SAT_flag[  big_to_binary[ nrval ][ i ] ] = 1;
		literals = clause_list[ big_to_binary[ nrval ][ i ] ];
	    	while( *literals != LAST_LITERAL )
	            TernaryImpReduction[ *(literals++) ]--;
	    }
	  }
#endif
	}
	

#ifdef GLOBAL_AUTARKY
#ifdef EQ	
	tmpEqImpSize[ nr ] = Veq[ nr ][ 0 ];
	for( i = 1; i < Veq[ nr ][0]; i++ )
        {
	    int j;
            ceqidx = Veq[ nr ][i];
            for( j = 0; j < CeqSizes[ ceqidx ]; j++ )
                TernaryImpReduction[ Ceq[ceqidx][j] ]++;
        }
#endif
#endif
	if( kSAT_flag )
	{
	  int UNSAT_flag, *clauseSet, clause_index;
	  int first_lit, *literals, lit;

	  // REMOVE UNSATISFIED LITERALS
	  UNSAT_flag = 0;
	  clauseSet = clause_set[ -nrval ];
	  while( *clauseSet != LAST_CLAUSE )
	  {
	    clause_index = *(clauseSet++);
#ifdef GLOBAL_AUTARKY
	    // if clause is for the first time reduced
	    if( clause_reduction[ clause_index ] == 0 )
	    {
                int *literals = clause_list[ clause_index ];
                while( *literals != LAST_LITERAL )
		    TernaryImpReduction[ *(literals++) ]++;

		clause_red_depth[ clause_index ] = depth;
	    }
	    clause_reduction[ clause_index ]++;
#endif
	    clause_length[ clause_index ]--;
#ifdef HIDIFF
	    HiRemoveLiteral( clause_index, nrval );
#endif
            if(  clause_length[ clause_index ] == 2 )
            {
#ifdef GLOBAL_AUTARKY
                literals = clause_list[ clause_index ];
                while( *literals != LAST_LITERAL )
                {
                    lit = *(literals)++;
		    if( timeAssignments[ lit ] < NARY_MAX )
			big_to_binary[ lit ][ btb_size[ lit ]++ ] = clause_index;
		}
#endif
		reduce_big_occurences( clause_index, -nrval );
		clause_length[ clause_index ] = SAT_INCREASE;

	        if( UNSAT_flag == 0 )
	        {
                    first_lit = 0;
                    literals = clause_list[ clause_index ];
                    while( *literals != LAST_LITERAL )
                    {
                        lit = *(literals)++;
                        if( IS_NOT_FIXED( lit ) )
                        {
                            if( first_lit == 0 ) first_lit = lit;
                            else
			    {
				UNSAT_flag = !DPLL_add_binary_implications( first_lit, lit ); 
				goto next_clause;
			    }
                        }
                        else if( !FIXED_ON_COMPLEMENT(lit) ) goto next_clause;
                    }

                    if( first_lit != 0 )  UNSAT_flag = !look_fix_binary_implications( first_lit );
                    else                  UNSAT_flag = 1;
                }
                next_clause:;
	    }
	  }

	  if( UNSAT_flag ) return UNSAT;
	}

	if( kSAT_flag == 0 )
	{
	    int *tImp = TernaryImp[ -nrval ];
            for( i = tmpTernaryImpSize[ -nrval ] - 1; i >= 0; i-- )
	    {
		int lit1 = *(tImp++);
		int lit2 = *(tImp++);
                if( DPLL_add_binary_implications( lit1, lit2 ) == UNSAT )
            	    return UNSAT;
	    }
	}

#ifdef EQ
        while( Veq[ nr ][ 0 ] > 1 )
        {
            ceqidx = Veq[ nr ][ 1 ];

            fixEq( nr, 1, SGN(nrval));
            PUSH( sub, nrval );

            if( CeqSizes[ ceqidx ] == 2 )
	    {
            	if ( DPLL_propagate_binary_equivalence( ceqidx ) == UNSAT )
               	    return UNSAT;
	    }
	    else if( CeqSizes[ ceqidx ] == 1 )
            {
            	if( look_fix_binary_implications(Ceq[ceqidx][0]*CeqValues[ceqidx]) == UNSAT )
                    return UNSAT;
            }
        }

        while( newbistackp != newbistack )
        {
            POP( newbi, ceqidx );
            if( CeqSizes[ ceqidx ] == 2 )
            	if ( DPLL_propagate_binary_equivalence( ceqidx ) == UNSAT )
                    return UNSAT;
        }
#endif
	return SAT;
}

inline void swap_ternary_implications( const int nrval, const int lit1, const int lit2 )
{
	int i, last, *tImp = TernaryImp[ nrval ];

	last = --TernaryImpSize[ nrval ];
	for( i = last - 1; i >= 0; i-- )
	    if( (tImp[ 2*i ] == lit1) && (tImp[ 2*i + 1 ] == lit2) )
	    {
	    	tImp[ 2*i     ] = tImp[ 2*last     ]; tImp[ 2*last     ] = lit1; 
		tImp[ 2*i + 1 ] = tImp[ 2*last + 1 ]; tImp[ 2*last + 1 ] = lit2; 
		return;
	    }
}

void remove_satisfied_implications( const int nrval )
{
        int i, lit1, lit2, *tImp = TernaryImp[ nrval ];

        for( i = TernaryImpSize[ nrval ] - 1; i >= 0; i-- )
        {
            lit1 = *(tImp++);
            lit2 = *(tImp++);
	    
	    swap_ternary_implications( lit1, lit2, nrval );
	    swap_ternary_implications( lit2, nrval, lit1 );
        }

        tmpTernaryImpSize[ nrval ] = TernaryImpSize[ nrval ];
        TernaryImpSize   [ nrval ] = 0;
}

int DPLL_propagate_binary_equivalence( const int bieq )
{
        int i, j, ceqsubst;
        int lit1, lit2;
        int value;

        lit1 = Ceq[ bieq ][ 0 ];
        lit2 = Ceq[ bieq ][ 1 ];
        value = CeqValues[ bieq ];

        for( i = 1; i < Veq[ lit1 ][ 0 ]; i++ )
        {
            ceqsubst = Veq[ lit1 ][ i ];
            for( j = 1; j < Veq[ lit2 ][ 0 ]; j++ )
            {
            	if( (ceqsubst == Veq[ lit2 ][ j ]) )
                {
                    fixEq( lit1, i, 1);
                    PUSH( sub, lit1 );

                    fixEq( lit2, j, value);
                    PUSH( sub, lit2 * value );

                    if( CeqSizes[ ceqsubst ] == 0 )
                       	if (CeqValues[ ceqsubst ] == -1 )
                            return UNSAT;

                    if( CeqSizes[ ceqsubst ] == 1 )
                      	if( !look_fix_binary_implications(Ceq[ceqsubst][0] * CeqValues[ceqsubst]) )
                    	    return UNSAT;
	
                    if( CeqSizes[ ceqsubst ] == 2 )
                     	PUSH( newbi, ceqsubst );

		    i--;
                    break;
                }
            }
        }

        if( (DPLL_add_binary_implications( lit1, -lit2 * value ) && 
	     DPLL_add_binary_implications( -lit1, lit2 * value )) == UNSAT )
                return UNSAT;

        return SAT;
}

inline int DPLL_add_compensation_resolvents( const int lit1, const int lit2 )
{
	int i, lit, *bImp = BIMP_START(lit2);

    	CHECK_NODE_STAMP( -lit1 );
	CHECK_BIMP_UPPERBOUND( -lit1, BinaryImp[ lit2 ][ 0 ] );

	for (i = BIMP_ELEMENTS; --i; )
	{
	    lit = *(bImp++); 
	    if( IS_FIXED(lit) ) continue;

	    if( bImp_stamps[ -lit ] == current_bImp_stamp )	
		return look_fix_binary_implications( lit1 );
#ifdef COMPENSATION_RESOLVENTS
	    if( bImp_stamps[ lit ] != current_bImp_stamp )
	    {
		CHECK_NODE_STAMP( -lit );
	      	CHECK_BIMP_BOUND  ( -lit );
		ADD_BINARY_IMPLICATIONS( lit, lit1 );
	    }
#endif
    	}
	return UNKNOWN;
}

int DPLL_add_binary_implications( int lit1, int lit2 )
{
	int i, *bImp;

	if( IS_FIXED(lit1) )
	{
	    if( !FIXED_ON_COMPLEMENT(lit1) )	return SAT;
	    else if( IS_FIXED(lit2) )	
		    return( !FIXED_ON_COMPLEMENT(lit2) );
	    else    return look_fix_binary_implications(lit2);
	}
	else if( IS_FIXED(lit2) )
	{
	    if( !FIXED_ON_COMPLEMENT(lit2) )	return SAT;
	    else    return look_fix_binary_implications(lit1);
	}
	
#ifdef BIEQ
	while( (VeqDepends[ NR(lit1) ] != INDEPENDENT) && 
	    (VeqDepends[ NR(lit1) ] != EQUIVALENT) )
		lit1 = VeqDepends[ NR(lit1) ] * SGN(lit1);

	while( (VeqDepends[ NR(lit2) ] != INDEPENDENT) && 
	    (VeqDepends[ NR(lit2) ] != EQUIVALENT) )
		lit2 = VeqDepends[ NR(lit2) ] * SGN(lit2);

	if( lit1 == -lit2 ) return SAT;
	if( lit1 ==  lit2 ) return look_fix_binary_implications(lit1);
#endif

	STAMP_IMPLICATIONS( -lit1 );
	if( bImp_stamps[ -lit2 ] == current_bImp_stamp )
	    return look_fix_binary_implications( lit1 );
	if( bImp_stamps[lit2] != current_bImp_stamp )
	{
	    int _result;

	    bImp_stamps[ BinaryImp[-lit1][ BinaryImp[-lit1][0] - 1] ] = current_bImp_stamp;

	    _result = DPLL_add_compensation_resolvents( lit1, lit2 );
	    if( _result != UNKNOWN ) 
		return _result;
	 
	    STAMP_IMPLICATIONS( -lit2 ); 
	    if( bImp_stamps[ -lit1 ] == current_bImp_stamp )
	    	return look_fix_binary_implications( lit2 );

	    _result = DPLL_add_compensation_resolvents( lit2, lit1 );
	    if( _result != UNKNOWN ) 
		return _result;

	    ADD_BINARY_IMPLICATIONS( lit1, lit2 );
	}
	return SAT;
}

int autarky_stamp( const int nrval )
{
	int i, *tImp, lit1, lit2, flag = 0;

	tImp = TernaryImp[ -nrval ];
	for( i = tmpTernaryImpSize[ -nrval ]; i--; )
	{
	    lit1 = *(tImp++);
	    lit2 = *(tImp++);
		
	    if( IS_NOT_FIXED(lit1) && IS_NOT_FIXED(lit2) )
	    {
		flag = 1;
		if( VeqDepends[ NR(lit1) ] == DUMMY )
		    autarky_stamp( lit1 );
		else
		    TernaryImpReduction[ lit1 ]++;

		if( VeqDepends[ NR(lit2) ] == DUMMY )
		    autarky_stamp( lit2 );
		else
		TernaryImpReduction[ lit2 ]++;
	    }
	}
	return flag;
}

int analyze_autarky( )
{
#ifdef LOOKAHEAD_ON_DUMMIES
	int *tImp, lit1, lit2;
#endif
	int i, j, nrval, twice;
	int new_bImp_flag, _depth, *_rstackp;

	if( depth == 0 ) return 0;

        for( i = 1; i <= nrofvars; i++ )
        {
            TernaryImpReduction[  i ] = 0;
            TernaryImpReduction[ -i ] = 0;
        }

	new_bImp_flag = 0;
	_rstackp      = rstackp;
	for( _depth = depth; _depth > 0; _depth-- )
	{
	  for( twice = 1 ; twice <= 2; twice++ )	    
	    while( *(--_rstackp) != STACK_BLOCK )
	    {
		nrval = *(_rstackp);
#ifndef LOOKAHEAD_ON_DUMMIES
		if( autarky_stamp( nrval ) == 1 )
		    new_bImp_flag = 1;
#else
		tImp = TernaryImp[ -nrval ];
		for( i = tmpTernaryImpSize[ -nrval ]; i--; )
		{
		    lit1 = *(tImp++);
		    lit2 = *(tImp++);
		
		    if( IS_NOT_FIXED(lit1) && IS_NOT_FIXED(lit2) )
		    {
			new_bImp_flag = 1;
			TernaryImpReduction[ lit1 ]++;
			TernaryImpReduction[ lit2 ]++;
		    }
		}
#endif
	        for( j = 1; j < tmpEqImpSize[NR(nrval)]; j++ )
	        {
	            int ceqsubst = Veq[NR(nrval)][j];
	    	    for( i = 0; Ceq[ceqsubst][i] != NR(nrval); i++ )
		    {
			new_bImp_flag = 1;
			//printf("%i (%i)", Ceq[ ceqsubst][i], _depth );
	   	        TernaryImpReduction[ Ceq[ceqsubst][i] ]++;
		    }
	        }
	    }
	  if( new_bImp_flag == 1 ) break;
	}
	return _depth;
}

void backtrack()
{
	int nrval, varnr, size;

	while( !( *( rstackp - 1 ) == STACK_BLOCK ) )
	{   POP_BACKTRACK_RECURSION_STACK }
	POP_RECURSION_STACK_TO_DEV_NULL

	while( !( *( subsumestackp - 1 ) == STACK_BLOCK ) )
	{	
	    POP( subsume, nrval );

	    TernaryImpSize[ TernaryImp[nrval][ 2*TernaryImpSize[nrval]   ] ]++;
	    TernaryImpSize[ TernaryImp[nrval][ 2*TernaryImpSize[nrval]+1 ] ]++;
	    TernaryImpSize[ nrval ]++;
	}
	subsumestackp--;

	while( !( *( rstackp - 1 ) == STACK_BLOCK ) )
	{   POP_BACKTRACK_RECURSION_STACK }
	POP_RECURSION_STACK_TO_DEV_NULL

        while( !( *( bieqstackp - 1 ) == STACK_BLOCK ) )
        {
	    POP( bieq, varnr );
            VeqDepends[ varnr ] = INDEPENDENT;         
        }
        bieqstackp--;

	while( !( *( impstackp - 1 ) == STACK_BLOCK ) )
	{
	    POP( imp, size );
	    POP( imp, nrval );
	    BinaryImp[ nrval ][ 0 ] = size;
	}
	impstackp--;
}

void MainDead( int *local_fixstackp )
{
	int nrval;
	mainDead++;

        while( end_fixstackp > local_fixstackp )
        {
	    nrval = *(--end_fixstackp);
	    UNFIX( nrval ); 
	}
	rstackp = end_fixstackp;
}

inline void restore_big_occurences( const int clause_index, const int nrval )
{
#ifdef HIDIFF
	HiAddClause( clause_index );
#endif
#ifdef NO_TRANSLATOR
	int *literals = clause_list[ clause_index ], lit;
	while( *literals != LAST_LITERAL )
	{
	    lit = *(literals++);
	    if( lit != nrval )
	        clause_set[ lit ][ big_occ[ lit ] ] = clause_index;
	    big_occ[ lit ]++;
	}
#endif
}

void restore_implication_arrays( const int nrval )
{
	int i, *bImp;
#ifdef GLOBAL_AUTARKY
	int lit1, lit2;
#endif
#ifdef EQ
	int ceqsubst, var;
	while( !( *( substackp - 1 ) == STACK_BLOCK ) )
	{
	    POP( sub, var );
	    ceqsubst = Veq[ NR(var) ][ Veq[ NR(var) ][ 0 ]++ ];
	    CeqValues[ ceqsubst ] *= SGN(var);
	    CeqSizes[ ceqsubst ]++; 
        }
        substackp--;
#ifdef GLOBAL_AUTARKY
	for( i = 1; i < Veq[NR(nrval)][0]; i++ )
	{
	    int j;
	    ceqsubst = Veq[NR(nrval)][i];
	    for( j = 0; j < CeqSizes[ceqsubst]; j++ )
	   	TernaryImpReduction[ Ceq[ceqsubst][j] ]--;
	}
#endif
#endif
//	printf("UNFIXING %i\n", nrval );

	if( kSAT_flag )
	{
	  int clause_index, *clauseSet;
	  clauseSet = clause_set[ nrval ];
	  while( *clauseSet != LAST_CLAUSE )
	  {
	    clause_index = *(clauseSet++);
	    clause_length[ clause_index ] -= SAT_INCREASE;

	    if( clause_length[ clause_index ] < SAT_INCREASE - 2 )
	    {
		restore_big_occurences( clause_index, nrval );
#ifdef GLOBAL_AUTARKY
		clause_SAT_flag[ clause_index ] = 0;
		if( clause_reduction[ clause_index ] > 0 )
		{
                    int *literals = clause_list[ clause_index ];
                    while( *literals != LAST_LITERAL )
			TernaryImpReduction[ *(literals++) ]++;
		}
#endif
	    }
	  }
#ifdef GLOBAL_AUTARKY
	  for( i = 0; i < btb_size[ nrval ]; ++i )
	  {
	    // decrease literal reduction
            int *literals = clause_list[ big_to_binary[ nrval ][ i ] ], flag = 0;
            while( *literals != LAST_LITERAL )
            {
		if( timeAssignments[ *(literals++) ] == NARY_MAX )
		{
		    if( flag == 1 ) { flag = 0; break; }
		    flag = 1;
		}
            }

	    if( flag == 1 )
	    {
		clause_SAT_flag[ big_to_binary[ nrval ][ i ] ] = 0;
		literals = clause_list[ big_to_binary[ nrval ][ i ] ];
            	while( *literals != LAST_LITERAL )
	            TernaryImpReduction[ *(literals++) ]++;
	    }
	  }
#endif
          clauseSet = clause_set[ -nrval ];
	  while( *clauseSet != LAST_CLAUSE )
          {
            clause_index = *(clauseSet++);
	    if( clause_length[ clause_index ] == SAT_INCREASE )
	    {
		restore_big_occurences( clause_index, -nrval );
		clause_length[ clause_index ] = 2;
#ifdef GLOBAL_AUTARKY
                int *literals = clause_list[ clause_index ];
                while( *literals != LAST_LITERAL )
                {
                    int lit = *(literals)++;
                    if( timeAssignments[ lit ] < NARY_MAX )
			btb_size[ lit ]--;
		}
#endif
	    }

#ifdef GLOBAL_AUTARKY
            clause_reduction[ clause_index ]--;

	    // if clause is restored to original length
	    if( clause_reduction[ clause_index ] == 0 )
	    {
		// decreasee literal reduction array
                int *literals = clause_list[ clause_index ];
                while( *literals != LAST_LITERAL )
		    TernaryImpReduction[ *(literals++) ]--;
		
		clause_red_depth[ clause_index ] = nrofvars;
	    }
#endif
#ifdef HIDIFF
	    HiAddLiteral( clause_index, nrval );
#endif
            clause_length[ clause_index ]++;
          }
	}

	/* restore all literals that were removed due to fixing of nrval */
	if( kSAT_flag == 0 )
	{
	    int *tImp = TernaryImp[ -nrval ];
	    for( i = TernaryImpSize[ -nrval ] = tmpTernaryImpSize[ -nrval ]; i--; )
	    {
	    	TernaryImpSize[ *(tImp++) ]++;
	    	TernaryImpSize[ *(tImp++) ]++;
	    }
#ifdef GLOBAL_AUTARKY
	    tImp = TernaryImp[ -nrval ];
            for( i = tmpTernaryImpSize[ -nrval ]; i--; )
	    {
	    	TernaryImpReduction[ *(tImp++) ]--;
	    	TernaryImpReduction[ *(tImp++) ]--;
	    }
#endif
	/* restore all clauses that were removed due to fixing of nrval */
	    tImp = TernaryImp[ nrval ];
	    for( i = TernaryImpSize[ nrval ] = tmpTernaryImpSize[ nrval ]; i--; )
	    {
	    	TernaryImpSize[ *(tImp++) ]++;
	    	TernaryImpSize[ *(tImp++) ]++;
	    }
#ifdef GLOBAL_AUTARKY
	    tImp = TernaryImp[ nrval ] + 2 * TernaryImpSize[ nrval ];
	    for( i = TernaryImpLast[ nrval ] - TernaryImpSize[ nrval ]; i--; )
	    {
		lit1 = *(tImp++);
		lit2 = *(tImp++);

	    	if( IS_REDUCED_TIMP( lit1, lit2 ) )
                    TernaryImpReduction[ lit1 ]++;
		else if( IS_REDUCED_TIMP( lit2, lit1 ) )
                    TernaryImpReduction[ lit2 ]++;
            }
#endif
	}

        bImp = BIMP_START(-nrval);
        for( i = BIMP_ELEMENTS; --i; )
            bImp_satisfied[ -(*(bImp++)) ]--;

	freevars++;
	
	UNFIX( nrval ); 
}

int checkSolution( )
{
	int i, *sizes;

//	printf("c checking solution %i\n", nodeCount );

	if( kSAT_flag ) sizes = big_occ;
	else 		sizes = TernaryImpSize;

	for ( i = 1; i <= original_nrofvars; i++ )
	    if( IS_NOT_FIXED( i ) )
	    {
	        if( sizes[  i ] > 0 ) { return UNSAT; }
	        if( sizes[ -i ] > 0 ) { return UNSAT; }
	        if( BinaryImp[  i ][ 0 ] > bImp_satisfied[  i ] ){ return UNSAT; }
	        if( BinaryImp[ -i ][ 0 ] > bImp_satisfied[ -i ] ){ return UNSAT; }
	    }

	return SAT;
}

int verifySolution()
{
	int i, j, satisfied, dollars;
#ifndef CUT_OFF
	unsigned long long mask = 0xffffffffffffffffLLU;

	dollars = solution_bits - depth;
	if( dollars < 1 ) dollars = 1;

	printf("\nc |" );
	for( i = solution_bits; i >= dollars; i-- )
	{
#ifdef DISTRIBUTION
	    if( solution_bits - i == jump_depth ) printf(".");
#endif
#ifdef SUPER_LINEAR
	    if( solution_bits - i == sl_depth ) printf(".");
#endif
	    if( ((solution_bin & mask) >> i) > 0 ) printf("1");
	    else printf("0");
	    mask = mask >> 1;
	}

	for( i = solution_bits - depth; i >= 2; i-- )
	   printf("$");

	printf("|\n");
#else
        printf("s %i\n", solution_bin + 1);
	fflush( stdout );

        last_SAT_bin = solution_bin;

        return UNSAT;
#endif
#ifdef DISTRIBUTION 
//	printf("c direction branchin :: current_rights = %i\n", current_rights );
#endif
#ifdef COUNT_SAT
	count_sat++;
	//printSolution( original_nrofvars );
	return UNSAT;
#endif
	do
	{ fixDependedEquivalences(); }
	while( dependantsExists() );

	/* check all 3-clauses */

	for( i = 0; i < nrofclauses; i++ )
	{
	    satisfied = 0;
	    if( Clength[ i ] == 0 ) continue;

	    for( j = 0; j < Clength[ i ]; j++ )
		if( timeAssignments[ Cv[ i ][ j ] ] == VARMAX ) satisfied = 1;

  	    if( !satisfied )
	    {
	 	printf("\nc clause %i: ", i);
		for( j = 0; j < Clength[ i ]; j++ )
	  	    printf("%i [%i] ", Cv[i][j], IS_FIXED(Cv[i][j]) );
		printf("not satisfied yet\n");	
		return UNKNOWN;
	    }
	}

#ifdef EQ
        for( i = 0; i < nrofceq; i++ )
        {
      	    int value = CeqValues[ i ];
            for( j = 0; j < CeqSizes[ i ]; j++ )
            	value *= EQSGN( Ceq[ i ][ j ] );
            if( value == -1 )
            {
                printf("c eq-clause %i is not satisfied yet\n", i);
            	return UNKNOWN;
            }
        }
#endif
	return SAT;
}

//kan ook elders heen...

void printSolution( const int orignrofvars )
{
	int i;
#ifdef WAERDEN
	int j, pos = 0, neg = 0, rows;

	for( i = 1; i <= orignrofvars; i++ )
	    if     ( timeAssignments[ i ] ==  VARMAX      ) pos++;
	    else if( timeAssignments[ i ] == (VARMAX + 1) ) neg++;

	rows    = (neg + pos) / pos;

	if( rows > 1 && (neg % pos == 0) )
	{
	    for( j = 1; j <= rows; j++ )
	    {
	    	printf("partition %i :: ", j );
	    	for( i = j; i <= orignrofvars; i += rows )
	            if( timeAssignments[ i ] ==  VARMAX ) printf(" %i", (i+ rows -1) / rows );
	    	printf("\n");
	    }
	    return;
	}
#endif
	printf("v");
	for( i = 1; i <= orignrofvars; i++ )
	    if     ( timeAssignments[ i ] ==  VARMAX      ) printf(" %i",  i);
	    else if( timeAssignments[ i ] == (VARMAX + 1) ) printf(" %i", -i);
	printf(" 0\n");
}

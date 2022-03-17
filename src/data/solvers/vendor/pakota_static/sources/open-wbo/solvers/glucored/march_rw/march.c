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
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*/

#include <stdlib.h>
#include <math.h>
#include <time.h>

#include "march.h"
#include "common.h"
#include "equivalence.h"
#include "lookahead.h"
#include "parser.h"
#include "preselect.h"
#include "progressBar.h"
#include "resolvent.h"
//#include "rootlook.h"
#include "solver.h"
#include "memory.h"
//#include "translator.h"

void handleUNSAT()
{
	printf( "c main():: nodeCount: %i\n", nodeCount );
	printf( "c main():: time=%f\n", ((float)(clock()))/CLOCKS_PER_SEC );
#ifdef SATTEST
	flup = fopen("results.py","w");
	fprintf(flup,"nodes=%d\n",nodeCount);
	fprintf(flup,"lookaheads=%d\n", lookAheadCount);
	fprintf(flup,"time=%f\n", ((double)(clock()))/((double)(CLOCKS_PER_SEC)) );
	fclose(flup);
#endif
       	printf( "s UNSATISFIABLE\n" );
	
	disposeFormula();

	exit( EXIT_CODE_UNSAT);
}

int main( int argc, char** argv )
{
	int i, result, exitcode;
#ifdef SATTEST
        FILE *flup;
#endif

	/* let's not be too optimistic... :) */
	result   = UNKNOWN;
	exitcode = EXIT_CODE_UNKNOWN;
#ifndef PRINT_FORMULA
	printf( "c main():: ***                                   [ march satisfiability solver ]                                   ***\n" );
	printf( "c main()::  **                Copyright (C) 2001-2009 M.J.H. Heule, J.E. van Zwieten, and M. Dufour                 **\n" );
	printf( "c main()::   *  This program may be redistributed and/or modified under the terms of the GNU Gereral Public License  *\n" );
	printf( "c main()::\n" );
#endif
        if( argc < 2 )
        {
                printf( "c main():: input file missing, usage: ./solve < DIMACS-file.cnf >\n" );
                return EXIT_CODE_ERROR;
        }
#ifdef PARALLEL
	if( argc == 4 )
	{
	    para_depth = atoi( argv[2] );
	    para_bin   = atoi( argv[3] );
	}
	else
	{
	    para_depth = 0;
	    para_bin   = 0;
	}
	printf("c running in parallel using %i processors with currently number %i\n", 1 << para_depth, para_bin );
#endif

	/*
		Parsing...
	*/
	runParser( argv[ 1 ] );
#ifndef PRINT_FORMULA
        printf( "c preprocessing fase I completed:: there are now %i free variables and %i clauses.\n", freevars, nrofclauses );
#endif
	/*
		Preprocessing...
	*/

#ifdef PRINT_FORMULA
	compactCNF();
	printFormula( Cv );
	exit(0);
#endif
#ifdef DIAMETER
	init_diameter();
#endif
	if( equivalence_reasoning() == UNSAT )	handleUNSAT();

        for( i = 0; i < nrofclauses; i++ )
            if( Clength[ i ] > 3 )
	    { kSAT_flag = 1; break; }
/*
#ifdef NO_TRANSLATOR
	kSAT_flag = check_kSAT();
#else
	kSAT_flag = 0;
#endif
*/

	if( kSAT_flag )	printf("c using k-SAT heuristics (size based diff)\n");
	else		printf("c using 3-SAT heuristics (occurence based diff)\n");
	//check_integrety();

#ifndef TERNARYLOOK
	if( resolvent_look() == UNSAT ) handleUNSAT();
#endif
        if( kSAT_flag )         allocate_big_clauses_datastructures();

//	TransformFormula();

        printf( "c main():: clause / variable ratio: ( %i / %i ) = %.2f\n", nrofclauses, nrofvars, (double) nrofclauses / nrofvars );

	depth                 = 0;   // naar solver.c ?
        nodeCount             = 0;
        lookAheadCount        = 0;
        unitResolveCount      = 0;
	necessary_assignments = 0;

//	dispose_preprocessor_eq();

	if( initSolver() )			//dit staat wel heel gek...
	{
#ifdef DOUBLELOOK
//	        printf("c suggested doublelook initial :: %i\n", (int) DL_trigger );
#endif
#ifdef TIMEOUT
		printf( "c main():: timeout = %i seconds\n", TIMEOUT );
#endif
		printf( "c main():: all systems go!\n" );

#ifdef PROGRESS_BAR
		pb_init( 6 );
#endif

#ifdef DISTRIBUTION
		result = distribution_branching();
#else  
#ifdef SUPER_LINEAR
		result = super_linear_branching();
#else
		result = march_solve_rec();
#endif
#endif

#ifdef PROGRESS_BAR
		pb_dispose();
#endif
	}
	else
	{
		printf( "c main():: conflict caused by unary equivalence clause found.\n" );
		result = UNSAT;
	}

#ifdef PLOT
	{
		int tmp = DL_trigger_sum / doublelook_count;

		if( tmp < 10 )
		    printf("#\tset\tyrange[0:15]\n");
		else if( tmp < 20 )
		    printf("#\tset\tyrange[0:30]\n");
		else if( tmp < 35 )
		    printf("#\tset\tyrange[0:50]\n");
		else if( tmp < 70 )
		    printf("#\tset\tyrange[0:100]\n");
		else if( tmp < 160 )
		    printf("#\tset\tyrange[0:200]\n");
		else if( tmp < 500 )
		    printf("#\tset\tyrange[0:600]\n");
		else if( tmp < 700 )
		    printf("#\tset\tyrange[0:900]\n");
		else
		    printf("#\tset\tyrange[0:1500]\n");

		printf("#\tset\txrange[1:%i]\n", nodeCount);
	}
#endif 	
        printf( "c main():: nodeCount: %i\n", nodeCount );
	printf( "c main():: dead ends in main: %i\n", mainDead );
        printf( "c main():: lookAheadCount: %i\n", lookAheadCount );
        printf( "c main():: unitResolveCount: %i\n", unitResolveCount );
        printf( "c main():: time=%f\n", ((float)(clock()))/CLOCKS_PER_SEC );
	printf( "c main():: necessary_assignments: %i\n", necessary_assignments );
	printf( "c main():: bin_sat: %i, bin_unsat %i\n", bin_sat, bin_unsat );
#ifdef DOUBLELOOK
	printf( "c main():: doublelook: #: %d, succes #: %d\n", (int) doublelook_count, (int) (doublelook_count - doublelook_failed) );
	printf( "c main():: doublelook: overall %.3f of all possible doublelooks executed\n", 
		100.0 * dl_actual_counter / (double) dl_possibility_counter );
 	printf( "c main():: doublelook: succesrate: %.3f, average DL_trigger: %.3f\n", 100.0 - 
		(100.0 * doublelook_failed / doublelook_count), DL_trigger_sum / doublelook_count );
#endif
#ifdef LONG_LOOK
	printf("c marin():: longlook conflicts #: %i\n", ll_conflicts );
#endif
#ifdef COUNT_SAT 
	printf( "c main():: found %i solutions\n", count_sat );
	if( count_sat > 0 ) result = SAT;
#endif


#ifdef SATTEST
        flup = fopen("results.py","w");
        fprintf(flup,"nodes=%d\n",nodeCount);
        fprintf(flup,"lookaheads=%d\n", lookAheadCount);
        fprintf(flup,"time=%f\n", ((unsigned double)(clock()))/((unsigned double)(CLOCKS_PER_SEC)) );
	fprintf(flup,"dlrate=%d\n", (int)ceil(100.0 - (100.0 * doublelook_failed / doublelook_count)) );
	fprintf(flup,"DL_trigger=%d\n", (int)ceil(DL_trigger_sum / doublelook_count) );
	if( dl_possibility_counter > 0 )
	    fprintf(flup,"dlactual=%.2f\n", 100 * dl_actual_counter / (double) dl_possibility_counter );
        fclose(flup);
#endif
	switch( result )
	{
	    case SAT:
		printf( "c main():: SOLUTION VERIFIED :-)\n" );
		printf( "s SATISFIABLE\n" );
#ifndef COUNT_SAT
		printSolution( original_nrofvars );
#endif
		exitcode = EXIT_CODE_SAT;
		break;

	    case UNSAT:
                printf( "s UNSATISFIABLE\n" );
	        exitcode = EXIT_CODE_UNSAT;
		break;

	    default:
		printf( "s UNKNOWN\n" );
		exitcode = EXIT_CODE_UNKNOWN;
        }

	disposeSolver();

	disposeFormula();

        return exitcode;
}

void runParser( char* fname )
{
	FILE* in;

	if( ( in = fopen( fname, "r" ) ) == NULL )
	{
		printf( "c runParser():: input file could not be opened!\n" );
		exit( EXIT_CODE_ERROR );
	}

	if( !initFormula( in ) )
	{
		printf( "c runParser():: p-line not found in input, but required by DIMACS format!\n" );
		fclose( in );
		exit( EXIT_CODE_ERROR );
	}
	
	if( !parseCNF( in ) )
        {
                printf( "c runParser():: parse error in input!\n" );
		fclose( in );
                exit( EXIT_CODE_ERROR );
        }

	fclose( in );

#ifndef PRINT_FORMULA
	printf( "c runParser():: parsing was successful, warming up engines...\n" );
#endif
	init_equivalence();

	if( simplify_formula() == UNSAT )
	{
		printf( "c runParser():: conflicting unary clauses, so instance is unsatisfiable!\n" );
        	printf( "s UNSATISFIABLE\n" );

		disposeFormula();
		exit( EXIT_CODE_UNSAT );
	}
}

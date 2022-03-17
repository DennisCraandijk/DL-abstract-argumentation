#include <stdio.h>
#include <stdlib.h>

#include "progressBar.h"
#include "distribution.h"
#include "common.h"

int pb_count, pb_granularity, pb_currentDepth, pb_branchCounted;

void pb_init( int granularity )
{
  int i;

  if( granularity < 1 || granularity > 6 )
	exit( EXIT_CODE_ERROR );

  pb_granularity = granularity - 1;
  pb_currentDepth = 0;
  pb_branchCounted = 0;
  pb_count = 0;

  printf( "c |" );
  for( i = 0; i < ( 1 << granularity ); i++ )
  	printf( "-" );
  printf( "|" );  
  fflush( stdout );
}

void pb_reset()
{
  int i;
  pb_currentDepth = 0;
  pb_branchCounted = 0;
  pb_count = 0;
#ifdef DISTRIBUTION
  printf( "| (" );
  if( (jump_depth >= 10) && (target_rights < 10) ) printf(" ");
  printf( "%i/%i) NodeCount: %i\nc |", target_rights, jump_depth, nodeCount );
#else 
  printf( "| NodeCount: %i\nc |", nodeCount );
#endif
  for( i = 0; i < ( 1 << (pb_granularity+1) ); i++ )
  	printf( "-" );
  printf( "|" );  
  fflush( stdout );
}

void pb_dispose()
{
  pb_update();
  printf( "\nc\n" );
}


void pb_update()
{
  int i;

  printf( "\rc |" );
  for( i = 0; i < pb_count; i++ )
  	printf( "*" );
  fflush( stdout );
}

void pb_descend() {
  pb_branchCounted = 0;
  pb_currentDepth++;
}

void pb_climb() {
  pb_currentDepth--;

  if( ( pb_currentDepth == pb_granularity ) || ( pb_currentDepth < pb_granularity && !pb_branchCounted ) )
  {
    pb_branchCounted = 1;
    pb_count += ( 1 << ( pb_granularity - pb_currentDepth ) );
    pb_update();
  } 
}


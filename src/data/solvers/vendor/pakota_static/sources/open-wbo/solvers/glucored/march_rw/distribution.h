#include "common.h"

struct record {
    int branch_literal;
    int nrofforced;
    int forced_offset;
    int child[2];
    int UNSAT_flag;
};

unsigned int current_record;
struct record *records;

void init_direction();

int init_new_record();

int fix_recorded_literals( int record_index );
void record_node( const int record_index, int branch_literal, const int skip_left, const int skip_right );
void check_forced_array_capacity();

void set_recorded_literals( int record_index );
void get_recorded_literals( int **_forced_literal_array, int *_forced_literals );

void print_records();

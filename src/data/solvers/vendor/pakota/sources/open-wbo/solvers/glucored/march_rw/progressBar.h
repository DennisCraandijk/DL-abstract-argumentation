#ifndef __PROGRESSBAR_H__
#define __PROGRESSBAR_H__

void pb_init( int granularity );
void pb_dispose();
void pb_update();
void pb_descend();
void pb_climb();
void pb_reset();

#endif

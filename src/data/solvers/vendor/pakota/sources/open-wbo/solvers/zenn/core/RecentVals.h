/*****************************************************************************************[Queue.h]
MiniSat -- Copyright (c) 2003-2006, Niklas Een, Niklas Sorensson
  2008 - Gilles Audemard, Laurent Simon

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

#ifndef RecentLBDs_h
#define RecentLBDs_h

#include "mtl/Vec.h"

namespace Minisat {

//=================================================================================================

class RecentVals {
    vec<uint32_t> 	buf;
    int       	  	first;
	int		  		end;
	uint64_t  		total;
	int       		cap;
	int       		sz;
	int				wait_time;
	
public:
	RecentVals() : first(0), end(0), total(0), cap(0), sz(0), wait_time(0) {
	}

	void init(int size, int wtime=0) {
		buf.growTo(size);
		clear();
		cap = size;
		wait_time = wtime;
	}
	
	void push(uint32_t x) {
		if (sz==cap) {
			assert(end==first); // The queue is full, next value to enter will replace oldest one
			total -= buf[end];
			if ((++end) == cap) end = 0;
		} else 
			sz++;
		total += x;
		buf[first] = x;
		if ((++first) == cap) first = 0;
		if (wait_time > 0) wait_time--;
	}

	uint64_t sum()     const { return total; }
	double   average() const { return (double)total / (double)sz; }
	bool     ready()   const { return wait_time == 0 && sz == cap; }
	
	void 	 clear() 		 { first = 0; end = 0; sz = 0; total = 0; }
};

//=================================================================================================
}
#endif

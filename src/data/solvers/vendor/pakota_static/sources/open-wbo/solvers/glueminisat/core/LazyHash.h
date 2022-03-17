/***********************************************************************************[LazyHash.h]
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

#ifndef LazyHash_h
#define LazyHash_h

#include "mtl/Vec.h"

namespace Minisat {

//=================================================================================================

class LazyHash {
    vec<Lit>	table;
    Lit         last_lit;

    inline uint32_t index  (const Lit p) const { return (unsigned)toInt(p) % table.size(); }

public:
           void init      (Lit p, int size)   { table.growTo(size, p); last_lit = p; }
    inline bool contains  (const Lit p) const { return table.size() > 0 && table[index(p)] == p; }
    inline void put       (const Lit p)       { assert(table.size() > 0); table[index(p)] = p; last_lit = p; }
    inline Lit  last      ()            const { return last_lit; }
    inline int  size      ()            const { return table.size(); }
    inline Lit  operator [] (int index) const { return table[index]; }
};

//=================================================================================================
}
#endif

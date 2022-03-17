/*****************************************************************************************[MaxSAT.h]
Open-WBO -- Copyright (c) 2013-2015, Ruben Martins, Vasco Manquinho, Ines Lynce

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
************************************************************************************************/

#ifndef Soft_h
#define Soft_h

#ifdef SIMP
#include "simp/SimpSolver.h"
#else
#include "core/Solver.h"
#endif

namespace NSPACE
{

class Soft
{

public:
  Soft(vec<Lit> &soft, int w, Lit assump, vec<Lit> &relax)
  {
    soft.copyTo(clause);
    weight = w;
    assumptionVar = assump;
    relax.copyTo(relaxationVars);
  }

  Soft() {}
  ~Soft()
  {
    clause.clear();
    relaxationVars.clear();
  }

  vec<Lit> clause;   // Soft clause
  int weight;        // Weight of the soft clause
  Lit assumptionVar; // Assumption variable used for retrieving the core
  // Relaxation variables that will be added to the soft clause
  vec<Lit> relaxationVars;
};
}

#endif

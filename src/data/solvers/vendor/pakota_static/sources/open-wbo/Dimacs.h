/*****************************************************************************************[Dimacs.h]
MiniSat  -- Copyright (c) 2003-2006, Niklas Een, Niklas Sorensson
            Copyright (c) 2007-2010, Niklas Sorensson
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

#ifndef Dimacs_h
#define Dimacs_h

#include <stdio.h>

#include "utils/ParseUtils.h"
#include "core/SolverTypes.h"
#include "MaxSAT.h"

namespace NSPACE
{

//=================================================================================================
// DIMACS Parser:

template <class B> static int64_t parseWeight(B &in)
{
  int64_t val = 0;
  while ((*in >= 9 && *in <= 13) || *in == 32)
    ++in;
  if (*in < '0' || *in > '9')
    fprintf(stderr, "PARSE ERROR! Unexpected char: %c\n", *in), exit(3);
  while (*in >= '0' && *in <= '9')
    val = val * 10 + (*in - '0'), ++in;
  return val;
}

template <class B, class MaxSAT>
static int readClause(B &in, MaxSAT *S, vec<Lit> &lits)
{
  int parsed_lit, var;
  int64_t weight = 1;
  lits.clear();
  if (S->getProblemType() == _WEIGHTED_) weight = parseWeight(in);
  assert(weight > 0);

  for (;;)
  {
    parsed_lit = parseInt(in);
    if (parsed_lit == 0) break;
    var = abs(parsed_lit) - 1;
    while (var >= S->nVars())
      S->newVar();
    lits.push((parsed_lit > 0) ? mkLit(var) : ~mkLit(var));
  }
  return weight;
}

template <class B, class MaxSAT> static void parse_DIMACS_main(B &in, MaxSAT *S)
{
  vec<Lit> lits;
  int hardWeight = INT32_MAX;
  for (;;)
  {
    skipWhitespace(in);
    if (*in == EOF)
      break;
    else if (*in == 'p')
    {
      if (eagerMatch(in, "p cnf"))
      {
        parseInt(in); // Variables
        parseInt(in); // Clauses
      }
      else if (eagerMatch(in, "wcnf"))
      {
        S->setProblemType(_WEIGHTED_);
        parseInt(in); // Variables
        parseInt(in); // Clauses
        if (*in != '\r' && *in != '\n')
        {
          hardWeight = parseWeight(in);
          S->setHardWeight(hardWeight);
        }
      }
      else
        printf("c PARSE ERROR! Unexpected char: %c\n", *in),
            printf("c UNKNOWN\n"), exit(_ERROR_);
    }
    else if (*in == 'c' || *in == 'p')
      skipLine(in);
    else
    {
      int64_t weight = readClause(in, S, lits);
      if (weight != hardWeight || S->getProblemType() == _UNWEIGHTED_)
      {
        // Currently the weight is being represented by 'int'.
        // If soft clauses have weights larger than 2^32 this
        // can become an issue.
        assert(weight < INT32_MAX);
        assert(weight > 0);
        // Updates the maximum weight of soft clauses.
        S->setCurrentWeight((int)weight);
        // Updates the sum of the weights of soft clauses.
        S->updateSumWeights((int)weight);
        S->addSoftClause((int)weight, lits);
      }
      else
        S->addHardClause(lits);
    }
  }
}

// Inserts problem into solver.
//
template <class MaxSAT> static void parse_DIMACS(gzFile input_stream, MaxSAT *S)
{
  StreamBuffer in(input_stream);
  parse_DIMACS_main(in, S);
  if (S->getCurrentWeight() == 1)
    S->setProblemType(_UNWEIGHTED_);
  else
    S->setProblemType(_WEIGHTED_);
}

//=================================================================================================
}

#endif

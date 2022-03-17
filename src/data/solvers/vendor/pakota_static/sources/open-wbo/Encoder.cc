/*****************************************************************************************[Encoder.cc]
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

#include "Encoder.h"

using namespace NSPACE;

/************************************************************************************************
 //
 // Encoding of exactly-one constraints
 //
 ************************************************************************************************/
void Encoder::encodeAMO(Solver *S, vec<Lit> &lits)
{

  switch (amo_encoding)
  {
  // Currently only the ladder encoding is used for AMO constraints.
  case _AMO_LADDER_:
    ladder.encode(S, lits);
    break;

  default:
    printf("Error: Invalid at-most-one encoding.\n");
    printf("s UNKNOWN\n");
    exit(_ERROR_);
  }
}

/************************************************************************************************
 //
 // Encoding of cardinality constraints
 //
 ************************************************************************************************/
//
// Manages the encoding of cardinality encodings.
void Encoder::encodeCardinality(Solver *S, vec<Lit> &lits, int64_t rhs)
{
  switch (cardinality_encoding)
  {
  case _CARD_TOTALIZER_:
    totalizer.build(S, lits, rhs);
    if (totalizer.hasCreatedEncoding()) totalizer.update(S, rhs);
    break;

  case _CARD_MTOTALIZER_:
    mtotalizer.encode(S, lits, rhs);
    break;

  case _CARD_CNETWORKS_:
    cnetworks.encode(S, lits, rhs);
    break;

  default:
    printf("Error: Invalid cardinality encoding.\n");
    printf("s UNKNOWN\n");
    exit(_ERROR_);
  }
}

// Manages the update of cardinality constraints.
void Encoder::updateCardinality(Solver *S, int64_t rhs)
{

  switch (cardinality_encoding)
  {
  case _CARD_TOTALIZER_:
    totalizer.update(S, rhs);
    break;

  case _CARD_MTOTALIZER_:
    mtotalizer.update(S, rhs);
    break;

  case _CARD_CNETWORKS_:
    cnetworks.update(S, rhs);
    break;

  default:
    printf("Error: Invalid cardinality encoding.\n");
    printf("s UNKNOWN\n");
    exit(_ERROR_);
  }
}

// Incremental methods for cardinality encodings:
//
// Manages the building of cardinality encodings.
// Currently is only used for incremental solving.
void Encoder::buildCardinality(Solver *S, vec<Lit> &lits, int64_t rhs)
{
  assert(incremental_strategy != _INCREMENTAL_NONE_);

  switch (cardinality_encoding)
  {
  case _CARD_TOTALIZER_:
    totalizer.build(S, lits, rhs);
    break;

  default:
    printf("Error: Cardinality encoding does not support incrementality.\n");
    printf("s UNKNOWN\n");
    exit(_ERROR_);
  }
}

// Manages the incremental update of cardinality constraints.
void Encoder::incUpdateCardinality(Solver *S, vec<Lit> &join, vec<Lit> &lits,
                                   int64_t rhs, vec<Lit> &assumptions)
{
  assert(incremental_strategy == _INCREMENTAL_ITERATIVE_);

  switch (cardinality_encoding)
  {
  case _CARD_TOTALIZER_:
    if (join.size() > 0) totalizer.join(S, join, rhs);

    assert(lits.size() > 0);
    totalizer.update(S, rhs, lits, assumptions);
    break;

  default:
    printf("Error: Cardinality encoding does not support incrementality.\n");
    printf("s UNKNOWN\n");
    exit(_ERROR_);
  }
}

/************************************************************************************************
 //
 // Encoding of pseudo-Boolean constraints
 //
 ************************************************************************************************/
//
// Manages the encoding of PB encodings.
void Encoder::encodePB(Solver *S, vec<Lit> &lits, vec<int> &coeffs, int64_t rhs)
{

  switch (pb_encoding)
  {
  case _PB_SWC_:
    swc.encode(S, lits, coeffs, rhs);
    break;

  default:
    printf("Error: Invalid PB encoding.\n");
    printf("s UNKNOWN\n");
    exit(_ERROR_);
  }
}

// Manages the update of PB encodings.
void Encoder::updatePB(Solver *S, int64_t rhs)
{

  switch (pb_encoding)
  {
  case _PB_SWC_:
    swc.update(S, rhs);
    break;

  default:
    printf("Error: Invalid PB encoding.\n");
    printf("s UNKNOWN\n");
    exit(_ERROR_);
  }
}

// Incremental methods for PB encodings:
//
// Manages the incremental encode of PB encodings.
void Encoder::incEncodePB(Solver *S, vec<Lit> &lits, vec<int> &coeffs,
                          int64_t rhs, vec<Lit> &assumptions, int size)
{
  assert(incremental_strategy == _INCREMENTAL_ITERATIVE_);

  switch (pb_encoding)
  {
  case _PB_SWC_:
    swc.encode(S, lits, coeffs, rhs, assumptions, size);
    break;

  default:
    printf("Error: PB encoding does not support incrementality.\n");
    printf("s UNKNOWN\n");
    exit(_ERROR_);
  }
}

// Manages the incremental update of PB encodings.
void Encoder::incUpdatePB(Solver *S, vec<Lit> &lits, vec<int> &coeffs,
                          int64_t rhs, vec<Lit> &assumptions)
{
  assert(incremental_strategy == _INCREMENTAL_ITERATIVE_);

  switch (pb_encoding)
  {
  case _PB_SWC_:
    swc.update(S, rhs, assumptions);
    swc.join(S, lits, coeffs, assumptions);
    break;

  default:
    printf("Error: PB encoding does not support incrementality.\n");
    printf("s UNKNOWN\n");
    exit(_ERROR_);
  }
}

// Manages the incremental update of assumptions.
// Currently only used for the iterative encoding with SWC.
void Encoder::incUpdatePBAssumptions(Solver *S, vec<Lit> &assumptions)
{
  assert(incremental_strategy == _INCREMENTAL_ITERATIVE_);

  switch (pb_encoding)
  {
  case _PB_SWC_:
    swc.updateAssumptions(S, assumptions);
    break;

  default:
    printf("Error: PB encoding does not support incrementality.\n");
    printf("s UNKNOWN\n");
    exit(_ERROR_);
  }
}

/************************************************************************************************
 //
 // Other
 //
 ************************************************************************************************/
// Returns true if the cardinality encoding was built, false otherwise.
bool Encoder::hasCardEncoding()
{

  if (cardinality_encoding == _CARD_TOTALIZER_)
    return totalizer.hasCreatedEncoding();
  else if (cardinality_encoding == _CARD_MTOTALIZER_)
    return mtotalizer.hasCreatedEncoding();
  else if (cardinality_encoding == _CARD_CNETWORKS_)
    return cnetworks.hasCreatedEncoding();

  return false;
}

// Returns true if the PB encoding was built, false otherwise.
bool Encoder::hasPBEncoding()
{
  if (pb_encoding == _PB_SWC_) return swc.hasCreatedEncoding();

  return false;
}

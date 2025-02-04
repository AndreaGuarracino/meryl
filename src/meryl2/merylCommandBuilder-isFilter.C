
/******************************************************************************
 *
 *  This file is part of meryl, a genomic k-kmer counter with nice features.
 *
 *  This software is based on:
 *    'Canu' v2.0              (https://github.com/marbl/canu)
 *  which is based on:
 *    'Celera Assembler' r4587 (http://wgs-assembler.sourceforge.net)
 *    the 'kmer package' r1994 (http://kmer.sourceforge.net)
 *
 *  Except as indicated otherwise, this is a 'United States Government Work',
 *  and is released in the public domain.
 *
 *  File 'README.licenses' in the root directory of this distribution
 *  contains full conditions and disclaimers.
 */

#include "meryl.H"




//  Returns 0 if the next letters are not a relation symbol.
//  Otherwise returns the length of the relation symbols found.
//
uint32
merylCommandBuilder::isRelation(uint32 bgn) {

  if (((_optString[bgn] == '=') && (_optString[bgn+1] == '=')) ||
      ((_optString[bgn] == 'e') && (_optString[bgn+1] == 'q')) ||
      ((_optString[bgn] == '!') && (_optString[bgn+1] == '=')) ||
      ((_optString[bgn] == '<') && (_optString[bgn+1] == '>')) ||
      ((_optString[bgn] == 'n') && (_optString[bgn+1] == 'e')) ||
      ((_optString[bgn] == '<') && (_optString[bgn+1] == '=')) ||
      ((_optString[bgn] == 'l') && (_optString[bgn+1] == 'e')) ||
      ((_optString[bgn] == '>') && (_optString[bgn+1] == '=')) ||
      ((_optString[bgn] == 'g') && (_optString[bgn+1] == 'e')) ||
      ((_optString[bgn] == 'l') && (_optString[bgn+1] == 't')) ||
      ((_optString[bgn] == 'g') && (_optString[bgn+1] == 't')))
    return(2);

  if ((_optString[bgn] == '=') ||
      (_optString[bgn] == '<') ||
      (_optString[bgn] == '>'))
    return(1);

  return(0);
}


//  Returns the relation indicated by the next letters, or adds an error to
//  err.
//
//  The error only triggers if the relation is completely bogus - the three
//  single-letter relations will match bogus stuff like '451,<625' which in
//  turn will fail to decode number '451,'.
//
merylFilterRelation
merylCommandBuilder::decodeRelation(uint32 bgn) {
  merylFilterRelation  relation = merylFilterRelation::isNOP;

  if      (strncmp(_optString+bgn, "==", 2) == 0)   { relation = merylFilterRelation::isEq;  }
  else if (strncmp(_optString+bgn, "=",  1) == 0)   { relation = merylFilterRelation::isEq;  }
  else if (strncmp(_optString+bgn, "eq", 2) == 0)   { relation = merylFilterRelation::isEq;  }

  else if (strncmp(_optString+bgn, "!=", 2) == 0)   { relation = merylFilterRelation::isNeq; }
  else if (strncmp(_optString+bgn, "<>", 2) == 0)   { relation = merylFilterRelation::isNeq; }
  else if (strncmp(_optString+bgn, "ne", 2) == 0)   { relation = merylFilterRelation::isNeq; }

  else if (strncmp(_optString+bgn, "<=", 2) == 0)   { relation = merylFilterRelation::isLeq; }
  else if (strncmp(_optString+bgn, "le", 2) == 0)   { relation = merylFilterRelation::isLeq; }
  else if (strncmp(_optString+bgn, ">=", 2) == 0)   { relation = merylFilterRelation::isGeq; }
  else if (strncmp(_optString+bgn, "ge", 2) == 0)   { relation = merylFilterRelation::isGeq; }

  else if (strncmp(_optString+bgn, "<",  1) == 0)   { relation = merylFilterRelation::isLt;  }
  else if (strncmp(_optString+bgn, "lt", 2) == 0)   { relation = merylFilterRelation::isLt;  }
  else if (strncmp(_optString+bgn, ">",  1) == 0)   { relation = merylFilterRelation::isGt;  }
  else if (strncmp(_optString+bgn, "gt", 2) == 0)   { relation = merylFilterRelation::isGt;  }

  else {
    sprintf(_errors, "No comparison operator found in '%s',", _optString);
    sprintf(_errors, "  expecting one of '==', 'eq', '!=', 'ge', '<', etc.");
    sprintf(_errors, "");
  }

  return(relation);
}



void
merylCommandBuilder::decodeFilter(uint32 bgn, merylFilter &f) {

  //  Output values.  Each arg is either a db-index or a constant.

  uint32   index1 = uint32max,  index2 = uint32max;
  uint64   const1 = 0,          const2 = 0;

  //  Find pointers to the various bits of the string.

  uint32 arg1b = bgn;                         //  arg1 is easy, we're already there.
  if (_optString[arg1b] == ':')               //  Just ensure that the driver did indeed
    arg1b++;                                  //  skip past the first ':'.

  uint32 relab = arg1b;                       //  Search for the relation.
  while ((_optString[relab] != 0) &&
         (isRelation(relab) == 0))
    relab++;

  uint32 arg1e = relab;                       //  The end of arg1 is the beginning of the relation,
  if (_optString[arg1e-1] == ':')             //  unless there is a ':' involved.
    arg1e--;

  uint32 arg2b = relab + isRelation(relab);   //  arg2 starts after the relation and any ':'.
  if (_optString[arg2b] == ':')
    arg2b++;

  uint32 arg2e = arg2b;                       //  Keep scanning until we hit the
  while (_optString[arg2e] != 0)              //  end-of-string.
    arg2e++;

  //  Debug.
#if 1
  fprintf(stderr, "decodeFilter()- WORD          '%s'\n",       _optString);
  fprintf(stderr, "decodeFilter()- ARG1  %3d-%3d '%s'\n", arg1b, arg1e, _optString + arg1b);
  fprintf(stderr, "decodeFilter()- RELA  %3d     '%s'\n", relab,        _optString + relab);
  fprintf(stderr, "decodeFilter()- ARG2  %3d-%3d '%s'\n", arg2b, arg2e, _optString + arg2b);
#endif

  //  Decode 'arg1' if one exists.  If it doesn't exist, arg1 will be the
  //  same as rela.

  if      (arg1b == relab) {
    index1 = 0;
  }
  else if (_optString[arg1b] == '@') {
    decodeInteger(_optString, arg1b+1, arg1e, index1, _errors);
  }  
  else if (_optString[arg1b] == '#') {
    decodeInteger(_optString, arg1b+1, arg1e, const1, _errors);
  }
  else {
    decodeInteger(_optString, arg1b+0, arg1e, const1, _errors);
  }

  //  Decode the relation.

  f._r = decodeRelation(relab);

  //  Decode 'arg2' if one exists.

  if      (arg2b == arg2e) {
    sprintf(_errors, "Invalid filter '%s': no second argument to comparison operator found.", _optString);
    sprintf(_errors, "");
  }
  else if (_optString[arg2b] == '@') {
    decodeInteger(_optString, arg2b+1, arg2e, index2, _errors);
  }
  else if (_optString[arg2b] == '#') {
    decodeInteger(_optString, arg2b+1, arg2e, const2, _errors);
  }
  else if (strncmp(_optString+arg2b, "distinct=", 9) == 0) {
    f._vValue2Distinct = strtodouble(_optString+arg2b+9);
  }
  else if (strncmp(_optString+arg2b, "word-freq=", 10) == 0) {
    f._vValue2WordFreq = strtodouble(_optString+arg2b+10);
  }
  else if (strncmp(_optString+arg2b, "word-frequency=", 15) == 0) {
    f._vValue2WordFreq = strtodouble(_optString+arg2b+15);
  }
  else if (strncmp(_optString+arg2b, "threshold=", 10) == 0) {
    decodeInteger(_optString, arg2b+10, arg2e, const2, _errors);
  }
  else {
    decodeInteger(_optString, arg2b+0, arg2e, const2, _errors);
  }

  //  Set the index and constants in the filter.
  //
  //  If an index was not specified, we'll set vIndex to uint32max (the
  //  default value for index1 and index2), which is code for 'use the
  //  constant', and the correct constant will be set to whatever was
  //  specified.
  //
  //  If an index is specified, index1 (index2) will not be uint32max, code
  //  for 'use the value from the kmer in database i; the output kmer if i=0`
  //  and the constants will be at their default value of zero.
  //
  //  Finally, if the two indexes are the same, the filter is constant true
  //  of false.  They're either both uint32max, and so the filter is
  //  comparing two constants, or both database indeces refering to the same
  //  kmer.

  f._vIndex1 = index1;
  f._vIndex2 = index2;

  if (f._vIndex1 == f._vIndex2) {
    sprintf(_errors, "Invalid filter '%s': always true (or false).", _optString);
    sprintf(_errors, "");
  }

  switch (f._q) {
    case merylFilterQuantity::isValue:
      f._vValue1 = const1;
      f._vValue2 = const2;
      break;
    case merylFilterQuantity::isLabel:
      f._vLabel1 = const1;
      f._vLabel2 = const2;
      break;
    case merylFilterQuantity::isBases:
      f._vBases1 = const1;
      f._vBases2 = const2;
      break;
    //case merylFilterQuantity::isIndex:
    //  f._vIndex1 = const1;
    //  f._vIndex2 = const2;
    //  break;
    default:
      assert(0);
      break;
  }
}





//  Decide if _optString is a value filter.  If so, decode the
//  stuff and add it to the current filter product term.
//
//  Value filters can look like (omitting the spaces):
//    value:          OP constant   - both of these use an implicit @1 on the
//    value:          OP @index     - left side; and are 'more natural'
//
//    value: @index   OP constant   - these allow crazy stuff like comparing
//    value: @index   OP @index     - @3<@2 then outputting the value from @1
//    value: constant OP @index     - 
//
//    value: constant OP            - technically will complete the set, but
//    value: @index   OP            - seem awkward to use
//  
bool
merylCommandBuilder::isValueFilter(void) {

  if (strncmp(_optString, "value:", 6) != 0)
    return(false);

  merylFilter  f(merylFilterQuantity::isValue,
                 merylFilterRelation::isNOP,
                 _invertNextFilter,
                 _optString);

  decodeFilter(6, f);

  getCurrent()->addFilterToProduct(f);

  return(true);
}


//  Decide if _optString is a label filter.  If so, decode the
//  stuff and add it to the current filter product term.
//
//  Label filters look like value filters.
//
bool
merylCommandBuilder::isLabelFilter(void) {

  if (strncmp(_optString, "label:", 6) != 0)
    return(false);

  merylFilter  f(merylFilterQuantity::isLabel,
                 merylFilterRelation::isNOP,
                 _invertNextFilter,
                 _optString);

  decodeFilter(6, f);

  getCurrent()->addFilterToProduct(f);

  return(true);
}



//  Decide if _optString is a base content filter.  If so, decode the
//  stuff and add it to the current filter product term.
//
//  Base content filters are slightly different than value and label filters
//  as they also specify what bases to count, and it makes no sense to
//  compare kmers in different databases (they are all the same).
//    bases:acgt: OP constant
//
bool
merylCommandBuilder::isBasesFilter(void) {

  if (strncmp(_optString, "bases:", 6) != 0)
    return(false);

  merylFilter  f(merylFilterQuantity::isBases,
                 merylFilterRelation::isNOP,
                 _invertNextFilter,
                 _optString);

  //  Decode the bases string itself.

  uint32  bpos = 6;

  while ((_optString[bpos] != ':') && (_optString[bpos] != 0)) {
    switch (_optString[bpos]) {
      case 'a':
      case 'A':
        f._countA = true;
        break;
      case 'c':
      case 'C':
        f._countC = true;
        break;
      case 't':
      case 'T':
        f._countT = true;
        break;
      case 'g':
      case 'G':
        f._countG = true;
        break;
      default:
        sprintf(_errors, "Invalid 'bases' letter in filter '%s'.", _optString);
        sprintf(_errors, "");
        break;
    }

    bpos++;
  }

  if (_optString[bpos] != ':') {
    sprintf(_errors, "Failed to parse 'bases' filter '%s'.", _optString);
    sprintf(_errors, "");
    return(true);
  }

  //  Pass the rest of the string to the usual filter decoding to get the
  //  operation and constant.

  decodeFilter(bpos, f);

  //  Make sure the user didn't specify a useless index.
  //
  //  vIndex must be either 0 (use the output kmer) or uint32max (use the constant).
  //  vIndex is 0 if nothing is supplied for this side: "bases:acgt:ge4" will set vIndex1 to 0.

  if      ((f._vIndex1 != uint32max) &&
           (f._vIndex1  > 0)) {
    sprintf(_errors, "filter '%s' right hand side cannot specify a database input.", _optString);
    sprintf(_errors, "");
  }

  if      ((f._vIndex2 != uint32max) &&
           (f._vIndex2  > 0)) {
    sprintf(_errors, "filter '%s' left hand side cannot specify a database input.", _optString);
    sprintf(_errors, "");
  }
  
  getCurrent()->addFilterToProduct(f);

  return(true);
}



bool
merylCommandBuilder::isInputFilter(void) {

  if (strncmp(_optString, "input:", 6) != 0)
    return(false);

  merylFilter  f(merylFilterQuantity::isIndex,
                 merylFilterRelation::isNOP,
                 _invertNextFilter,
                 _optString);

  //  The 'input' filter is a ':' or ',' separated list specifying how
  //  many and which input databases a kmer must be present in.
  //
  //  How many input databases the kmer must be present in:
  //    'n'      = in exactly n files
  //    'n-m'    = in between n and m inclusive
  //    'all'    - in all
  //    'any'    - in any number (== '1-all', the default)
  //    'n-all'  = in at least n
  //
  //  Which input files the kmer must be present in:
  //    'first'  - in the first input file (== '@1')
  //    '@n'     = in the nth input file
  //    '@n-@m'  = in input files n-m inclusive
  //

  for (uint32 ii=0; ii<_optStringLen; ii++)
    if (_optString[ii] == ',')
      _optString[ii] = ':';

  splitToWords  W(_optString, ':');

  for (uint32 ww=1; ww<W.numWords(); ww++) {     //  Skip the first word; it is 'input'.
    splitToWords  P(W[ww], '-');

    //  Kmer must be present in all input databases.
    if      (strcmp(W[ww], "all") == 0) {
      //fprintf(stderr, "PUSH input_num_all\n");
      f._input_num_all = true;
    }

    //  Kmer must be present in any number of input databases.
    //  This is the default if nothing else is specified.
    else if (strcmp(W[ww], "any") == 0) {
      //fprintf(stderr, "PUSH input_num_any\n");
      f._input_num_any = true;
    }

    //  Kmer must be present in the first database.
    //  Equivalent to @1, and implemented as such.
    else if (strcmp(W[ww], "first") == 0) {
      //fprintf(stderr, "PUSH input_idx 1\n");
      f._input_idx.push_back(1);
    }

    //  @a: Kmer must be present in a specific input file.
    else if ((P.numWords() == 1) &&
             (P[0][0] == '@') && (isDecInteger(P[0]+1) == true)) {
      uint32 a = strtouint32(P[0]+1);

      //fprintf(stderr, "PUSH input_idx a   <- %u\n", a);
      f._input_idx.push_back(a);
    }

    //  @a-@b:  Kmer must be present in input files a through b, inclusive.
    else if ((P.numWords() == 2) &&
             (P[0][0] == '@') && (isDecInteger(P[0]+1) == true) &&
             (P[1][0] == '@') && (isDecInteger(P[1]+1) == true)) {
      uint32 a = strtouint32(P[0]+1);
      uint32 b = strtouint32(P[1]+1);

      for (uint32 x=a; x<=b; x++) {
        //fprintf(stderr, "PUSH input_idx a-b <- %u\n", x);
        f._input_idx.push_back(x);
      }
    }

    //  a:  Kmer must occur in a input files.
    else if ((P.numWords() == 1) &&
             (isDecInteger(P[0]) == true)) {
      uint32 a = strtouint32(P[0]);

      //fprintf(stderr, "PUSH a   input_num <- %u\n", a);
      f._input_num.push_back(a);
    }

    //  a-b:  Kmer must occur in between a and b input files, inclusive.
    else if ((P.numWords() == 2) &&
             (isDecInteger(P[0]) == true) &&
             (isDecInteger(P[1]) == true)) {
      uint32 a = strtouint32(P[0]);
      uint32 b = strtouint32(P[1]);

      for (uint32 x=a; x<=b; x++) {
        //fprintf(stderr, "PUSH a-b input_num <- %u\n", x);
        f._input_num.push_back(x);
      }
    }

    //  a-all:  Kmer must occur in at least a input files.
    else if ((P.numWords() == 2) &&
             (isDecInteger(P[0]) == true) &&
             (strcmp(P[1], "all") == 0)) {
      uint32 a = strtouint32(P[0]);

      //fprintf(stderr, "PUSH input_num_at_least <- u\n", a);
      f._input_num_at_least = std::min(a, f._input_num_at_least);
    }

    else {
      sprintf(_errors, "filter '%s' cannot be decoded: unknown word '%s'.", _optString, W[ww]);
      sprintf(_errors, "");
    }
  }

  getCurrent()->addFilterToProduct(f);

  return(true);
}



//  Process any 'and', 'or' or 'not' filter connectives.
//
bool
merylCommandBuilder::isFilterConnective(void) {

  //  The word 'not' will invert the sense of the next filter.
  //
  if (strcmp(_optString, "not") == 0) {
    _invertNextFilter = !_invertNextFilter;
    return(true);
  }

  //  The word 'and' is just syntactic sugar.  We don't need or use it.
  if (strcmp(_optString, "and") == 0) {
    return(true);
  }

  //  The word 'or' tells us to make a new product term.
  if (strcmp(_optString, "or") == 0) {
    if (getCurrent()->addNewFilterProduct()) {
      sprintf(_errors, "attempt to add new filter product when existing filter product is empty.");
      sprintf(_errors, "");
    }
    return(true);
  }

  //  Nope, not a connective.
  return(false);
}



//  Handle all filter related words.
//
//  This fails to detect some errors in the command line:
//    f1 and and f2
//    f1 and or  f2
//    f1 not and f2
//    f1 or
//
//  But does detect a few:
//    or f1
//    f1 or or f2
//
bool
merylCommandBuilder::isFilter(void) {

  //  If we find a filter word, process it and then reset
  //  the invert flag.
  //
  if ((isValueFilter()      == true) ||   //  Consumes 'value:' filters
      (isLabelFilter()      == true) ||   //  Consumes 'label:' filters
      (isBasesFilter()      == true) ||   //  Consumes 'bases:' filters
      (isInputFilter()      == true)) {   //  Consumes 'input:' filters
    _invertNextFilter = false;
    return(true);
  }

  //  Not a filter word.  Check if it's a connective.

  if (isFilterConnective() == true)       //  Consumes 'and', 'or', 'not'
    return(true);

  return(false);
}

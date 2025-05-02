# include "ohana.h"

/* local private functions */
char **InsertValue (char **myOutput, unsigned int *Nout, unsigned int *Nchar, unsigned int *NCHAR, char c);
char **EndOfString (char **myOutput, unsigned int *Nout, unsigned int *Nchar, unsigned int *NOUT, unsigned int *NCHAR);
int IsAnOp (char *c);
int IsTwoOp (char *c);

char **isolate_elements (unsigned int Nin, char **in, unsigned int *nout) {
  
  /* local private static variables */
  unsigned int NCHAR, Nchar, Nout, NOUT;
  char **myOutput;

  unsigned int i, j, minus, negate, plus, posate, OpStat, SciNotation;

  NOUT = Nin;
  Nchar = Nout = 0;
  NCHAR = 256;
  ALLOCATE (myOutput, char *, NOUT);
  ALLOCATE (myOutput[Nout], char, NCHAR);

  for (i = 0; i < Nin; i++) {
    for (j = 0; j < strlen(in[i]); j++) {
      SciNotation = FALSE;
      /* identify 'negate' or 'minus' ops */
      negate = minus = FALSE;
      if (in[i][j] == '-') { 
	minus = TRUE;  
	/* if - is first thing on line, must be a negator */
	if ((Nout == 0) && (Nchar == 0)) {  
	  minus = FALSE;
	  negate = TRUE;
	  goto skip1;
	}
	/* check previous entry on line */
	if (Nchar) {
	  OpStat = IsAnOp (myOutput[Nout]);
	  if (myOutput[Nout][0] == ')') OpStat = FALSE;
	} else {
	  OpStat = IsAnOp (myOutput[Nout-1]);
	  if (myOutput[Nout-1][0] == ')') OpStat = FALSE;
	}
	/* if - follows an operator, must be negator */
	if (OpStat) {
	  minus = FALSE;
	  negate = TRUE;
	  goto skip1;
	}
	/* if - follows 'e' is part of 1e-5 */
	if (j == 0) goto skip1;
	if ((in[i][j-1] == 'e') || (in[i][j-1] == 'E')) {
	  SciNotation = TRUE;
	  negate = minus = FALSE;
	}
      }
    skip1:
      /* idenfity 'posate' or 'plus' ops */
      posate = plus = FALSE;
      if (in[i][j] == '+') { 
	plus = TRUE;  
	/* if + is first thing on line, must be a posator */
	if ((Nout == 0) && (Nchar == 0)) {  
	  plus = FALSE;
	  posate = TRUE;
	  goto skip2;
	}
	/* check previous entry on line */
	if (Nchar) {
	  OpStat = IsAnOp (myOutput[Nout]);
	  if (myOutput[Nout][0] == ')') OpStat = FALSE;
	} else {
	  OpStat = IsAnOp (myOutput[Nout-1]);
	  if (myOutput[Nout-1][0] == ')') OpStat = FALSE;
	}
	/* if + follows an operator, must be posator */
	if (OpStat) {
	  plus = FALSE;
	  posate = TRUE;
	  goto skip2;
	}
	/* if + follows 'e' is part of 1e+5 */
	if (j == 0) goto skip2;
	if ((in[i][j-1] == 'e') || (in[i][j-1] == 'E')) {
	  SciNotation = TRUE;
	  posate = plus = FALSE;
	}
      }
    skip2:
      /* operators */
      if (negate || minus || posate || plus || (IsAnOp (&in[i][j]) && !SciNotation)) {
	if (posate) continue;
	myOutput = EndOfString (myOutput, &Nout, &Nchar, &NOUT, &NCHAR);
	/* copy operator to myOutput[Nout] */
	myOutput = InsertValue (myOutput, &Nout, &Nchar, &NCHAR, in[i][j]);
	if (negate) {
	  myOutput = InsertValue (myOutput, &Nout, &Nchar, &NCHAR, '-');
	}

	if (IsTwoOp (&in[i][j])) {
	  myOutput = InsertValue (myOutput, &Nout, &Nchar, &NCHAR, in[i][j+1]);
	  j++;
	} 
	myOutput = EndOfString (myOutput, &Nout, &Nchar, &NOUT, &NCHAR);
	continue;
      }
      /* quoted string */
      if (in[i][j] == '"') {
	myOutput = InsertValue (myOutput, &Nout, &Nchar, &NCHAR, in[i][j]);
	j++;
	while ((j < strlen(in[i])) && (in[i][j] != '"')) {
	  myOutput = InsertValue (myOutput, &Nout, &Nchar, &NCHAR, in[i][j]);
	  j++;
	}
	if (in[i][j] != '"') continue;
	/* 
	  gprint (GP_ERR, "mismatched quotes\n");
	  return (FALSE);
	}
	*/
	myOutput = InsertValue (myOutput, &Nout, &Nchar, &NCHAR, in[i][j]);
	myOutput = EndOfString (myOutput, &Nout, &Nchar, &NOUT, &NCHAR);
	continue;
      }
      /* not an operator, not a quoted string */
      if (!OHANA_WHITESPACE (in[i][j])) {
	myOutput = InsertValue (myOutput, &Nout, &Nchar, &NCHAR, in[i][j]);
      } else {
	myOutput = EndOfString (myOutput, &Nout, &Nchar, &NOUT, &NCHAR);
      }
    }
    myOutput = EndOfString (myOutput, &Nout, &Nchar, &NOUT, &NCHAR);
  }

  /* one extra entry is allocated, free here */
  free (myOutput[Nout]);
  *nout = Nout;
  return (myOutput);

}

char **InsertValue (char **myOutput, unsigned int *Nout, unsigned int *Nchar, unsigned int *NCHAR, char c) {
  myOutput[*Nout][*Nchar] = c;
  (*Nchar) ++;
  if ((*Nchar) >= (*NCHAR) - 2) {
    (*NCHAR) += 256;
    REALLOCATE (myOutput[*Nout], char, *NCHAR);
  }
  myOutput[*Nout][*Nchar] = 0;
  return (myOutput);
}

char **EndOfString (char **myOutput, unsigned int *Nout, unsigned int *Nchar, unsigned int *NOUT, unsigned int *NCHAR) {
  if ((*Nchar) > 0) {
    myOutput[*Nout][*Nchar] = 0;
    (*Nout) ++;
    (*Nchar) = 0;
    
    if ((*Nout) >= (*NOUT) - 1) {
      (*NOUT) += 10; 
      REALLOCATE (myOutput, char *, (*NOUT)); 
    } 
    (*NCHAR) = 256;
    ALLOCATE (myOutput[*Nout], char, (*NCHAR)); 
  }
  return (myOutput);
}

int IsAnOp (char *c) {

  // order matches convert_to_RPN.c
  if (!strncmp (c, "?",  1)) return (TRUE);

  // if (!strncmp (c, ":",  1)) return (TRUE);

  // do not include : in this list: an unisolated colon acts as a modifier

  if (!strncmp (c, "^",  1)) return (TRUE);

  if (!strncmp (c, ",",  1)) return (TRUE);
  if (!strncmp (c, "@",  1)) return (TRUE);
  if (!strncmp (c, "/",  1)) return (TRUE);
  if (!strncmp (c, "*",  1)) return (TRUE);
  if (!strncmp (c, "%",  1)) return (TRUE);

  if (!strncmp (c, "+",  1)) return (TRUE);
  if (!strncmp (c, "-",  1)) return (TRUE);

  if (!strncmp (c, "(",  1)) return (TRUE);
  if (!strncmp (c, ")",  1)) return (TRUE);

  if (!strncmp (c, "<<", 2)) return (TRUE);
  if (!strncmp (c, ">>", 2)) return (TRUE);
  if (!strncmp (c, "&&", 2)) return (TRUE);
  if (!strncmp (c, "||", 2)) return (TRUE);
  if (!strncmp (c, "==", 2)) return (TRUE);
  if (!strncmp (c, "!=", 2)) return (TRUE);
  if (!strncmp (c, "<=", 2)) return (TRUE);
  if (!strncmp (c, ">=", 2)) return (TRUE);
  if (!strncmp (c, "<~", 2)) return (TRUE);
  if (!strncmp (c, "~>", 2)) return (TRUE);

  if (!strncmp (c, "<",  1)) return (TRUE);
  if (!strncmp (c, ">",  1)) return (TRUE);
  if (!strncmp (c, "&",  1)) return (TRUE);
  if (!strncmp (c, "|",  1)) return (TRUE);

  return (FALSE);

}

int IsTwoOp (char *c) {

  if (!strncmp (c, "<<", 2)) return (TRUE);
  if (!strncmp (c, ">>", 2)) return (TRUE);
  if (!strncmp (c, "&&", 2)) return (TRUE);
  if (!strncmp (c, "||", 2)) return (TRUE);
  if (!strncmp (c, "==", 2)) return (TRUE);
  if (!strncmp (c, "!=", 2)) return (TRUE);
  if (!strncmp (c, "<=", 2)) return (TRUE);
  if (!strncmp (c, ">=", 2)) return (TRUE);
  if (!strncmp (c, "<~", 2)) return (TRUE);
  if (!strncmp (c, "~>", 2)) return (TRUE);

  return (FALSE);

}

# include "opihi.h"

/* local private functions */
void InsertValue (char c);
void EndOfString (void);
int IsAnOp (char *c);
int IsTwoOp (char *c);

/* local private static variables */
int Nchar, Nout, NOUT;
char **out;

// split up the input arguments into appropriate blocks
char **dvoBooleanElements (int Nin, char **in, int *nout) {
  
  int i, j, minus, negate, plus, posate, OpStat, SciNotation;

  NOUT = Nin;
  Nchar = Nout = 0;
  ALLOCATE (out, char *, NOUT);
  ALLOCATE (out[Nout], char, NCHARS);

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
	  OpStat = IsAnOp (out[Nout]);
	  if (out[Nout][0] == ')') OpStat = FALSE;
	} else {
	  OpStat = IsAnOp (out[Nout-1]);
	  if (out[Nout-1][0] == ')') OpStat = FALSE;
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
	  OpStat = IsAnOp (out[Nout]);
	  if (out[Nout][0] == ')') OpStat = FALSE;
	} else {
	  OpStat = IsAnOp (out[Nout-1]);
	  if (out[Nout-1][0] == ')') OpStat = FALSE;
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
	EndOfString ();
	/* copy operator to out[Nout] */
	InsertValue (in[i][j]);
	if (negate) InsertValue ('-');

	if (IsTwoOp (&in[i][j])) {
	  InsertValue (in[i][j+1]);
	  j++;
	} 
	EndOfString ();
	continue;
      }
      /* quoted string */
      if (in[i][j] == '"') {
	InsertValue (in[i][j]);
	j++;
	while ((j < strlen(in[i])) && (in[i][j] != '"')) {
	  InsertValue (in[i][j]);
	  j++;
	}
	if (in[i][j] != '"') continue;
	/* 
	  gprint (GP_ERR, "mismatched quotes\n");
	  return (FALSE);
	}
	*/
	InsertValue (in[i][j]);
	EndOfString ();
	continue;
      }
      /* not an operator, not a quoted string */
      if (!OHANA_WHITESPACE (in[i][j])) {
	InsertValue (in[i][j]);
      } else {
	EndOfString ();
      }
    }
    EndOfString ();
  }

  /* one extra entry is allocated, free here */
  free (out[Nout]);
  *nout = Nout;
  return (out);

}

void InsertValue (char c) {
  out[Nout][Nchar] = c;
  Nchar ++;
  out[Nout][Nchar] = 0;
}

void EndOfString () {
  if (Nchar > 0) {
    out[Nout][Nchar] = 0;
    Nout ++;
    Nchar = 0;
    
    if (Nout >= NOUT - 1) {
      NOUT += 10; 
      REALLOCATE (out, char *, NOUT); 
    } 
    ALLOCATE (out[Nout], char, NCHARS); 
  }
}

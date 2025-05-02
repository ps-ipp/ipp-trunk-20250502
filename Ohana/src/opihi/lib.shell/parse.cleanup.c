# include "opihi.h"

enum {OP_SET_SCALAR, OP_INCR_SCALAR, OP_DECR_SCALAR, OP_ADD_SCALAR, OP_SUB_SCALAR, OP_SET_VECTOR, OP_INCR_VECTOR, OP_DECR_VECTOR, OP_ADD_VECTOR, OP_SUB_VECTOR};

char *parse (int *status, char *line) {

  double fval;
  float *fptr;
  char *newline, *N, *L, *val, *B, *V, *V0, *V1, *c1, *c2, *p, *px, *py;
  int Nx, Ny, Nbytes, filestatus, size, NLINE, isBuffer, inRange;
  FILE *f;
  Vector *vec;
  Buffer *buf;

  *status = TRUE; 

  Ny = 0;
  val = V0 = NULL;

  if (line == (char *) NULL) goto error;

  // what are we trying to do here?
  /* case 1: $var = expression */
  if (line[0] == '$') {  
    /* find the target variable name and the rest of the line */
    interpolate_slash (line);
    V0 = thisvar (line);
    if (V0 == NULL) goto error;
    V1 = aftervar (line);
    if (V1 == NULL) goto error;
 
    if (!strcmp  (V1, "++"))    { mode = OP_INCR_SCALAR; }
    if (!strcmp  (V1, "--"))    { mode = OP_DECR_SCALAR; }
    if (!strncmp (V1, "+=", 2)) { mode = OP_ADD_SCALAR;  }
    if (!strncmp (V1, "-=", 2)) { mode = OP_SUB_SCALAR;  }
    if (!strcmp  (V1, "="))     { mode = OP_SET_SCALAR;  }
  }

  /* case 2: vect[N] = value or buff[N][M] = value */
  V0 = thiscomm (line);
  px = py = NULL;
  if (strchr(V0, '[') != (char *) NULL) { 
    isBuffer = FALSE;

    N = strchr (V0, '[');
    if (N == V0) goto error;

    // find end of 1st bracket
    L = strchr (V0, ']');
    if (!L) goto error;
    px = N + 1;

    // is there a second bracket?
    N = L + 1;
    if (*N == '[') {
      isBuffer = TRUE;

      // find end of 2nd bracket
      L = strchr (N, ']');
      if (!L) goto error;
      py = N + 1;
    }

    V1 = nextcomm (line);

    if (!strcmp  (V1, "++"))    { mode = OP_INCR_VECTOR; }
    if (!strcmp  (V1, "--"))    { mode = OP_DECR_VECTOR; }
    if (!strncmp (V1, "+=", 2)) { mode = OP_ADD_VECTOR;  }
    if (!strncmp (V1, "-=", 2)) { mode = OP_SUB_VECTOR;  }
    if (!strcmp  (V1, "="))     { mode = OP_SET_VECTOR;  }
  }
  free (V0); V0 = (char *) NULL;

  

  /* case 1: $var = expression */
  if (line[0] == '$') {  
    /* find the target variable name and the rest of the line */
    interpolate_slash (line);
    V0 = thisvar (line);
    if (V0 == NULL) goto error;
    V1 = aftervar (line);
    if (V1 == NULL) goto error;
 
    /* increment operator */
    if (!strcmp (V1, "++")) {
      val = get_variable (V0);
      if (val == NULL) {
	fval = 1.0;
      } else {
	fval = atof (val) + 1.0;
      }
      set_variable (V0, fval);
      goto escape;
    }

    /* decrement operator */
    if (!strcmp (V1, "--")) {
      val = get_variable (V0);
      if (val == NULL) {
	fval = -1.0;
      } else {
	fval = atof (val) - 1.0;
      }
      set_variable (V0, fval);
      goto escape;
    }

    if (!strncmp (V1, "+=", 2)) {
	V1 ++;
	if (*V1 == 0) goto error;
	V1 ++;
	if (*V1 == 0) goto error;

	val = get_variable (V0);
	if (val == NULL) {
	    fval = 0.0;
	} else {
	    fval = atof (val);
	}

	/* dvomath returns a new string, or NULL, with the result of the expression */
	val = dvomath (1, &V1, &size, 0);
	if (val == NULL) goto error;
	fval += atof(val);
	// save the result
	set_variable (V0, fval);
	goto escape;
    }

    if (!strncmp (V1, "-=", 2)) {
	V1 ++;
	if (*V1 == 0) goto error;
	V1 ++;
	if (*V1 == 0) goto error;

	val = get_variable (V0);
	if (val == NULL) {
	    fval = 0.0;
	} else {
	    fval = atof (val);
	}

	/* dvomath returns a new string, or NULL, with the result of the expression */
	val = dvomath (1, &V1, &size, 0);
	if (val == NULL) goto error;
	fval -= atof(val);
	// save the result
	set_variable (V0, fval);
	goto escape;
    }

    /* not an assignement, syntax error */
    if (*V1 != '=') goto error;

    /* find first non-WHITESPACE character after = */
    V1 ++;
    while (isspace (*V1)) V1++;
    if (*V1 == 0) {
      // assign empty vector
      set_str_variable (V0, "");
      goto escape;
    }

    /* command replacement.  execute the line, place answer in val. */
    if (*V1 == '`') {

      /* look for end of line, must be ` */
      B = V1 + strlen (V1) - 1;
      if (*B != '`') goto error;

      /* val will hold the result */
      // XXX this is limiting!!!
      ALLOCATE (val, char, 1024);

      /* B is the command to be executed */
      B = strncreate (V1 + 1, strlen(V1) - 2);
      f = popen (B, "r");
      Nbytes = fread (val, 1, 1023, f);
      val[Nbytes] = 0;
      filestatus = pclose (f);
      free (B);

      if (filestatus) gprint (GP_ERR, "warning: exit status of command %d\n", filestatus);

      /* convert all but last return to ' '.  drop last return */
      for (B = val; *B != 0; B++) {
	if (*B == '\n') {
	  if (B[1]) {
	    *B = ' ';
	  } else {
	    *B = 0;
	  }
	}
      }
      // save the resulting value
      set_str_variable (V0, val);
      goto escape;  /* frees temp variables */
    }

    /* simple variable assignment */
    /* dvomath returns a new string, or NULL, with the result of the expression */
    val = dvomath (1, &V1, &size, 0);
    if (val == NULL) { 
      while (OHANA_WHITESPACE (*V1)) V1++;
      val = strcreate (V1);
    } 
    // save the result
    set_str_variable (V0, val);
    goto escape;  /* frees temp variables */
  }

  /* case 2: vect[N] = value or buff[N][M] = value */
  V0 = thiscomm (line);
  if (strchr(V0, '[') != (char *) NULL) { 
    /* get vector name (left in V0) */
    N = strchr (V0, '[');
    if (N == V0) goto error;

    /* get bracket contents (left in V) */
    L = strchr (V0, ']');
    if (L == (char *) NULL) goto error;

    *N = 0;
    V = strncreate (N+1, L - N - 1);

    /* expand value in brackets */
    val = dvomath (1, &V, &size, 0);
    if (val == NULL) {
      print_error ();
      goto error;
    }
    Nx = atoi (val);
    free (val); val = NULL;
    free (V);

    isBuffer = FALSE;
    N = L + 1;
    if (*N == '[') {
      isBuffer = TRUE;
      L = strchr (N, ']');

      V = strncreate (N+1, L - N - 1);

      /* expand value in brackets */
      val = dvomath (1, &V, &size, 0);
      if (val == NULL) {
	print_error ();
	goto error;
      }
      Ny = atoi (val);
      free (val); val = NULL;
      free (V);
    }

    /* find value for assignment */
    V1 = nextcomm (line);
    if (V1 == (char *) NULL) goto error;
    if (V1[0] != '=') goto error;
    V1 ++;

    /*** assign vector element to value ***/
    val = dvomath (1, &V1, &size, 0); 
    if (val == (char *) NULL) {
      print_error ();
      goto error;
    }

    /* find vector */
    if (isBuffer) {
      if ((buf = SelectBuffer (V0, OLDBUFFER, TRUE)) == NULL) goto error;
      free (V0); V0 = (char *) NULL;

      inRange = TRUE;
      inRange &= (Nx <  +1*buf[0].header.Naxis[0]);
      inRange &= (Nx >= -1*buf[0].header.Naxis[0]);
      inRange &= (Ny <  +1*buf[0].header.Naxis[1]);
      inRange &= (Ny >= -1*buf[0].header.Naxis[1]);
      if (!inRange) {
	gprint (GP_ERR, "no element %d,%d\n", Nx, Ny);
	goto error;
      }
      if (Nx < 0) Nx += buf[0].header.Naxis[0];
      if (Ny < 0) Ny += buf[0].header.Naxis[1];

      fptr = (float *) buf[0].matrix.buffer;
      fptr[Nx + Ny*buf[0].header.Naxis[0]] = atof (val);
    } else {
      if ((vec = SelectVector (V0, OLDVECTOR, TRUE)) == NULL) goto error;
      free (V0); V0 = (char *) NULL;

      inRange = TRUE;
      inRange &= (Nx <  +1*vec[0].Nelements);
      inRange &= (Nx >= -1*vec[0].Nelements);
      if (!inRange) {
	gprint (GP_ERR, "no element %d\n", Nx);
	goto error;
      }
      if (Nx < 0) Nx += vec[0].Nelements;
      if (vec[0].type == OPIHI_FLT) {
	vec[0].elements.Flt[Nx] = atof (val);
      } else {
	vec[0].elements.Int[Nx] = atol (val);
      }
    }
    goto escape;
  }
  free (V0); V0 = (char *) NULL;

  /* case 3: {expression} */
  NLINE = MAX (128, strlen (line));
  ALLOCATE (newline, char, NLINE);
  memset (newline, 0, NLINE);
  for (L = line; *L != 0; ) { 

    // copy elements from L (line) to newline up to first '{'
    p = strchr (L, '{');
    newline = opihi_append (newline, &NLINE, L, p);
    if (p == NULL) break;
    L = p + 1;

    // check for \{ at this point, replace \ in newline with {
    if ((p > line) && (*(p - 1) == 0x5c)) {
      N = newline + strlen(newline) - 1;
      *N = '{'; // replace \ with {
      continue;
    }

    /* check on the syntax of the line */
    L = p + 1;
    c1 = strchr (L, '}');
    if (c1 == NULL) {
      gprint (GP_ERR, "no close brackets!\n");
      goto error;
    }
    c2 = strchr (L, '{');
    if ((c2 != NULL) && (c2 < c1)) {
      gprint (GP_ERR, "can't nest brackets!\n");
      goto error;
    }
    *c1 = 0;

    /* value in brackets must be a math expression of some sort.
       isolated words are not valid here */
    val = dvomath (1, &L, &size, -1);
    if (val == NULL) {
      print_error ();
      goto error;
    } 

    /* interpolate vector element into newline (being accumulated) */
    newline = opihi_append (newline, &NLINE, val, NULL);

    /* copy val to outline */
    L = c1 + 1;
    free (val); val = (char *) NULL;
  }

  free (line); line = (char *) NULL;

  /* \ protects next character */
  interpolate_slash (newline);
  
  REALLOCATE (newline, char, strlen (newline) + 1);
  *status = TRUE;
  return (newline); 

error:
  gprint (GP_ERR, "syntax error\n");
  // assignment or increment operation: free line and return NULL
  if (line != (char *) NULL) free (line);
  if (val != (char *) NULL) free (val);
  if (V0 != (char *) NULL) free (V0);
  *status = FALSE;
  return NULL;

escape:
  // assignment or increment operation: free line and return NULL
  if (line != (char *) NULL) free (line);
  if (val != (char *) NULL) free (val);
  if (V0 != (char *) NULL) free (V0);
  *status = TRUE;
  return (NULL);
}

/* this routine looks for math and/or logic expressions that 
   need to be evaluated and passes them on to "parenthesis",
   which evaluates the expression */

/* There are two situations now which define an expression to 
   be evaluated:
   1) $var = expression   -- everything after the = must be a math expression or a string
   2) {expression}        -- everything within the {} must be a math expression
*/

/* case 1: $var = expression.  test that
   a) there is a $ as the first character
   b) the variable defined by that $ is not null
   c) there is an = sign following the variable
   d) there is something following the = sign
*/
  
/* case 2: vect[N] = expression. check syntax:
   a) last char must be ]
   b) word must contain [
   c) between [ & ] must be an integer
   d) vector must exist
   e) there is an = sign following the first word
   f) there is something following the = sign
*/
     

/* case 3: we hunt for '{', and then the first '}' and pass everything
   inbetween.  no nested {{}}s are allowed! */
  
/* thisword ALLOCATES
   thisvar  ALLOCATES
   thiscomm ALLOCATES
   
   nextword !ALLOCATE
   nextcomm !ALLOCATE
   
   lastword !ALLOCATE
   lastvar !ALLOCATE
   
   aftervar !ALLOCATE
*/   


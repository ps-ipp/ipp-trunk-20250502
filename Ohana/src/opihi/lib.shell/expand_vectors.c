# include "opihi.h"

// XXX need to unroll this function so we can more flexibly 
// convert items in the line

// converts a[], a[4], a[2][3] to their numeric values
// add a?[] and a?[][] as equivalent to $?a
// a[][?]

enum {ANS_NONE, ANS_INT, ANS_FLT, ANS_STR};

char *expand_vectors (char *line) {

  char *newline, *tmpline, strValue[128], *val;
  char *L, *N, *p, *q, *p2, *w;
  int n, I, J, size, showLength, showXsize, showYsize, checkType, NLINE, Noff, isBuffer, inRange;
  float *ptr;
  opihi_flt valueFlt;
  opihi_int valueInt;
  char *valueStr;
  int answerType;
  Vector *vec;
  Buffer *buf;

  if (line == NULL) return (NULL);

  NLINE = MAX (128, strlen(line));
  ALLOCATE (newline, char, NLINE);

  // special case: don't expand vectors in while () statement
  char *V0 = thiscomm (line);
  if (V0 && !strncmp ("while", V0, strlen(V0))) {
      strcpy (newline, line);
      free (line);
      return (newline);
  }
  free (V0);

  /* look for form fred[stuff] */
  if (*line == '(') {
    // if line is "(stuff)" then do not skip first word
      L = line;
  } else {
    /* skip first word (command) */
    L = nextword (line);
  }
  if (L == NULL) {
    free (newline);
    return (line);
  }

  n = L - line;
  strncpy_nowarn (newline, line, n);
  N = newline + n;
  J = 0;
  while (1) {
    /* find square-bracket pair [..] */
    p = strchr (L, '[');
    q = strchr (L, ']');
    if ((p == NULL) && (q != NULL)) goto dumpline;
    if ((p != NULL) && (q == NULL)) goto dumpline;
    if ((p == NULL) && (q == NULL)) goto dumpline;
    if (p > q) goto dumpline; /* odd state: unmatched pair: ][ */

    // we have a bracket pair, now interpret the context:
    // a[N] - vector value
    // a?[] or a?[N] - vector existence
    // a[]  - vector length
    // a[N][M] - buffer value
    // a?[][], a?[N][], a?[][M] - buffer existence
    // a[][N] - buffer X size
    // a[N][] - buffer Y size

    /* find vector subscript */
    n = (int) (q - p - 1);
    val = NULL;
    showLength = FALSE;
    showXsize  = FALSE;
    showYsize  = FALSE;
    answerType = ANS_NONE;

    if (n == 0) {
      showLength = TRUE;
      showXsize  = TRUE;
    } else {
      tmpline = strncreate (p+1, n);
      val = dvomath (1, &tmpline, &size, 0);
      free (tmpline);
      if (val == NULL) goto dumpline; /* not a valid vector subscript */
    }
    I = 0; 
    if (val != NULL) {
      I = atoi (val);
      free (val);
    }      

    /* if a second [..] immediately follows, we are a buffer, not a vector */
    isBuffer = FALSE;
    if (*(q + 1) == '[') {
      p2 = strchr (q + 1, ']');
      if (p2 != NULL) {
	// if (showLength) {
	//   gprint (GP_ERR, "unsupported : name[][..]\n"); 
	//   goto asVector;
	// }

	isBuffer = TRUE;

	/* find buffer second subscript */
	n = (int) (p2 - q - 2);
	val = NULL;
	if (n == 0) {
	  showYsize = TRUE;
	  // gprint (GP_ERR, "unsupported : name[..][]\n"); 
	  // isBuffer = FALSE;
	  // goto asVector;
	}  else {
	  tmpline = strncreate (q+2, n);
	  val = dvomath (1, &tmpline, &size, 0);
	  free (tmpline);
	  if (val == NULL) {
	    isBuffer = FALSE;
	    goto asVector; /* not a valid vector subscript */
	  }
	}
	J = 0; 
	if (val != NULL) {
	  J = atoi (val);
	  free (val);
	}      
      }
      q = p2;
    }

  asVector:
    /* find vector/buffer name */
    /* 'checkType' really means check for existence.
       if 'a' is a vector, then 'a?[]' returns TRUE, else FALSE
       if 'a' is a buffer, then 'a?[][]' returns TRUE, else FALSE
    */

    checkType = FALSE;
    if (*(p-1) == '?') {
      checkType = TRUE;
      p--;
    }

    // Scan backwards for the start of the vector name.  ISVEC is defined in
    // include/shell.h.  A vector name cannot include a colon, while a variable name
    // cannot include a dot.

    for (w = p - 1; (w >= line) && !OHANA_WHITESPACE(*w) && ISVEC(*w); w--);
    w ++;
    n = (int)(p - w);
    tmpline = strncreate (w, n);

    if (isBuffer) {
      buf = SelectBuffer (tmpline, OLDBUFFER, !checkType);
      if (checkType) {
	valueInt = (buf != NULL);
	answerType = ANS_INT;
	goto skipBuffer;
      } 
      if (showXsize && showYsize) {
	gprint (GP_ERR, "ambiguous size option buf[][]\n"); 
	goto dumpline;
      }
      if (buf == NULL) goto dumpline;
      if (showXsize) {
	valueInt = buf[0].header.Naxis[0];
	answerType = ANS_INT;
	goto skipBuffer;
      }
      if (showYsize) {
	valueInt = buf[0].header.Naxis[1];
	answerType = ANS_INT;
	goto skipBuffer;
      }
      /* find buffer element */
      inRange = TRUE;
      inRange &= (I <  +1*buf[0].header.Naxis[0]);
      inRange &= (I >= -1*buf[0].header.Naxis[0]);
      inRange &= (J <  +1*buf[0].header.Naxis[1]);
      inRange &= (J >= -1*buf[0].header.Naxis[1]);
      if (!inRange) {
	gprint (GP_ERR, "buffer subscript out of range\n"); 
	goto escape;
      }
      if (I < 0) I += buf[0].header.Naxis[0];
      if (J < 0) J += buf[0].header.Naxis[1];
      ptr = (float *) buf[0].matrix.buffer;
      valueFlt = ptr[I + J*buf[0].header.Naxis[0]];
      answerType = ANS_FLT;
    } else {
      vec = SelectVector (tmpline, OLDVECTOR, !checkType);
      if (checkType) {
	valueInt = (vec != NULL);
	answerType = ANS_INT;
      } else {
	if (vec == NULL) goto dumpline;

	/* find vector element */
	if (showLength) {
	  valueInt = vec[0].Nelements;
	  answerType = ANS_INT;
	} else {
	  if ((I >= vec[0].Nelements) || (I < -1*vec[0].Nelements)) {
	    gprint (GP_ERR, "vector subscript out of range\n"); 
	    goto escape;
	  }
	  if (I < 0) I += vec[0].Nelements;

	  switch (vec[0].type) {
	    case OPIHI_FLT:
	      valueFlt = vec[0].elements.Flt[I];
	      answerType = ANS_FLT;
	      break;
	    case OPIHI_INT:
	      valueInt = vec[0].elements.Int[I];
	      answerType = ANS_INT;
	      break;
	    case OPIHI_STR:
	      valueStr = vec[0].elements.Str[I];
	      answerType = ANS_STR;
	      break;
	  }
	}
      }
    }

  skipBuffer:
    free (tmpline);

    switch (answerType) {
      case ANS_NONE:
	goto dumpline;
      case ANS_INT:
	snprintf (strValue, 128, OPIHI_INT_FMT, valueInt); 
	break;
      case ANS_FLT:
	snprintf (strValue, 128, "%.12g", valueFlt);
	break;
      case ANS_STR:
	snprintf (strValue, 128, "%s", valueStr); 
	break;
    }

    /* interpolate vector element into newline (being accumulated) */
    size = (N - newline) + (w - L) + strlen(strValue);
    if (size >= NLINE) {
      Noff = N - newline;
      NLINE += 128 + strlen(strValue) + (w - L);
      REALLOCATE (newline, char, NLINE);
      N = newline + Noff;
    }

    n = (int) (w - L);
    strncpy_nowarn (N, L, n);
    N += n;
    n = strlen (strValue);
    strncpy_nowarn (N, strValue, n);
    N += n;
    L = q + 1;
  }

dumpline:
  size = (N - newline) + strlen(L);
  if (size >= NLINE) {
    Noff = N - newline;
    NLINE += 128 + strlen(L);
    REALLOCATE (newline, char, NLINE);
    N = newline + Noff;
  }

  n = strlen (L);
  strncpy_nowarn (N, L, n);
  free (line);
  return (newline);
  
escape:
  free (line);
  free (newline);
  return (NULL);
}


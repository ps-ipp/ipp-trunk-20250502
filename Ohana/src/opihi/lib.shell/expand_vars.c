# include "opihi.h"

char *expand_vars (char *line) {

  char *L, *N, *V0, *V1, *Val, *newline, *c, found;
  int done, MacroDepth, NLINE, Noff;

  if (line == NULL) return (NULL);
  MacroDepth = GetMacroDepth ();

  found = FALSE;
  NLINE = MAX (128, strlen(line));
  ALLOCATE (newline, char, NLINE);  /* WARNING: this limits the length of the input line */

  V0 = thiscomm (line);
  if (V0 && !strncmp ("while", V0, strlen(V0))) {
      // special case: don't expand variables in while () statement
      strcpy (newline, line);
      free (line);
      return (newline);
  }
  free (V0);

  /* look for form $a$b..$n and expand only the last ones */

  // expand from line (L) into newline (N)
  for (L = line, N = newline; *L != 0; N++, L++) {  /* loop until end of line */

    // look for end of line or start of variable ($)
    for (done = FALSE; !done;) {
      if (*L == 0) done = TRUE;

      // look for the last $var
      if (*L == '$') {
	V1 = aftervar(L);
	if ((V1 != NULL) && (*V1 != '$')) done = TRUE;
	if (V1 == NULL) done = TRUE;
      }

      // copy into newline
      if (!done) { 
	*N = *L;
	 L++; 
	 N++;
	 if (N - newline >= NLINE - 5) {
	   Noff = N - newline;
	   NLINE += 128;
	   REALLOCATE (newline, char, NLINE);
	   N = newline + Noff;
	 }
      }
    }

    // end of the line
    if (*L == 0) break;

    V1 = aftervar (L);           /* V1 points to the first non-WHITESPACE after the variable */
    V0 = thisvar (L);            /* V0 points to the name of the var */
    /* note: V1 points to a fraction of L, it does not need to be freed */

    /* no variable name, e.g., word$ or $$.  keep the $ intact */
    if (V0 == NULL) {
      *N = *L;
      if (N - newline >= NLINE - 5) {
	Noff = N - newline;
	NLINE += 128;
	REALLOCATE (newline, char, NLINE);
	N = newline + Noff;
      }
      continue;
    }

    /* variable assignment ($ at start of the line) : skip these variables */
    if ((L == line) && (V1 != NULL)) {
      int isAssignment;

      isAssignment = FALSE;
      isAssignment |= (V1[0] == '=');
      isAssignment |= (V1[0] == '+') && (V1[1] == '=');
      isAssignment |= (V1[0] == '-') && (V1[1] == '=');
      isAssignment |= (V1[0] == '+') && (V1[1] == '+');
      isAssignment |= (V1[0] == '-') && (V1[1] == '-');

      if (isAssignment) {
	*N = *L;
	free (V0);
	if (N - newline >= NLINE - 5) {
	  Noff = N - newline;
	  NLINE += 128;
	  REALLOCATE (newline, char, NLINE);
	  N = newline + Noff;
	}
	continue;
      }
    }

    // V0 is the name of a variable.  
    // $tree_$branch_$twig should work (e.g,. $foo_bar_baz exists, $twig = baz, $foo_bar_$twig )
    // $ref = name; echo $$ref:n word  need to expand $ref to get value of $name:n

    // examples of valid nested variables, given $name = value, $ref = name, $name:n = 1, $name:0 = a, $i = 0
    // $$ref -> value
    // $$ref:n -> $name:n
    // $$ref:$i -> $$ref:0 $name:9
    
    // is there a $ in the previous character?
    // do not include ':' in the variable name
    int isRef = FALSE;
    if ((L != line) && (L[-1] == '$')) {
      free (V0);
      V0 = thisref(L);
      isRef = TRUE;
    }

    // check for macro-arguments ($1, $2)
    found = TRUE;
    for (c = V0; isdigit(*c); c++); /* test if this is a macro argument variable (ie, $1.2) */
    if (*c == 0) {  /* all digit var == macro parameter */
      if (!MacroDepth) {
	gprint (GP_ERR, "not in a macro\n");
	goto error;
      } else { /* if we are executing a macro, attach the list depth to the front, pass on down the line */
	// XXX this is limiting!!!
	ALLOCATE (c, char, 1024);
	sprintf (c, "%d.%s", MacroDepth, V0);
	free (V0);
	V0 = c;
      }
    }

    // we have something of the form $word, where V0 now is word.  expand it out:

    Val = get_variable_ptr (V0);
    if (Val == NULL) {   /* var was not found! */
      gprint (GP_ERR, "variable %s not found\n", V0);
      goto error;
    }
    for (; *Val != 0; N++, Val++)  {
      *N = *Val; /* place the value of the variable in the newline */
      if (N - newline >= NLINE - 5) {
	Noff = N - newline;
	NLINE += 128;
	REALLOCATE (newline, char, NLINE);
	N = newline + Noff;
      }
    }
    N--; /* we overshoot on last loop */

    /* skip past the variable (or reference) in the source line */
    /* L currently points at $ */
    L++;
    if (*L == '?') L++;  /* skip past ? in $?name */

    if (isRef) {
      while (ISREF(*L)) L++;
    } else {
      while (ISVAR(*L)) L++;
    }
    L--; /* we overshoot */

    if (V0 != NULL) free (V0);
  }

  *N = 0;
  free (line);
  REALLOCATE (newline, char, strlen (newline) + 1);
  if (found) { /* try again for new variables */
    newline = expand_vars (newline);
  }
  return (newline);

error:
  free (line);
  free (V0);
  free (newline);
  return (NULL);
}


  /************************
    we go through the line looking for the dollar signs ($) marking the 
    variables.  We don't expand a variable if it is the first thing on the line 
    AND it is followed by an equals sign.  
    that is, don't expand if:
    V0 = NULL ($ is last non-blank char on line -- no variable) or
    V1[0] is = (assignment of variable value), AND L == line (ie, at beginning of line)
    *************************/


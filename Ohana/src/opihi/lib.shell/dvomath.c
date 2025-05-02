# include "opihi.h"
# include "dvo.h"

/* return value on success is temp vector/buffer name or scalar value return value on error is NULL, all
   internals freed.  errors are sent to error stack.  may be printed by calling function */

/* XXX this function breaks the elements of the line into objects, numbers, and operators.  it
 * then converts the list to an RPN expresssion.  it then evaluates the expression.  if an
 * expression is not a valid math expression (string - string), it results in an error.  The
 * problem is that elements of a valid expression may be invalid as expressions on their own
 * (eg, string consisting of word-word).  this function probably should be somewhat smarter
 * about breaking down items into strings and objects.  for the time being, the user must
 * protect string items with double quotes for safety.
 */
   
char *dvomath (int argc, char **argv, int *size, int validsize) {
  
  int  i, Nstack;
  char   **cstack, *outname;
  StackVar *stack;
  Buffer *buf;
  Vector *vec;

  buf = NULL;
  vec = NULL;
  ALLOCATE (outname, char, 256);

  /* take char array with expression, convert to important elements */
  unsigned int Ncstack;
  cstack = isolate_elements (argc, argv, &Ncstack); 

  // for (i = 0; i < Ncstack; i++) {
  //   fprintf (stderr, "%d : %s\n", i, cstack[i]);
  // }

  /* generate RPN stack from cstack arguments */
  stack = convert_to_RPN (Ncstack, cstack, &Nstack);
  if (Nstack < 1) goto error;

  /* distinguish scalar, vector, matrix, check dimensions */
  *size = check_stack (stack, Nstack, validsize);
  if (*size < 0) goto error;

  switch (*size) {
    case 0:
      break;
    case 1:  /* allocate temp vector */
      vec = NULL;
      for (i = 0; (i < 1000) && (vec == NULL); i++) {
	sprintf (outname, "tmp%03d", i);
	vec = SelectVector (outname, NEWVECTOR, FALSE);
      }
      if (vec == NULL) { 
	push_error ("too many tmp vectors");
	goto error;
      }
      break;
    case 2:  /* allocate temp buffer */
      buf = NULL;
      for (i = 0; (i < 1000) && (buf == NULL); i++) {
	sprintf (outname, "tmp%03d", i);
	buf = SelectBuffer (outname, NEWBUFFER, FALSE);
      }
      if (buf == NULL) {
	push_error ("too many tmp buffers");
	goto error;
      }
      break;
    default:
      goto error;
  }

  /* evaluate operations, free stack on error */
  Ncstack = Nstack;
  if (!evaluate_stack (stack, &Nstack)) {
    if (*size == 1) DeleteVector (vec);
    if (*size == 2) DeleteBuffer (buf);
    goto error;
  }

  switch (*size) {
    case 0:
      if (Ncstack == 1) {
	/* use exact input word */
	sprintf (outname, "%s", stack[0].name);
      } else {
	if (stack[0].type == ST_SCALAR_INT) {
	  sprintf (outname, OPIHI_INT_FMT, stack[0].IntValue);
	} else {
	  sprintf (outname, "%.12g", stack[0].FltValue);
	}
      }
      break;

    case 1:
      MoveVector (vec, stack[0].vector);
      break;
  
    case 2:
      MoveBuffer (buf, stack[0].buffer);
      break;
  }

  clean_stack (stack, Nstack);
  free (stack);
  return (outname);

error:  
  clean_stack (stack, Nstack);
  free (stack);
  free (outname);
  return (NULL);
}

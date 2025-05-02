# include "opihi.h"
# define VERBOSE 0

// all three operands must have the same type
# define THREE_OP(A,FUNC) 						\
  if ((stack[i - 3].type == A) && (stack[i - 2].type == A) && (stack[i - 1].type == A)) { \
    status = FUNC (&tmp_stack, &stack[i - 3], &stack[i - 2], &stack[i - 1], stack[i].name); \
    goto got_three_op; } \
  if ((stack[i - 3].type == A+1) && (stack[i - 2].type == A) && (stack[i - 1].type == A)) { \
    status = FUNC (&tmp_stack, &stack[i - 3], &stack[i - 2], &stack[i - 1], stack[i].name); \
    goto got_three_op; } \
  if ((stack[i - 3].type == A) && (stack[i - 2].type == A+1) && (stack[i - 1].type == A)) { \
    status = FUNC (&tmp_stack, &stack[i - 3], &stack[i - 2], &stack[i - 1], stack[i].name); \
    goto got_three_op; } \
  if ((stack[i - 3].type == A+1) && (stack[i - 2].type == A+1) && (stack[i - 1].type == A)) { \
    status = FUNC (&tmp_stack, &stack[i - 3], &stack[i - 2], &stack[i - 1], stack[i].name); \
    goto got_three_op; } \
  if ((stack[i - 3].type == A) && (stack[i - 2].type == A) && (stack[i - 1].type == A+1)) { \
    status = FUNC (&tmp_stack, &stack[i - 3], &stack[i - 2], &stack[i - 1], stack[i].name); \
    goto got_three_op; } \
  if ((stack[i - 3].type == A+1) && (stack[i - 2].type == A) && (stack[i - 1].type == A+1)) { \
    status = FUNC (&tmp_stack, &stack[i - 3], &stack[i - 2], &stack[i - 1], stack[i].name); \
    goto got_three_op; } \
  if ((stack[i - 3].type == A) && (stack[i - 2].type == A+1) && (stack[i - 1].type == A+1)) { \
    status = FUNC (&tmp_stack, &stack[i - 3], &stack[i - 2], &stack[i - 1], stack[i].name); \
    goto got_three_op; } \
  if ((stack[i - 3].type == A+1) && (stack[i - 2].type == A+1) && (stack[i - 1].type == A+1)) { \
    status = FUNC (&tmp_stack, &stack[i - 3], &stack[i - 2], &stack[i - 1], stack[i].name); \
    goto got_three_op; } \

// A & B value types all have 2 possible values
// 
# define TWO_OP(A,B,FUNC) {						\
    if ((stack[i - 2].type == A) && (stack[i - 1].type == B)) {		\
      status = FUNC (&tmp_stack, &stack[i - 2], &stack[i - 1], stack[i].name); \
      goto got_two_op; }						\
    if ((stack[i - 2].type == A+1) && (stack[i - 1].type == B)) {	\
      status = FUNC (&tmp_stack, &stack[i - 2], &stack[i - 1], stack[i].name); \
      goto got_two_op; }						\
    if ((stack[i - 2].type == A) && (stack[i - 1].type == B+1)) {	\
      status = FUNC (&tmp_stack, &stack[i - 2], &stack[i - 1], stack[i].name); \
      goto got_two_op; }						\
    if ((stack[i - 2].type == A+1) && (stack[i - 1].type == B+1)) {	\
      status = FUNC (&tmp_stack, &stack[i - 2], &stack[i - 1], stack[i].name); \
      goto got_two_op; } \
  }

# define ONE_OP(A,FUNC)						\
  if (stack[i - 1].type == A) {					\
    status = FUNC (&tmp_stack, &stack[i - 1], stack[i].name);   \
    goto got_one_op; }

int evaluate_stack (StackVar *stack, int *Nstack) {
  
  int i, j, status;
  char line[512]; // this is only used to report an error
  StackVar tmp_stack;

  status = TRUE;
  init_stack (&tmp_stack);

  if (*Nstack == 1) {
    if ((stack[0].type == ST_SCALAR_INT) || (stack[0].type == ST_SCALAR_FLT)) {
      clear_stack (&tmp_stack);
      return (TRUE);
    }
    if (stack[0].type == ST_VECTOR) {
      /* need to make a copy so we set output value? */
      if (stack->vector->type == OPIHI_STR) {
	L_unary (&tmp_stack, &stack[0], "=");
      } else {
	V_unary (&tmp_stack, &stack[0], "=");
      }
      move_stack (&stack[0], &tmp_stack);
      return (TRUE);
    }
    if (stack[0].type == ST_MATRIX) {
      /* need to make a copy so we set output value? */
      M_unary (&tmp_stack, &stack[0], "=");
      move_stack (&stack[0], &tmp_stack);
      return (TRUE);
    }
    push_error ("syntax error: not a math expression");
    clear_stack (&tmp_stack);
    return (FALSE);
  }      
    
  for (i = 0; i < *Nstack; i++) {

    if (VERBOSE) {
      gprint (GP_ERR, "%d: ", i);
      for (j = 0; j < *Nstack; j++) {
	gprint (GP_ERR, "%s ", stack[j].name);
      }
      if (tmp_stack.type == ST_SCALAR_INT) {
	gprint (GP_ERR, "---> "OPIHI_INT_FMT" ", tmp_stack.IntValue);
      }
      if (tmp_stack.type == ST_SCALAR_FLT) {
	gprint (GP_ERR, "---> %f ", tmp_stack.FltValue);
      }
      if (*Nstack > 0) gprint (GP_ERR, "\n");
      gprint (GP_ERR, "%d: ", i);
      for (j = 0; j < *Nstack; j++) {
	gprint (GP_ERR, "%d ", stack[j].type);
      }
      if (tmp_stack.type != ST_NONE) {
	gprint (GP_ERR, "---> %d ", tmp_stack.type);
      }
      if (*Nstack > 0) gprint (GP_ERR, "\n");
    }

    /***** trinary operators *****/
    switch (stack[i].type) {
      case ST_TRINARY:

	if (i < 3) {  /* need two variables to operate on */
	  snprintf (line, 512, "syntax error: trinary operator without three operands: %s\n(Note that the : in a trinary operation must be protected by spaces", stack[i].name);
	  push_error (line);
	  clear_stack (&tmp_stack);
	  return (FALSE);
	}

	status = FALSE;
	THREE_OP (ST_MATRIX,MMM_trinary);
	THREE_OP (ST_VECTOR,VVV_trinary);

	THREE_OP (ST_SCALAR_FLT,SSS_trinary);
	THREE_OP (ST_SCALAR_INT,SSS_trinary);

	/* there are no valid trinary string operators */
	snprintf (line, 512, "invalid operands for trinary operator %s (mismatch types?)\n(Note that the : in a trinary operation must be protected by spaces)", stack[i].name);
	push_error (line);
	clear_stack (&tmp_stack);
	return (FALSE);

      got_three_op:
	if (!status) {
	  snprintf (line, 512, "syntax error: invalid operand for trinary operation: %s or %s or %s\n(Note that the : in a trinary operation must be protected by spaces)", stack[i-1].name, stack[i-2].name, stack[i-3].name);
	  push_error (line);
	  clear_stack (&tmp_stack);
	  return (FALSE);
	}
	move_stack (&stack[i-3], &tmp_stack);
	delete_stack (&stack[i-2], 3);
	for (j = i + 1; j < *Nstack; j++) {
	  move_stack (&stack[j-3], &stack[j]);
	}
	*Nstack -= 3;
	i -= 3;
	init_stack (&tmp_stack);
	continue;

	/***** binary operators *****/
      case ST_OR:
      case ST_AND:
      case ST_LOGIC:
      case ST_BITWISE:
      case ST_ADD:
      case ST_TIMES:
      case ST_POWER:
      case ST_BINARY:

	if (i < 2) {  /* need two variables to operate on */
	  snprintf (line, 512, "syntax error: binary operator with one operand: %s\n", stack[i].name);
	  push_error (line);
	  clear_stack (&tmp_stack);
	  return (FALSE);
	}

	// TWO_OP(A,B,OP) : A,B types all have a main value and a secondary value at A+1 or B+1:
	// ST_MATRIX -> ST_MATRIX_TMP
	// ST_VECTOR -> ST_VECTOR_TMP
	// ST_SCALAR_INT -> ST_SCALAR_FLT 
	status = FALSE;
	TWO_OP (ST_MATRIX,ST_MATRIX,MM_binary);
	TWO_OP (ST_MATRIX,ST_VECTOR,MV_binary);
	TWO_OP (ST_MATRIX,ST_SCALAR_INT,MS_binary); // handles ST_SCALAR_INT and ST_SCALAR_FLT 

	TWO_OP (ST_VECTOR,ST_MATRIX,VM_binary);
	TWO_OP (ST_VECTOR,ST_VECTOR,VV_binary);
	TWO_OP (ST_VECTOR,ST_SCALAR_INT,VS_binary); // handles ST_SCALAR_INT and ST_SCALAR_FLT 

	TWO_OP (ST_SCALAR_INT,ST_MATRIX,SM_binary); // handles ST_SCALAR_INT and ST_SCALAR_FLT 
	TWO_OP (ST_SCALAR_INT,ST_VECTOR,SV_binary); // handles ST_SCALAR_INT and ST_SCALAR_FLT 
	TWO_OP (ST_SCALAR_INT,ST_SCALAR_INT,SS_binary);       // handles ST_SCALAR_INT and ST_SCALAR_FLT 

//      XXX: this block is not needed
//	TWO_OP (ST_SCALAR_FLT,ST_MATRIX,SM_binary);
//	TWO_OP (ST_SCALAR_FLT,ST_VECTOR,SV_binary);
//	TWO_OP (ST_SCALAR_FLT,ST_SCALAR_INT,SS_binary);      

	TWO_OP (ST_VECTOR,ST_STRING,LW_binary);      
	TWO_OP (ST_STRING,ST_VECTOR,WL_binary);      

	TWO_OP (ST_STRING,ST_STRING,WW_binary);      
	TWO_OP (ST_STRING,ST_SCALAR_INT,WW_binary);      
	TWO_OP (ST_SCALAR_INT,ST_STRING,WW_binary);      
      
      got_two_op:
	if (!status) {
	  // we are guaranteed to have stack[i-1] and stack[i-2] since i >= 2 (above)
	  char tmpvector[] = "Temporary Vector";
	  char tmpmatrix[] = "Temporary Matrix";
	  
	  char *name1 = NULL;
	  if (stack[i-1].name) {
	    name1 = stack[i-1].name;
	  } else {
	    if (stack[i-1].type == ST_VECTOR_TMP) name1 = tmpvector;
	    if (stack[i-1].type == ST_MATRIX_TMP) name1 = tmpmatrix;
	  }
	  char *name2 = NULL;
	  if (stack[i-2].name) {
	    name2 = stack[i-2].name;
	  } else {
	    if (stack[i-2].type == ST_VECTOR_TMP) name2 = tmpvector;
	    if (stack[i-2].type == ST_MATRIX_TMP) name2 = tmpmatrix;
	  }

	  snprintf (line, 512, "syntax error: invalid operand for binary operation: %s or %s\n", name1, name2);
	  push_error (line);

	  int isTrinary = TRUE;
	  isTrinary = isTrinary && (i >= 2);
	  // isTrinary = isTrinary && (stack[i - 1] != NULL);
	  // isTrinary = isTrinary && (stack[i - 2] != NULL);
	  isTrinary = isTrinary && (stack[i - 1].name != NULL);
	  isTrinary = isTrinary && (stack[i - 2].name != NULL);
	  isTrinary = isTrinary && (strchr(stack[i-1].name, ':') || strchr(stack[i-2].name, ':'));
	  if (isTrinary) {
	    snprintf (line, 512, "syntax error: invalid operand for binary operation: %s or %s\n(Note that the : in a trinary operation must be protected by spaces)\n", stack[i-1].name, stack[i-2].name);
	    push_error (line);
	  }

	  clear_stack (&tmp_stack);
	  return (FALSE);
	}
	move_stack (&stack[i-2], &tmp_stack);
	delete_stack (&stack[i-1], 2);
	for (j = i + 1; j < *Nstack; j++) {
	  move_stack (&stack[j - 2], &stack[j]);
	}
	*Nstack -= 2;
	i -= 2;
	init_stack (&tmp_stack);
	continue;

	/***** unary operators **/
      case ST_UNARY:

	if (i < 1) {  /* need one variable to operate on */
	  push_error ("syntax error: unary operator with no operand");
	  clear_stack (&tmp_stack);
	  return (FALSE);
	}

	ONE_OP (ST_MATRIX, M_unary);
	ONE_OP (ST_MATRIX_TMP, M_unary);

	ONE_OP (ST_VECTOR, V_unary);
	ONE_OP (ST_VECTOR_TMP, V_unary);

	ONE_OP (ST_SCALAR_INT, S_unary);
	ONE_OP (ST_SCALAR_FLT, S_unary);

	ONE_OP (ST_STRING, W_unary);

	snprintf (line, 512, "invalid operand for unary operator %s (undefined value?)", stack[i].name);
	push_error (line);
	clear_stack (&tmp_stack);
	return (FALSE);

      got_one_op:
	move_stack (&stack[i-1], &tmp_stack);
	delete_stack (&stack[i], 1);
	for (j = i + 1; j < *Nstack; j++) {
	  move_stack (&stack[j - 1], &stack[j]);
	}
	init_stack (&tmp_stack);
	*Nstack -= 1;
	i -= 1;
	continue;

      case ST_SCALAR_INT:
      case ST_SCALAR_FLT:
      case ST_VECTOR:
      case ST_VECTOR_TMP:
      case ST_MATRIX:
      case ST_MATRIX_TMP:
      case ST_STRING:
	continue;

      default:
	snprintf (line, 512, "syntax error: unexpected operator type %s", stack[i].name);
	push_error (line);
	clear_stack (&tmp_stack);
	return (FALSE);
    }
  }
  clear_stack (&tmp_stack);

  if (*Nstack > 1) {
    push_error ("syntax error in evaluation");
    return (FALSE);
  }
  return (TRUE);
}

/* copy data to new stack variable */
void copy_stack (StackVar *stack1, StackVar *stack2) {
  stack1[0].name     = stack2[0].name  ;
  stack1[0].type     = stack2[0].type  ;
  stack1[0].buffer   = stack2[0].buffer;
  stack1[0].vector   = stack2[0].vector;
  stack1[0].FltValue = stack2[0].FltValue;
  stack1[0].IntValue = stack2[0].IntValue;
}

/* replace data with new stack variable */
void move_stack (StackVar *stack1, StackVar *stack2) {
  clear_stack (stack1);
  copy_stack (stack1, stack2);
  stack2[0].name = NULL;
}

/* delete name and data */
void clean_stack (StackVar *stack, int Nstack) {

  int i;

  for (i = 0; i < Nstack; i++) {
    if (IsBufferPtr (stack[i].buffer) && (stack[i].type == ST_MATRIX_TMP)) {
      if (VERBOSE) gprint (GP_ERR, "free %s (buff) (%lx)\n", stack[i].name, (long) stack[i].buffer);
      free (stack[i].buffer[0].header.buffer);
      free (stack[i].buffer[0].matrix.buffer);
      free (stack[i].buffer);
      stack[i].buffer = NULL;
    }	
    if (IsVectorPtr (stack[i].vector) && (stack[i].type == ST_VECTOR_TMP)) {
      if (VERBOSE) gprint (GP_ERR, "free %s (vect) (%lx)\n", stack[i].name, (long) stack[i].vector);
      free (stack[i].vector[0].elements.Ptr);
      free (stack[i].vector);
      stack[i].vector = NULL;
    }	
    if (VERBOSE) gprint (GP_ERR, "free %s (name) (%d) (%lx)\n", stack[i].name, i, (long) stack[i].name);
    clear_stack (&stack[i]);
  }

}

/* delete name only, not data */
void delete_stack (StackVar *stack, int Nstack) {
  int i;
  for (i = 0; i < Nstack; i++) {
    clear_stack (&stack[i]);
  }
}

void init_stack (StackVar *stack) {
  stack[0].buffer = NULL;
  stack[0].vector = NULL;
  stack[0].name = NULL;
  stack[0].type = ST_NONE;
}

// strip off surrounding " and return newly allocated string
char *clean_stack_name (char *name) {

  if (!name) return name;

  char *tmpname = name;
  if (name[0] == '"') tmpname ++;

  char *output = strcreate (tmpname);

  int Nchar = strlen(output);
  if (Nchar == 0) return output; // empty string

  // squash ending quotes:
  if (output[Nchar - 1] == '"') output[Nchar - 1] = 0;

  return output;
}

void assign_stack (StackVar *stack, char *name, StackVarType type) {
  stack->name = strcreate(name);
  stack->type = type;
}

void clear_stack (StackVar *stack) {
  if (stack->name == NULL) return;
  // fprintf (stderr, "free %s\n", stack->name);
  free (stack->name);
  stack->name = NULL;
  return;
}

# include "opihi.h"
# define DUMPSTACK 0

StackVar *convert_to_RPN (int argc, char **argv, int *nstack) {
  
 StackVarType type;
  int i, j, Nstack, Nop_stack, NSTACK;
  StackVar *stack, *op_stack;

  /* max total stack size is argc, though should be less, this is safe */
  NSTACK = argc + 5;
  ALLOCATE (stack, StackVar, NSTACK);
  ALLOCATE (op_stack, StackVar, NSTACK);
  for (i = 0; i < NSTACK; i++) {
    init_stack (&stack[i]);
    init_stack (&op_stack[i]);
  }
  
  Nstack = Nop_stack = 0;
  for (i = 0; i < argc; i++) {
    
    /* decide on priority of object */
    type = ST_NONE;

    /* trinary operations */
    if (!strcmp (argv[i], "?"))      { type = ST_TRINARY; goto gotit; }
    if (!strcmp (argv[i], ":"))      { type = ST_COMMA; goto gotit; }

    /* unary operations */
    if (!strcmp (argv[i], "abs"))    { type = ST_UNARY; goto gotit; }
    if (!strcmp (argv[i], "int"))    { type = ST_UNARY; goto gotit; }
    if (!strcmp (argv[i], "floor"))  { type = ST_UNARY; goto gotit; }
    if (!strcmp (argv[i], "round"))  { type = ST_UNARY; goto gotit; }
    if (!strcmp (argv[i], "ceil"))   { type = ST_UNARY; goto gotit; }
    if (!strcmp (argv[i], "rint"))   { type = ST_UNARY; goto gotit; }
    if (!strcmp (argv[i], "exp"))    { type = ST_UNARY; goto gotit; }
    if (!strcmp (argv[i], "ten"))    { type = ST_UNARY; goto gotit; }
    if (!strcmp (argv[i], "log"))    { type = ST_UNARY; goto gotit; }
    if (!strcmp (argv[i], "ln"))     { type = ST_UNARY; goto gotit; }
    if (!strcmp (argv[i], "sqrt"))   { type = ST_UNARY; goto gotit; }
    if (!strcmp (argv[i], "erf"))    { type = ST_UNARY; goto gotit; }
    if (!strcmp (argv[i], "sinh"))   { type = ST_UNARY; goto gotit; }
    if (!strcmp (argv[i], "cosh"))   { type = ST_UNARY; goto gotit; }
    if (!strcmp (argv[i], "asinh"))  { type = ST_UNARY; goto gotit; }
    if (!strcmp (argv[i], "acosh"))  { type = ST_UNARY; goto gotit; }
    if (!strcmp (argv[i], "sin"))    { type = ST_UNARY; goto gotit; }
    if (!strcmp (argv[i], "cos"))    { type = ST_UNARY; goto gotit; }
    if (!strcmp (argv[i], "tan"))    { type = ST_UNARY; goto gotit; }
    if (!strcmp (argv[i], "dsin"))   { type = ST_UNARY; goto gotit; }
    if (!strcmp (argv[i], "dcos"))   { type = ST_UNARY; goto gotit; }
    if (!strcmp (argv[i], "dtan"))   { type = ST_UNARY; goto gotit; }
    if (!strcmp (argv[i], "asin"))   { type = ST_UNARY; goto gotit; }
    if (!strcmp (argv[i], "acos"))   { type = ST_UNARY; goto gotit; }
    if (!strcmp (argv[i], "atan"))   { type = ST_UNARY; goto gotit; }
    if (!strcmp (argv[i], "dasin"))  { type = ST_UNARY; goto gotit; }
    if (!strcmp (argv[i], "dacos"))  { type = ST_UNARY; goto gotit; }
    if (!strcmp (argv[i], "datan"))  { type = ST_UNARY; goto gotit; }
    if (!strcmp (argv[i], "lgamma")) { type = ST_UNARY; goto gotit; }
    if (!strcmp (argv[i], "rnd"))    { type = ST_UNARY; goto gotit; }
    if (!strcmp (argv[i], "drnd"))   { type = ST_UNARY; goto gotit; }
    if (!strcmp (argv[i], "lrnd"))   { type = ST_UNARY; goto gotit; }
    if (!strcmp (argv[i], "mrnd"))   { type = ST_UNARY; goto gotit; }
    if (!strcmp (argv[i], "xramp"))  { type = ST_UNARY; goto gotit; }
    if (!strcmp (argv[i], "yramp"))  { type = ST_UNARY; goto gotit; }
    if (!strcmp (argv[i], "zramp"))  { type = ST_UNARY; goto gotit; }
    if (!strcmp (argv[i], "ramp"))   { type = ST_UNARY; goto gotit; }
    if (!strcmp (argv[i], "zero"))   { type = ST_UNARY; goto gotit; }
    if (!strcmp (argv[i], "--"))     { type = ST_UNARY; goto gotit; }
    if (!strcmp (argv[i], "not"))    { type = ST_UNARY; goto gotit; }
    if (!strcmp (argv[i], "isinf"))  { type = ST_UNARY; goto gotit; }
    if (!strcmp (argv[i], "isnan"))  { type = ST_UNARY; goto gotit; }

    /* string-valid unary operations */
    if (!strcmp (argv[i], "isword")) { type = ST_UNARY; goto gotit; }
    if (!strcmp (argv[i], "isnum"))  { type = ST_UNARY; goto gotit; }
    if (!strcmp (argv[i], "isint"))  { type = ST_UNARY; goto gotit; }
    if (!strcmp (argv[i], "isflt"))  { type = ST_UNARY; goto gotit; }
    if (!strcmp (argv[i], "length")) { type = ST_UNARY; goto gotit; }

    /* binary operations */
    // NOTE: I archically use the first character of the function name in a switch to identify the operation
    // I should re-work this with an enum which defines the operatino
    if (!strcmp (argv[i], "^"))      { type = ST_POWER; goto gotit; }

    if (!strcmp (argv[i], "max"))    { type = ST_BINARY; strcpy (argv[i], "U"); goto gotit; }
    if (!strcmp (argv[i], "min"))    { type = ST_BINARY; strcpy (argv[i], "D"); goto gotit; }
    if (!strcmp (argv[i], "atan2"))  { type = ST_BINARY; strcpy (argv[i], "a"); goto gotit; }
    if (!strcmp (argv[i], "datan2")) { type = ST_BINARY; strcpy (argv[i], "d"); goto gotit; }

    if (!strcmp (argv[i], "<~"))     { type = ST_BINARY; strcpy (argv[i], "l"); goto gotit; }
    if (!strcmp (argv[i], "~>"))     { type = ST_BINARY; strcpy (argv[i], "r"); goto gotit; }

    if (!strcmp (argv[i], ","))      { type = ST_COMMA; goto gotit; }

    if (!strcmp (argv[i], "@"))      { type = ST_TIMES; goto gotit; }
    if (!strcmp (argv[i], "/"))      { type = ST_TIMES; goto gotit; }
    if (!strcmp (argv[i], "*"))      { type = ST_TIMES; goto gotit; }
    if (!strcmp (argv[i], "%"))      { type = ST_TIMES; goto gotit; }

    if (!strcmp (argv[i], "+"))      { type = ST_ADD; goto gotit; }
    if (!strcmp (argv[i], "-"))      { type = ST_ADD; goto gotit; }
	
    if (!strcmp (argv[i], "&"))      { type = ST_BITWISE; goto gotit; }
    if (!strcmp (argv[i], "|"))      { type = ST_BITWISE; goto gotit; }

    if (!strcmp (argv[i], "<"))      { type = ST_LOGIC; goto gotit; }
    if (!strcmp (argv[i], ">"))      { type = ST_LOGIC; goto gotit; }
    if (!strcmp (argv[i], "=="))     { type = ST_LOGIC; strcpy (argv[i], "E"); goto gotit; }
    if (!strcmp (argv[i], "!="))     { type = ST_LOGIC; strcpy (argv[i], "N"); goto gotit; }
    if (!strcmp (argv[i], "<="))     { type = ST_LOGIC; strcpy (argv[i], "L"); goto gotit; }
    if (!strcmp (argv[i], ">="))     { type = ST_LOGIC; strcpy (argv[i], "G"); goto gotit; }
    if (!strcmp (argv[i], ">>"))     { type = ST_LOGIC; strcpy (argv[i], "U"); goto gotit; }
    if (!strcmp (argv[i], "<<"))     { type = ST_LOGIC; strcpy (argv[i], "D"); goto gotit; }

    /* XXX I would like to change the syntax to allow << and >> to mean bitshifts
       but that means breaking these older values which means MIN and MAX */
    // for now, use <- and -> to mean bitshift 

    if (!strcmp (argv[i], "&&"))     { type = ST_AND; strcpy (argv[i], "A"); goto gotit; }
    if (!strcmp (argv[i], "||"))     { type = ST_OR ; strcpy (argv[i], "O"); goto gotit; }

    if (!strcmp (argv[i], "("))      { type = ST_LEFT;  goto gotit; }
    if (!strcmp (argv[i], ")"))      { type = ST_RIGHT; goto gotit; }

  gotit:
    /* choose how to deal with object */
    switch (type) {
      case ST_POWER:  /* exponentiation: 2^2^3 = 64 != 256 (precedence is right-to-left, not left-to-right!) */
	/* pop previous, higher operators from OP stack to stack */
	for (j = Nop_stack - 1; (j >= 0) && (op_stack[j].type > type); j--) {
	  move_stack (&stack[Nstack], &op_stack[j]);
	  Nstack ++;
	  Nop_stack --;
	}
	/* push operator on OP stack */
	assign_stack (&op_stack[Nop_stack], argv[i], type);
	Nop_stack ++;
	break;
      case ST_BINARY: 
      case ST_TRINARY:
      case ST_TIMES:
      case ST_ADD:
      case ST_BITWISE: 
      case ST_LOGIC: 
      case ST_AND: 
      case ST_OR: 
	/* pop previous, higher or equal operators from OP stack to stack */
	for (j = Nop_stack - 1; (j >= 0) && (op_stack[j].type >= type); j--) {
	  move_stack (&stack[Nstack], &op_stack[j]);
	  Nstack ++;
	  Nop_stack --;
	}
	/* push operator on OP stack */
	assign_stack (&op_stack[Nop_stack], argv[i], type);
	Nop_stack ++;
	break;
      case ST_UNARY: 
      case ST_LEFT:  
	/* push operator on OP stack */
	assign_stack (&op_stack[Nop_stack], argv[i], type);
	// fprintf (stderr, "push %s\n", op_stack[Nop_stack].name);
	Nop_stack ++;
	break;
      case ST_RIGHT: 
	/* pop rest of operators from OP stack to stack, looking for '(' */
	for (j = Nop_stack - 1; (j >= 0) && (op_stack[j].type != ST_LEFT); j--) {
	  move_stack (&stack[Nstack], &op_stack[j]);
	  Nstack ++;
	  Nop_stack --;
	}
	if ((j == -1) || (op_stack[j].type != ST_LEFT)) {
	  push_error ("syntax error: mismatched parenthesis");
	  Nstack = 0;
	  goto cleanup;
	}
	// delete the '(' from op_stack:
	// fprintf (stderr, "pop %s\n", op_stack[j].name);
	clear_stack (&op_stack[j]);
	Nop_stack --;
	break;
      case ST_COMMA: 
	/* pop rest of operators from OP stack to stack, looking for '(' (but do not pop the '(')*/
	for (j = Nop_stack - 1; (j >= 0) && (op_stack[j].type != ST_LEFT) && (op_stack[j].type != ST_TRINARY); j--) {
	  move_stack (&stack[Nstack], &op_stack[j]);
	  Nstack ++;
	  Nop_stack --;
	}
	break;
      case ST_NONE:
	/* place the value (number or vector/matrix name) on stack */
	/* value of 'X' is used as sentinel until we sort out values */
	assign_stack (&stack[Nstack], argv[i], ST_VALUE);
	Nstack ++;
	break;

      default:
	push_error ("invalid stack typ");
	Nstack = 0;
	goto cleanup;
    }
  }

  /* dump remaining operators on stack, checking for '(' */
  for (j = Nop_stack - 1; j >= 0; j--) {
    if (op_stack[j].type == ST_LEFT) {
      push_error ("syntax error: mismatched parenthesis");
      Nstack = 0;
      goto cleanup;
    }
    move_stack (&stack[Nstack],  &op_stack[j]);
    Nstack ++;
  }

cleanup: 
  /*** free up unused stack space ***/

  // If we parsed everything above, there should not be any un-freed op_stacks at this
  // point.  However, if we had a syntax error, we will have op_stack entries left behind.
  delete_stack (op_stack, NSTACK);
  free (op_stack);

  // XXX there should not be any data on higher stack entries
  // clean_stack (&stack[Nstack], NSTACK - Nstack);
  REALLOCATE (stack, StackVar, MAX (Nstack, 1));
  *nstack = Nstack;

  for (i = 0; i < argc; i++) {
    free (argv[i]);
  }
  free (argv);

# if (DUMPSTACK)
  for (i = 0; i < Nstack; i++) {
    gprint (GP_ERR, "%s ", stack[i].name);
  }
  if (Nstack > 0) gprint (GP_ERR, "\n");
  for (i = 0; i < Nstack; i++) {
    gprint (GP_ERR, "%d ", stack[i].type);
  }
  if (Nstack > 0) gprint (GP_ERR, "\n");
# endif

  return (stack);

}

/* here are the rules for parsing a math AOL line to RPN:

1) if object is a number, push on stack
2) if object is a third order operand (exp, sin, cos), push on op stack
3) if object is an open paren, push on op stack,
4) if object is a second order operand, push on stack
5) if object is a first order operand, pop all second order operands from stack 
until paren, push on stack
6) if object is an end paren, pop all objects from stack until paren, 
pop next stack, if third order op
7) if end of line, pop all remaining objects, second order first, etc.
   
*/


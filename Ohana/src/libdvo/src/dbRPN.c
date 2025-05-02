# include "dvo.h"
# define DUMPSTACK 0

dbStack *dbRPN (int argc, char **argv, int *nstack) {
  
  int type;
  int i, j, Nstack, Nop_stack, NSTACK;
  dbStack *stack, *op_stack;

  /* max total stack size is argc, though should be less, this is safe */
  NSTACK = argc + 5;
  ALLOCATE (stack, dbStack, NSTACK);
  ALLOCATE (op_stack, dbStack, NSTACK);
  for (i = 0; i < NSTACK; i++) {
    dbInitStack (&stack[i]);
    dbInitStack (&op_stack[i]);
  }
  
  Nstack = Nop_stack = 0;
  for (i = 0; i < argc; i++) {
    
    /* decide on priority of object */
    type = DB_STACK_NONE;
    /* unary operations */
    if (!strcmp (argv[i], "abs"))    { type = DB_STACK_UNARY; goto gotit; }
    if (!strcmp (argv[i], "int"))    { type = DB_STACK_UNARY; goto gotit; }
    if (!strcmp (argv[i], "exp"))    { type = DB_STACK_UNARY; goto gotit; }
    if (!strcmp (argv[i], "ten"))    { type = DB_STACK_UNARY; goto gotit; }
    if (!strcmp (argv[i], "log"))    { type = DB_STACK_UNARY; goto gotit; }
    if (!strcmp (argv[i], "ln"))     { type = DB_STACK_UNARY; goto gotit; }
    if (!strcmp (argv[i], "sqrt"))   { type = DB_STACK_UNARY; goto gotit; }
    if (!strcmp (argv[i], "erf"))    { type = DB_STACK_UNARY; goto gotit; }
    if (!strcmp (argv[i], "sinh"))   { type = DB_STACK_UNARY; goto gotit; }
    if (!strcmp (argv[i], "cosh"))   { type = DB_STACK_UNARY; goto gotit; }
    if (!strcmp (argv[i], "asinh"))  { type = DB_STACK_UNARY; goto gotit; }
    if (!strcmp (argv[i], "acosh"))  { type = DB_STACK_UNARY; goto gotit; }
    if (!strcmp (argv[i], "sin"))    { type = DB_STACK_UNARY; goto gotit; }
    if (!strcmp (argv[i], "cos"))    { type = DB_STACK_UNARY; goto gotit; }
    if (!strcmp (argv[i], "tan"))    { type = DB_STACK_UNARY; goto gotit; }
    if (!strcmp (argv[i], "dsin"))   { type = DB_STACK_UNARY; goto gotit; }
    if (!strcmp (argv[i], "dcos"))   { type = DB_STACK_UNARY; goto gotit; }
    if (!strcmp (argv[i], "dtan"))   { type = DB_STACK_UNARY; goto gotit; }
    if (!strcmp (argv[i], "asin"))   { type = DB_STACK_UNARY; goto gotit; }
    if (!strcmp (argv[i], "acos"))   { type = DB_STACK_UNARY; goto gotit; }
    if (!strcmp (argv[i], "atan"))   { type = DB_STACK_UNARY; goto gotit; }
    if (!strcmp (argv[i], "dasin"))  { type = DB_STACK_UNARY; goto gotit; }
    if (!strcmp (argv[i], "dacos"))  { type = DB_STACK_UNARY; goto gotit; }
    if (!strcmp (argv[i], "datan"))  { type = DB_STACK_UNARY; goto gotit; }
    if (!strcmp (argv[i], "lgamma")) { type = DB_STACK_UNARY; goto gotit; }
    if (!strcmp (argv[i], "rnd"))    { type = DB_STACK_UNARY; goto gotit; }
    if (!strcmp (argv[i], "xramp"))  { type = DB_STACK_UNARY; goto gotit; }
    if (!strcmp (argv[i], "yramp"))  { type = DB_STACK_UNARY; goto gotit; }
    if (!strcmp (argv[i], "ramp"))   { type = DB_STACK_UNARY; goto gotit; }
    if (!strcmp (argv[i], "zero"))   { type = DB_STACK_UNARY; goto gotit; }
    if (!strcmp (argv[i], "--"))     { type = DB_STACK_UNARY; goto gotit; }
    if (!strcmp (argv[i], "not"))    { type = DB_STACK_UNARY; goto gotit; }
    if (!strcmp (argv[i], "isinf"))  { type = DB_STACK_UNARY; goto gotit; }
    if (!strcmp (argv[i], "isnan"))  { type = DB_STACK_UNARY; goto gotit; }

    /* binary operations */
    if (!strcmp (argv[i], "^"))      { type = DB_STACK_POWER; goto gotit; }

    if (!strcmp (argv[i], "@"))      { type = DB_STACK_MULTIPLY; goto gotit; }
    if (!strcmp (argv[i], "/"))      { type = DB_STACK_MULTIPLY; goto gotit; }
    if (!strcmp (argv[i], "*"))      { type = DB_STACK_MULTIPLY; goto gotit; }
    if (!strcmp (argv[i], "%"))      { type = DB_STACK_MULTIPLY; goto gotit; }

    if (!strcmp (argv[i], "+"))      { type = DB_STACK_SUM; goto gotit; }
    if (!strcmp (argv[i], "-"))      { type = DB_STACK_SUM; goto gotit; }
	
    if (!strcmp (argv[i], "&"))      { type = DB_STACK_BITWISE; goto gotit; }
    if (!strcmp (argv[i], "|"))      { type = DB_STACK_BITWISE; goto gotit; }

    if (!strcmp (argv[i], "<"))      { type = DB_STACK_COMPARE; goto gotit; }
    if (!strcmp (argv[i], ">"))      { type = DB_STACK_COMPARE; goto gotit; }
    if (!strcmp (argv[i], "=="))     { type = DB_STACK_COMPARE; strcpy (argv[i], "E"); goto gotit; }
    if (!strcmp (argv[i], "!="))     { type = DB_STACK_COMPARE; strcpy (argv[i], "N"); goto gotit; }
    if (!strcmp (argv[i], "<="))     { type = DB_STACK_COMPARE; strcpy (argv[i], "L"); goto gotit; }
    if (!strcmp (argv[i], ">="))     { type = DB_STACK_COMPARE; strcpy (argv[i], "G"); goto gotit; }
    if (!strcmp (argv[i], ">>"))     { type = DB_STACK_COMPARE; strcpy (argv[i], "U"); goto gotit; }
    if (!strcmp (argv[i], "<<"))     { type = DB_STACK_COMPARE; strcpy (argv[i], "D"); goto gotit; }

    if (!strcmp (argv[i], "&&"))     { type = DB_STACK_LOGIC; strcpy (argv[i], "A"); goto gotit; }
    if (!strcmp (argv[i], "||"))     { type = DB_STACK_LOGIC; strcpy (argv[i], "O"); goto gotit; }

    if (!strcmp (argv[i], "("))      { type = DB_STACK_OPEN_PAR; goto gotit; }
    if (!strcmp (argv[i], ")"))      { type = DB_STACK_CLOSE_PAR; goto gotit; }

  gotit:
    /* choose how to deal with object */
    switch (type) {
      case DB_STACK_POWER:  /* exponentiation: 2^2^3 = 64 != 256 (precedence is right-to-left, not left-to-right!) */
	/* pop previous, higher operators from OP stack to stack */
	for (j = Nop_stack - 1; (j >= 0) && (op_stack[j].type > type); j--) {
	  stack[Nstack] = op_stack[j];
	  op_stack[j].name = NULL;
	  Nstack ++;
	  Nop_stack --;
	}
	/* push operator on OP stack */
	op_stack[Nop_stack].name = strcreate (argv[i]);
	op_stack[Nop_stack].type = type;
	Nop_stack ++;
	break;
      case DB_STACK_UNARY: /* unary OPs */
      case DB_STACK_MULTIPLY: /* binary OPs */
      case DB_STACK_SUM:
      case DB_STACK_BITWISE: 
      case DB_STACK_COMPARE: 
      case DB_STACK_LOGIC: 
	/* pop previous, higher or equal operators from OP stack to stack */
	for (j = Nop_stack - 1; (j >= 0) && (op_stack[j].type >= type); j--) {
	  stack[Nstack] = op_stack[j];
	  op_stack[j].name = NULL;
	  Nstack ++;
	  Nop_stack --;
	}
	/* push operator on OP stack */
	op_stack[Nop_stack].name = strcreate (argv[i]);
	op_stack[Nop_stack].type = type;
	Nop_stack ++;
	break;
      case DB_STACK_OPEN_PAR:  
	/* push operator on OP stack */
	op_stack[Nop_stack].name = strcreate (argv[i]);
	op_stack[Nop_stack].type = type;
	Nop_stack ++;
	break;
      case DB_STACK_CLOSE_PAR: 
	/* pop rest of operators from OP stack to stack, looking for '(' */
	for (j = Nop_stack - 1; (j >= 0) && (op_stack[j].type != DB_STACK_OPEN_PAR); j--) {
	  stack[Nstack] = op_stack[j];
	  op_stack[j].name = NULL;
	  Nstack ++;
	  Nop_stack --;
	}
	if ((j == -1) || (op_stack[j].type != DB_STACK_OPEN_PAR)) {
	  push_error ("syntax error: mismatched parenthesis");
	  Nstack = 0;
	  goto cleanup;
	}
	Nop_stack --;
	break;
      case 0:
	/* place the value (number or vector/matrix name) on stack */
	/* value of 'X' is used as sentinel until we sort out values */
	stack[Nstack].name = strcreate (argv[i]);
	stack[Nstack].type = DB_STACK_VALUE;
	Nstack ++;
	break;
    }
  }

  /* dump remaining operators on stack, checking for '(' */
  for (j = Nop_stack - 1; j >= 0; j--) {
    if (op_stack[j].type == DB_STACK_OPEN_PAR) {
      push_error ("syntax error: mismatched parenthesis");
      Nstack = 0;
      goto cleanup;
    }
    stack[Nstack] = op_stack[j];
    op_stack[j].name = NULL;
    Nstack ++;
  }

cleanup: 
  /*** free up unused stack space ***/
  dbFreeStack (op_stack, NSTACK);
  free (op_stack);
  dbFreeStack (&stack[Nstack], NSTACK - Nstack);
  REALLOCATE (stack, dbStack, MAX (Nstack, 1));
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

/* here are the rules for parsing a math AOL expression to RPN:

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

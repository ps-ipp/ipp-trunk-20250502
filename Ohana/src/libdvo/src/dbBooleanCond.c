# include "dvo.h"

// evaluate the expression in inStack as a boolean; necessary db field values are
// supplied by fields, in order 0 - Nfields (validate before calling)
// XXX fields needs to be typed (dbValue), stack math needs to deal with the type cases
int dbBooleanCond (dbStack *inStack, int NinStack, dbValue *fields) {
  
  float value;
  int i, j, N, Nstack;
  dbStack **stack, *output;

  // 'no stack' means 'no where statement'
  if (NinStack == 0) return (TRUE);

  Nstack = NinStack;
  ALLOCATE (stack, dbStack *, NinStack);
  for (i = 0; i < NinStack; i++) {
    stack[i] = &inStack[i];
  }

  for (i = 0; i < Nstack; i++) {

    /***** binary operators *****/
    if ((stack[i][0].type >= DB_STACK_LOGIC) && (stack[i][0].type <= DB_STACK_POWER)) {

      // pre-test that op and entries match
      output = dbBinary (stack[i-2], stack[i-1], stack[i][0].name, fields); 

      // free temporary stack items, drop external items
      dbFreeTempEntry (stack[i-2]);
      dbFreeTempEntry (stack[i-1]);

      stack[i-2] = output;
      for (j = i + 1; j < Nstack; j++) {
	stack[j-2] = stack[j];
      }

      Nstack -= 2;
      i -= 2;
      continue;
    }

    /***** unary operators **/
    if (stack[i][0].type == DB_STACK_UNARY) {

      // pre-test that op and entries match
      output = dbUnary (stack[i-1], stack[i][0].name, fields); 

      // free temporary stack items, drop external items
      dbFreeTempEntry (stack[i-1]);

      stack[i-1] = output;
      for (j = i + 1; j < Nstack; j++) {
	stack[j-1] = stack[j];
      }

      Nstack -= 1;
      i -= 1;
      continue;
    } 
  }

  // the result here is a single stack entry with a value:
  if (stack[0][0].type & DB_STACK_FIELD) {
    N = stack[0][0].field;
    value = (stack[0][0].type & DB_STACK_INT) ? fields[N].Int : fields[N].Flt;
  } else {
    value = (stack[0][0].type & DB_STACK_INT) ? stack[0][0].IntValue : stack[0][0].FltValue;
  }
    
  for (i = 0; i < Nstack; i++) {
    dbFreeEntry (stack[i]);
  }
  free (stack);

  // XXX fix this limit
  if (fabs(value) > 1e-7) {
    return (TRUE);
  } else {
    return (FALSE);
  }

  // pre-test that op and entries match
  return (TRUE);
}

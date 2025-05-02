# include "opihi.h"

int CheckBooleanCondition (dvoStack dbStack, int NdbStack, float *values, dvoFields *fields, int Nfields) {
  
  int i, j, Nstack;
  dvoStack **stack, *output;

  Nstack = NdbStack;
  ALLOCATE (stack, dvoStack *, NdbStack);
  for (i = 0; i < NdbStack; i++) {
    stack[i] = &dbStack[i];
  }

  for (i = 0; i < Nstack; i++) {

    /***** binary operators *****/
    if ((stack[i].type >= 3) && (stack[i].type <= 8)) {

      // pre-test that op and entries match
      output = db_binary (stack[i-2], stack[i-1], stack[i].name, fields, Nfields); 

      // free temporary stack items, drop external items
      clear_stack (stack[i-2]);
      clear_stack (stack[i-1]);

      stack[i-2] = output;
      for (j = i + 1; j < Nstack; j++) {
	stack[j-2] = stack[j];
      }

      Nstack -= 2;
      i -= 2;
      continue;
    }

    /***** unary operators **/
    if (stack[i].type == 9) {

      // pre-test that op and entries match
      output = db_unary (&stack[i-1], stack[i].name, fields, Nfields); 

      // free temporary stack items, drop external items
      clear_stack (stack[i-2]);
      clear_stack (stack[i-1]);

      for (j = i + 1; j < Nstack; j++) {
	stack[j-1] = stack[j];
      }

      Nstack -= 1;
      i -= 1;
      continue;
    } 
  }

  // pre-test that op and entries match
  return (TRUE);
}

/* delete name and data */
void clear_stack (dvoStack *stack) {

  if (stack->type != 'T') return;
  free (stack);
}

# include "dvo.h"

void dbInitStack (dbStack *stack) {
  stack[0].type   = DB_STACK_NONE;
  stack[0].name   = NULL;
  stack[0].field  = 0;
  stack[0].FltValue = 0.0;
  stack[0].IntValue = 0;
}

// free data for stack entries (free stack explicitly)
void dbFreeStack (dbStack *stack, int Nstack) {

  int i;

  if (stack == NULL) return;

  for (i = 0; i < Nstack; i++) {
    if (stack[i].name != NULL) {
      free (stack[i].name);
      stack[i].name = NULL;
    }
  }
}

static int NfreeStack = 0;

/* delete name and data */
void dbFreeEntry (dbStack *stack) {

  if (stack[0].name != NULL)  free (stack[0].name);
  free (stack);
  NfreeStack ++;
}

/* delete name and data */
void dbFreeTempEntry (dbStack *stack) {

  if (!(stack->type & DB_STACK_TEMP)) return;

  if (stack[0].name != NULL)  free (stack[0].name);
  free (stack);
  NfreeStack ++;
}

void dbStackFreePrint () {

  fprintf (stderr, "dbFreeStack: %d\n", NfreeStack);
}

void dbStackFreeReset () {

  NfreeStack = 0;
}

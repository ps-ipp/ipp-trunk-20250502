# include "pcontrol.h"

/* these stacks are not super efficient, and should probably be replaced with linked lists, 
   but I find these easier to get my brain around.
*/

/* Stacks and thread locks: interacting with the Stacks needs to be thread-safe so that the user may
 * perform operations which interact with the stacks at the same time that the background loops
 * check the current status of the jobs and hosts in the different stacks.  The simplest way in
 * which the stacks are made thread safe is to lock them with a mutex before every interaction
 */

# define DEBUG 0

void PrintStackInfo (Stack *stack, const char *func) {

  if (!DEBUG) return;
  fprintf (stderr, "%s: %p  ", func, (void *) stack);
  fprintf (stderr, "objects: %p  ", (void *) stack[0].object);
  fprintf (stderr, "Nobjects: %d, NOBJECTS: %d\n", stack[0].Nobject, stack[0].NOBJECT);
}

/* allocate stack, setup with default values, allocate data */
Stack *InitStack () {

  Stack *stack;

  ALLOCATE (stack, Stack, 1);

  stack[0].Nobject = 0;
  stack[0].NOBJECT = 50;

  ALLOCATE (stack[0].object, void *, stack[0].NOBJECT);
  ALLOCATE (stack[0].name,   char *, stack[0].NOBJECT);
  ALLOCATE (stack[0].id,     int,    stack[0].NOBJECT);

# ifdef THREADED
  pthread_mutex_init (&stack[0].mutex, NULL);
# endif
  return (stack);
}

void FreeStack (Stack *stack) {

  free (stack[0].object);
  free (stack[0].name);
  free (stack[0].id);
  free (stack);
}

/* STACK_TOP == 0, STACK_BOTTOM == -1 */
/* this code correctly handles the negative 'where' and Nobject == 0 */

/* push object on stack at given location */
int PushStack (Stack *stack, int where, void *object, int id, char *name) {

  int i;

  PrintStackInfo (stack, __func__);
  ASSERT (stack != NULL, "stack not set");
  LockStack (stack);

  if (where < 0) where += stack[0].Nobject + 1;
  if (where < 0) {
    UnlockStack (stack);
    return (FALSE);
  }
  if (where > stack[0].Nobject) {
    UnlockStack (stack);
    return (FALSE);
  }

  /* extend stack as needed */
  if (stack[0].Nobject >= stack[0].NOBJECT) {
    stack[0].NOBJECT += 100;
    REALLOCATE (stack[0].object, void *, stack[0].NOBJECT);
    REALLOCATE (stack[0].name,   char *, stack[0].NOBJECT);
    REALLOCATE (stack[0].id,     int, stack[0].NOBJECT);
  }

  for (i = stack[0].Nobject; i > where; i--) {
    stack[0].object[i] = stack[0].object[i-1];
    stack[0].name[i]   = stack[0].name[i-1];
    stack[0].id[i]     = stack[0].id[i-1];
  }
  stack[0].object[where] = object;
  stack[0].name[where]   = name;
  stack[0].id[where]     = id;
  stack[0].Nobject ++;

  UnlockStack (stack);
  return (TRUE);
}

/* get object from specified point in stack (negative == distance from end) */
void *PullStackByLocation (Stack *stack, int where) {

  void *object;
  
  PrintStackInfo (stack, __func__);
  ASSERT (stack != NULL, "stack not set");
  LockStack (stack);

  if (where < 0) where += stack[0].Nobject;
  if (where < 0) { 
    UnlockStack (stack); 
    return (NULL); 
  }
  if (where >= stack[0].Nobject) {
    UnlockStack (stack); 
    return (NULL);
  }

  object = stack[0].object[where];
  RemoveStackEntry (stack, where);
  UnlockStack (stack); 
  return (object);
}

/* get object from stack which matches name */
void *PullStackByName (Stack *stack, char *name) {

  int i;
  void *object;

  PrintStackInfo (stack, __func__);
  ASSERT (stack != NULL, "stack not set");
  LockStack (stack);

  for (i = 0; i < stack[0].Nobject; i++) {
    if (strcasecmp (stack[0].name[i], name)) continue;

    /* here is the element of interest */
    object = stack[0].object[i];
    RemoveStackEntry (stack, i);
    UnlockStack (stack); 
    return (object);
  }
  UnlockStack (stack); 
  return (NULL);
}

/* get object from point in stack (negative == distance from end) */
void *PullStackByID (Stack *stack, int id) {

  int i;
  void *object;
  
  PrintStackInfo (stack, __func__);
  ASSERT (stack != NULL, "stack not set");
  LockStack (stack);

  for (i = 0; i < stack[0].Nobject; i++) {
    if (stack[0].id[i] != id) continue;

    /* here is the element of interest */
    object = stack[0].object[i];
    RemoveStackEntry (stack, i);
    UnlockStack (stack); 
    return (object);
  }
  UnlockStack (stack); 
  return (NULL);
}

/* should only be called if you know where is a valid entry */
int RemoveStackEntry (Stack *stack, int where) {

  int i;

  PrintStackInfo (stack, __func__);
  ASSERT (stack != NULL, "stack not set");

  if (where < 0) abort();
  if (where >= stack[0].Nobject) abort();
  if (stack[0].Nobject < 1) abort();

  /* shift the remaining entries by one */
  /* XXX free associated memory */
  stack[0].Nobject --;
  for (i = where; i < stack[0].Nobject; i++) {
    stack[0].object[i] = stack[0].object[i+1];
    stack[0].name[i]   = stack[0].name[i+1];
    stack[0].id[i]     = stack[0].id[i+1];
  }
  return (TRUE);
}

/* should only be called if you manually lock the stack */
void *RemoveStackByID (Stack *stack, int id) {

  int i;
  void *object;
  
  PrintStackInfo (stack, __func__);
  ASSERT (stack != NULL, "stack not set");
  for (i = 0; i < stack[0].Nobject; i++) {
    if (stack[0].id[i] != id) continue;

    /* here is the element of interest */
    object = stack[0].object[i];
    RemoveStackEntry (stack, i);
    return (object);
  }
  return (NULL);
}

void LockStack (Stack *stack) {
# ifdef THREADED
  PrintStackInfo (stack, __func__);
  ASSERT (stack != NULL, "stack not set");
  pthread_mutex_lock (&stack[0].mutex);
# endif
  return;
}

void UnlockStack (Stack *stack) {
# ifdef THREADED
  PrintStackInfo (stack, __func__);
  ASSERT (stack != NULL, "stack not set");
  pthread_mutex_unlock (&stack[0].mutex);
# endif
  return;
}

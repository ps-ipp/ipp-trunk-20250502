# include "pcontrol.h"

/* Stacks and thread locks: interacting with the Stacks needs to be thread-safe so that the user may
 * perform operations which interact with the stacks at the same time that the background loops
 * check the current status of the jobs and hosts in the different stacks.  The simplest way in
 * which the stacks are made thread safe is to lock them with a mutex before every interaction
 */

# define DEBUG 0

void PrintStackInfo (Stack *stack, const char *func) {

  if (!DEBUG) return;
  fprintf (stderr, "%s: %p  ", func, stack);
  fprintf (stderr, "objects: %p  ", stack[0].object);
  fprintf (stderr, "Nobjects: %d (linked list)\n", stack[0].Nobject);
}

/* allocate stack: create head node (empty node?) */
Stack *InitStack () {

  Stack *stack;

  ALLOCATE (stack, Stack, 1);

  stack->Nobject = 0;
  stack->head = NULL;
  stack->tail = NULL;

# ifdef THREADED
  pthread_mutex_init (&stack[0].mutex, NULL);
# endif
  return (stack);
}

void FreeStack (Stack *stack) {

  ASSERT (!stack->head, "stack items not freed");
  ASSERT (!stack->tail, "stack items not freed");
  free (stack);
}

/* STACK_TOP == 0, STACK_BOTTOM == -1 
   in this version, entries by seq number other than head and tail are not allowed
 */

StackItem InitStackItem (void *object, char *name, int id) {

  StackItem *item;
  ALLOCATE (item, StackItem, 1);
  item->object = object;
  item->name = name;
  item->id = id;

  return item;
}

/* push object on stack at given location */
int PushStack (Stack *stack, int where, void *object, int id, char *name) {

  int i;

  ASSERT (where == STACK_TOP || where == STACK_BOTTOM, "invalid location");
  ASSERT (stack != NULL, "stack not set");

  PrintStackInfo (stack, __func__);
  LockStack (stack);

  // make this a StackItem generator
  StackItem *item = InitStackItem (object, name, id);
  ALLOCATE (item, StackItem, 1);

  /* there is a special case when the stack is empty.  in this case, we have to update
     both head and tail.
   */
  if (!stack->head && !stack->tail) {
    stack->head = item;
    stack->tail = item;
    stack->Nobject ++;
    UnlockStack (stack);
    return (TRUE);
  }

  if (where == STACK_TOP) {
    StackHead *oldHead = stack->head;
    stack->head = item;
    item->next = oldHead;
    oldHead->prev = item;
  } else {
    StackHead *oldTail = stack->tail;
    stack->tail = item;
    item->prev = oldTail;
    oldTail->next = item;
  }
  stack->Nobject ++;
  UnlockStack (stack);
  return (TRUE);
}

/* get object from specified point in stack (negative == distance from end) */
void *PullStackByLocation (Stack *stack, int where) {

  void *object;
  
  ASSERT (where == STACK_TOP || where == STACK_BOTTOM, "invalid location");
  ASSERT (stack != NULL, "stack not set");

  PrintStackInfo (stack, __func__);
  LockStack (stack);

  if (

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

/*** I need a function to sort a stack by something.
     I also need to replace these arrays with linked lists.
 ***/

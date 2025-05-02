# include "elixir.h"

/* get object from top of stack */
Object *GetObject (Queue *queue) {

  int i;
  Object *object;
  
  if (queue[0].Nobject == 0) return ((Object *) NULL);

  object = queue[0].object[0];
  queue[0].Nobject --;
  for (i = 0; i < queue[0].Nobject; i++) {
    queue[0].object[i] = queue[0].object[i+1];
  }
  return (object);
}

/* put object on bottom of stack */
void PutObject (Queue *queue, Object *object) {

  if (queue[0].Nobject == queue[0].NOBJECT) {
    queue[0].NOBJECT += 100;
    REALLOCATE (queue[0].object, Object *, queue[0].NOBJECT);
  }
  queue[0].object[queue[0].Nobject] = object;
  queue[0].Nobject ++;

}

/* push object on top of stack */
void PushObject (Queue *queue, Object *object) {

  int i;

  if (queue[0].Nobject == queue[0].NOBJECT) {
    queue[0].NOBJECT += 100;
    REALLOCATE (queue[0].object, Object *, queue[0].NOBJECT);
  }
  for (i = queue[0].Nobject; i > 0; i--) {
    queue[0].object[i] = queue[0].object[i-1];
  }
  queue[0].object[0] = object;
  queue[0].Nobject ++;

}

/* allocate queue, setup with default values, allocate data */
Queue *InitQueue () {

  Queue *queue;

  ALLOCATE (queue, Queue, 1);

  queue[0].Nobject = 0;
  queue[0].NOBJECT = 50;
  ALLOCATE (queue[0].object, Object *, queue[0].NOBJECT);
  return (queue);

}

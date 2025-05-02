# include "elixir.h"

int CheckProcess (Process *process) {

  Machine *machine;
  int cargc, *cargd;
  char **cargv;
  Object *object;

  CheckCluster (process[0].cluster, process[0].success, process[0].failure, process[0].pending);
  if (drand48() > 0.5) return (FALSE);


  if ((object = GetObject (process[0].pending)) == (Object *) NULL) return (FALSE);
  object[0].lastproc = process[0].name;

  /* can't create arguments, some dependencies might not be ready */
  if (!MakeArgs (process, object, &cargc, &cargv, &cargd)) {
    FreeArgs (cargc, cargv, cargd);
    PutObject (process[0].pending, object);
    return (FALSE);
  }

  /* dependencies not ready */
  switch (CheckDepend (object, cargc, cargv, cargd)) {
  case 0:
    FreeArgs (cargc, cargv, cargd);
    PutObject (process[0].pending, object);
    return (FALSE);
  case 2:
    FreeArgs (cargc, cargv, cargd);
    PutObject (process[0].failure, object);
    return (FALSE);
  case 1:
  default:
    break;
  }
    
  /* no machine ready */
  if ((machine = GrabMachine ()) == (Machine *) NULL) {
    /* need to free appropriate items from above */
    FreeArgs (cargc, cargv, cargd);
    PushObject (process[0].pending, object);
    return (FALSE);
  }

  StartMachine (machine, object, cargc, cargv);
  FreeArgs (cargc, cargv, cargd);
  StartProcessTimer (process, machine);

  PutMachine (machine, process[0].cluster);
  return (TRUE);
}

/* objects that are not ready go to the bottom of the stack (PutObject)
   objects that are ready, but have no machine free, go to top of stack (PushObject) 
*/

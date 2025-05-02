# include "elixir.h"

Process *InitProcess (char *name, Queue *pending, Queue *failure, int (*mkargs)()) {

  Process *process;

  ALLOCATE (process, Process, 1);

  process[0].success = InitQueue ();
  process[0].failure = failure;
  process[0].pending = pending;

  process[0].name    = strcreate (name);
  /* process[0].mkarg   = mkargs; */

  process[0].cluster = InitCluster ();

  return (process);
}

# include "opihi.h"

int exec_loop (Macro *loop) {

  int j, status, ThisList;
  char *line;
  
  /* increase the shell level (Nlists) by one */
  ThisList = increase_list_depth();
  if (ThisList == 0) abort();

  /* copy the macro to the current list */
  for (j = 0; j < loop[0].Nlines; j++) {
    add_listentry (ThisList, loop[0].line[j]);
  }

  /* set up interrupts */
  struct sigaction *old_sigaction = SetInterrupt();

  /* process the list */
  loop_next = loop_break = loop_last = FALSE;
  status = TRUE;

  while (!interrupt) {
    line = get_next_listentry (ThisList);
    if (line == NULL) break;
    status = multicommand (line);
    free (line);
    if (auto_break && !status) loop_break = TRUE;
    if (loop_break || loop_last || loop_next) break;
  }
  ClearInterrupt (old_sigaction);

  /* free remaining lines on the list, free the list, decrement the shell level */
  /*** can we free a list which is not the bottom lists? */
  decrease_list_depth();

  if (loop_break) return (FALSE);
  return (TRUE);
}

/** note that the list number runs from 1 - Nlists+1 **/

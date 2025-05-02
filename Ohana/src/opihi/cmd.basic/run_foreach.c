# include "basic.h"
# define D_NLINES 100
static char prompt[] = ">> ";

int run_foreach (int argc, char **argv) {

  int ThisList, depth, i, done, status, NLINES, j;
  char *input;
  Macro loop;

  if (argc < 2) {
    gprint (GP_ERR, "USAGE: foreach (var) value [value value ...] -- terminate with 'END'\n");
    return (FALSE);
  }

  NLINES = D_NLINES;
  ALLOCATE (loop.line, char *, NLINES);

  /* read in loop */
  depth = 0;
  ThisList = current_list_depth();
  for (i = 0, done = FALSE; !done; ) {

    /* get the next line (from correct place) */
    if (ThisList == 0) 
      input = opihi_readline (prompt);
    else 
      input = get_next_listentry (ThisList);
    stripwhite (input);

    /* check for end-of-data syntax error */
    if (input == NULL) {
      if (ThisList == 0)  {
	gprint (GP_ERR, "end loop with 'END'\n");
	continue;
      } else {
	gprint (GP_ERR, "misbalanced loop\n");
	for (j = 0; j < loop.Nlines; j++) {
	  free (loop.line[j]);
	}
	free (loop.line);
	return (FALSE);
      }	
    }

    /* test for new macro (or other list, in the future?) */
    if (is_list (input)) depth ++;

    /* test for end of nested list -- if not nested, END refers to this macro */
    if (!strncasecmp (input, "END", 3)) {
      depth --;
      if (depth < 0) { 
	free (input);
	break;
      }
    }

    /* if line has data, add to loop list */
    if (*input) { 
      loop.line[i] = input;
      i++;
      if (i == NLINES - 1) {
	NLINES += D_NLINES;
	REALLOCATE (loop.line, char *, NLINES);
      }
    }
  }

  /* cleanup loop data */
  loop.Nlines = i;
  REALLOCATE (loop.line, char *, MAX (loop.Nlines, 1));

  status = TRUE;
  for (i = 2; (i < argc) && !interrupt; i++) {
    set_str_variable (argv[1], argv[i]);
    status = exec_loop (&loop);
    if (loop_next) continue;
    if (loop_last) break;
    if (loop_break) break;
  }
  /* 'last' and 'next' should only affect one loop */
  loop_last = loop_next = FALSE; 

  /* break should propagate up if auto_break is set */
  loop_break = FALSE;
  if (auto_break && !status) loop_break = TRUE;

  /* cleanup list */
  for (j = 0; j < loop.Nlines; j++) {
    free (loop.line[j]);
  }
  free (loop.line);

  if (loop_break) return (FALSE);
  return (TRUE);
}

/*
  If we are entering at the keyboard (ThisList == 0), use readline.
  Otherwise, read from the current list, remove list lines.
  execute when we hit the final "END" (the true END -- we count macro defines!!) 
*/
     
/* while processing the loop, the loop status variables may be set
   by the loop commands, or the loop may quite on a failed command,
   setting the exec_loop return status to false. the loop status variables
   are:
   loop_next : stop this loop, but try another loop
   loop_last : stop loop processing, but return true so external loop may continue
   loop_break : stop loop processing, and return false so external loop will break
   interrupt : external interrupt signal
*/


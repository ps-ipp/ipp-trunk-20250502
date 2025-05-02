# include "basic.h"
# define D_NLINES 100
static char prompt[] = ">> ";

int run_while (int argc, char **argv) {

  int ThisList, depth, i, done, status, NLINES, j;
  char *input, *val, *logic_line;
  int logic, size;
  Macro loop;

  if (argc != 2) {
    gprint (GP_ERR, "USAGE: while (condition)\n");
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
    if (input == (char *) NULL) {
      if (ThisList == 0)  {
	gprint (GP_ERR, "end 'while' loop with 'END'\n");
	continue;
      } else {
	gprint (GP_ERR, "misbalanced 'while' loop\n");
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
      if (depth < 0) break;
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
  free (input);
  loop.Nlines = i;
  REALLOCATE (loop.line, char *, MAX (loop.Nlines, 1));

  // test the logic once before running the loop
  logic_line = strcreate (argv[1]);
  logic_line = expand_vars (logic_line); // recalculates scalar elements
  logic_line = expand_vectors (logic_line); // recalculates vector/matrix elements (e.g., vec[])
  val = dvomath (1, &logic_line, &size, 0);
  free (logic_line);

  // if we have a parse failure, return FALSE
  if (val == NULL) return (FALSE);
  logic = atof (val); /* warning: round-off error is a danger */
  free (val);

  /* execute for loop */
  status = FALSE;
  while (logic) { 
    status = exec_loop (&loop);
    if (loop_next) continue;
    if (loop_last) break;
    if (loop_break) break;

    logic_line = strcreate (argv[1]);
    logic_line = expand_vars (logic_line); // recalculates scalar elements
    logic_line = expand_vectors (logic_line); // recalculates vector/matrix elements (e.g., vec[])
    val = dvomath (1, &logic_line, &size, 0);
    free (logic_line);

    logic = FALSE;
    if (val) {
      logic = atof (val); /* warning: round-off error is a danger */
      free (val);
    }
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

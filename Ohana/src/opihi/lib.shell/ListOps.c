# include "opihi.h"
# include "macro.h"

/*** local static variables used to track the command lists  ***/
static List *lists = NULL;	/* variable to store the list of all lists */
static int  Nlists = 0;		/* number of currently available lists */

void InitLists () {
  Nlists = 0;
  ALLOCATE (lists, List, 1); 
  return;
}

void FreeLists () {

  int i, j;

  // Nlists is a bit weird: it is the currently highest valid list, not the number of lists
  // Nlists = 0 is never allocated
  for (i = 1; i < Nlists + 1; i++) {
    for (j = 0; j < lists[i].Nlines; j++) {
      free (lists[i].line[j]);
    }
    free (lists[i].line);
  }
  free (lists);
}

int current_list_depth () {
  return Nlists;
}

int increase_list_depth () {
  Nlists ++;
  REALLOCATE (lists, List, MAX (Nlists + 1, 0) + 1);
  ALLOCATE (lists[Nlists].line, char *, 16);
  lists[Nlists].Nalloc = 16;
  lists[Nlists].Nlines = 0;
  lists[Nlists].n = 0;
  return Nlists;
}

int decrease_list_depth () {
  
  int i;

  for (i = 0; i < lists[Nlists].Nlines; i++) {
    free (lists[Nlists].line[i]);
  }
  free (lists[Nlists].line);
  Nlists --;
  REALLOCATE (lists, List, MAX (Nlists + 1, 0) + 1);
  return Nlists;
}

/* return a new string consisting of the next line in the current list */
char *get_next_listentry (int ThisList) {

  int Nline;
  char *output;

  Nline = lists[ThisList].n;

  if (Nline >= lists[ThisList].Nlines) return (NULL);

  output = strcreate (lists[ThisList].line[Nline]);
  lists[ThisList].n ++;
  
  return (output);
}

# if (0)
char *remove_listentry (int current) {

  int i;
  char *output;

  if ((current + 1) >= lists[Nlists].Nlines) 
    return ((char *) NULL);

  output = lists[Nlists].line[current + 1];
  
  for (i = current + 1; i < lists[Nlists].Nlines - 1; i++) {
    lists[Nlists].line[i] = lists[Nlists].line[i + 1];
  }

  lists[Nlists].Nlines --;
  return (output);

}
# endif 

int add_listentry (int ThisList, char *line) {

  int Nlines;

  Nlines = lists[ThisList].Nlines;
  lists[ThisList].line[Nlines] = strcreate (line);
  lists[ThisList].Nlines ++;

  if (lists[ThisList].Nlines == lists[ThisList].Nalloc) {
    lists[ThisList].Nalloc += 16;
    REALLOCATE (lists[ThisList].line, char *, lists[ThisList].Nalloc);
  }
    
  return (lists[ThisList].Nlines);
}

int is_for_loop (char *line) {

  int status;
  char *comm;

  comm = thisword (line);
  if (comm == (char *) NULL) return (FALSE);
  
  status = !strcmp (comm, "for");
  free (comm);
  return (status);
}

int is_foreach_loop (char *line) {

  int status;
  char *comm;

  comm = thisword (line);
  if (comm == (char *) NULL) return (FALSE);
  
  status = !strcmp (comm, "foreach");
  free (comm);
  return (status);
}

int is_macro_create (char *line) {

  int status;
  char *comm;
  char *this_macro;
  CommandF *cmd;

  comm = thisword (line);
  if (comm == NULL) return (FALSE);

  status = !strcmp (comm, "macro");
  free (comm);
  if (!status) return (FALSE);
  
  this_macro = thisword (nextword (line));
  if (this_macro == NULL) return (FALSE);

  cmd = find_macro_command (this_macro);
  free (this_macro);

  if (cmd == NULL) {
    return (FALSE);
  }

  return (TRUE);
}

int is_if_block (char *line) {

  char *comm, *temp;

  temp = thisword (nextword (nextword (line)));
  comm = thisword (line);

  if (comm == NULL) goto escape;

  if (strcmp (comm, "if")) goto escape;

  /* if (cond) (command) does not define a complete block */
  if (temp != NULL) goto escape;

  if (temp != NULL) free (temp);
  if (comm != NULL) free (comm);
  return (TRUE);

escape: 
  if (comm != NULL) free (comm);
  if (temp != NULL) free (temp);
  return (FALSE);
}

// list (word) : nested list
// list (word) -x : not nested list [-x may be the list of options below]
// list -excel (word) -x : 
int is_list_data (char *line) {

  char *comm = NULL;
  char *temp = NULL;
  char *name  = NULL;
  char *opts  = NULL;

  comm = thisword (line);
  if (comm == NULL) goto escape;
  if (strcmp (comm, "list")) goto escape;

  // name == name of the list in question
  name = nextword (line);

  // skip a -excel or -excel-style modifier
  if (!strncmp("-excel", name, strlen("-excel"))) name = nextword (name);
  if (!strncmp("-excel-style", name, strlen("-excel-style"))) name = nextword (name);

  // get the thing following ptr, if any (opts points at a substring, temp is a new string)
  opts = nextword (name);
  temp = thisword (opts);

  /* if (cond) (command) does not define a complete block */
  if (temp != NULL) {
      if (!strcmp (temp, "-x")) goto escape;
      if (!strcmp (temp, "-glob")) goto escape;
      if (!strcmp (temp, "-file")) goto escape;
      if (!strcmp (temp, "-split")) goto escape;
      if (!strcmp (temp, "-join")) goto escape;
      if (!strcmp (temp, "-splitbychar")) goto escape;
      if (!strcmp (temp, "-copy")) goto escape;
      if (!strcmp (temp, "-add")) goto escape;
      if (!strcmp (temp, "-del")) goto escape;
      if (!strcmp (temp, "-vectors")) goto escape;
      if (!strcmp (temp, "-buffers")) goto escape;
  }

  if (temp != NULL) free (temp);
  if (comm != NULL) free (comm);
  return (TRUE);

escape: 
  if (comm != NULL) free (comm);
  if (temp != NULL) free (temp);
  return (FALSE);
}

int is_loop (char *line) {

  int status;
  char *comm;

  comm = thisword (line);
  if (comm == (char *) NULL) return (FALSE);

  status = !strcmp (comm, "while");
  free (comm);
  return (status);
}

int is_task (char *line) {

  int status;
  char *comm;

  comm = thisword (line);
  if (comm == (char *) NULL) return (FALSE);

  status = !strcmp (comm, "task");
  free (comm);
  return (status);
}

int is_task_exit (char *line) {

  int status;
  char *comm;

  comm = thisword (line);
  if (comm == (char *) NULL) return (FALSE);

  status = !strcmp (comm, "task.exit");
  free (comm);
  return (status);
}

int is_task_exec (char *line) {

  int status;
  char *comm;

  comm = thisword (line);
  if (comm == (char *) NULL) return (FALSE);

  status = !strcmp (comm, "task.exec");
  free (comm);
  return (status);
}

int is_list (char *line) {
  
  int status;

  status = is_if_block (line);
  status |= is_macro_create (line);
  status |= is_for_loop (line);
  status |= is_foreach_loop (line);
  status |= is_list_data (line);
  status |= is_loop (line);
  status |= is_task (line);
  status |= is_task_exit (line);
  status |= is_task_exec (line);

  return (status);

}

# include "basic.h"
# include <glob.h>
# define D_NLINES 100
# define MAX_LINE_LENGTH 1024
static char prompt[] = ">> ";

int SplitByCharToList (int argc, char **argv, int EXCEL_STYLE);
int GlobToList (char *listname, char *Glob, int EXCEL_STYLE);
int CommandToList (char *listname, char *Command, int EXCEL_STYLE);
int FileToList (char *listname, char *Filename, int EXCEL_STYLE);

int list (int argc, char **argv) {

  int ThisList, depth, i, done, found;
  char *input, line[MAX_LINE_LENGTH];
  int N;

  char *Command = NULL;
  if ((N = get_argument (argc, argv, "-x"))) {
    remove_argument (N, &argc, argv);
    Command = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "-exec"))) {
    remove_argument (N, &argc, argv);
    Command = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }

  char *Glob = NULL;
  if ((N = get_argument (argc, argv, "-glob"))) {
    remove_argument (N, &argc, argv);
    Glob = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }

  char *Filename = NULL;
  if ((N = get_argument (argc, argv, "-file"))) {
    remove_argument (N, &argc, argv);
    Filename = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }

  int EXCEL_STYLE = FALSE;
  if ((N = get_argument (argc, argv, "-excel-style"))) {
    remove_argument (N, &argc, argv);
    EXCEL_STYLE = TRUE;
  }
  if ((N = get_argument (argc, argv, "-excel"))) {
    remove_argument (N, &argc, argv);
    EXCEL_STYLE = TRUE;
  }

  if ((N = get_argument (argc, argv, "-vectors"))) {
    remove_argument (N, &argc, argv);
    if (argc != 2) {
      gprint (GP_ERR, "USAGE: list (root) -vectors\n");
      return (FALSE);
    }
    ListVectorsToList (argv[1]); // <-- generate the list
    return TRUE;
  }

  if ((N = get_argument (argc, argv, "-buffers"))) {
    remove_argument (N, &argc, argv);
    if (argc != 2) {
      gprint (GP_ERR, "USAGE: list (root) -buffers\n");
      return (FALSE);
    }
    ListBuffersToList (argv[1]); // <-- generate the list
    return TRUE;
  }

  if (Command) {
    if (argc != 2) {
      gprint (GP_ERR, "USAGE: list (root) -exec (command)\n");
      gprint (GP_ERR, "   OR: list (root) -x (command)\n");
      return (FALSE);
    }
    int status = CommandToList (argv[1], Command, EXCEL_STYLE); // <-- generate the list
    return status;
  }

  if (Glob) {
    if (argc != 2) {
      gprint (GP_ERR, "USAGE: list (root) -glob (fileglob)\n");
      return (FALSE);
    }
    int status = GlobToList (argv[1], Glob, EXCEL_STYLE); // <-- generate the list
    return status;
  }

  if (Filename) {
    if (argc != 2) {
      gprint (GP_ERR, "USAGE: list (root) -file (filename)\n");
      return (FALSE);
    }
    int status = FileToList (argv[1], Filename, EXCEL_STYLE); // <-- generate the list
    return status;
  }

  // return an error if -add is given with no other args
  if ((argc > 2) && (!strcmp (argv[2], "-split"))) {
    if (argc == 3) {
      gprint (GP_ERR, "USAGE: list (root) -split (word) (word) ...\n");
      return (FALSE);
    }
    
    for (i = 0; i < argc - 3; i++) {
      set_list_varname (line, argv[1], i, EXCEL_STYLE);
      set_str_variable (line, argv[i+3]);
    }
    snprintf_nowarn (line, MAX_LINE_LENGTH, "%s:n", argv[1]);
    set_int_variable (line, i);

    return (TRUE);
  }

  // return an error if -add is given with no other args
  if ((argc > 2) && (!strcmp (argv[2], "-join"))) {
    int start = 0;
    if ((N = get_argument (argc, argv, "-start"))) {
      remove_argument (N, &argc, argv);
      start = atoi (argv[N]);
      remove_argument (N, &argc, argv);
    }

    if (argc != 4) {
      gprint (GP_ERR, "USAGE: list (root) -join (variable)\n");
      return (FALSE);
    }
    
    int isFound;

    snprintf_nowarn (line, MAX_LINE_LENGTH, "%s:n", argv[1]);
    int nList = get_int_variable (line, &isFound);
    if (!isFound) {
      gprint (GP_ERR, "list %s not found\n", argv[1]);
      return FALSE;
    }

    char tmpline[MAX_LINE_LENGTH], output[MAX_LINE_LENGTH];
    memset (tmpline, 0, MAX_LINE_LENGTH);
    memset (output, 0, MAX_LINE_LENGTH);

    for (i = start; i < nList; i++) {
      snprintf_nowarn (line, MAX_LINE_LENGTH, "%s:%d", argv[1], i);
      char *word = get_variable (line);
      if (!word) break;
      if (i > 0) {
	snprintf_nowarn (output, MAX_LINE_LENGTH, "%s %s", tmpline, word);
      } else {
	snprintf_nowarn (output, MAX_LINE_LENGTH, "%s", word);
      }
      strcpy (tmpline, output);
    }
    set_str_variable (argv[3], output);

    return (TRUE);
  }

  // return an error if -add is given with no other args
  if ((argc > 2) && (!strcmp (argv[2], "-splitbychar"))) {
    if (argc < 4) {
      gprint (GP_ERR, "USAGE: list (root) -splitbychar (char) (word) [(word)...]\n");
      return (FALSE);
    }
    int status = SplitByCharToList (argc, argv, EXCEL_STYLE); // <-- generate the list
    return status;
  }

  // return an error if -copy is given with no other args
  if ((argc > 2) && (!strcmp (argv[2], "-copy"))) {
    char *value;
    if (argc == 3) {
      gprint (GP_ERR, "USAGE: list (newlist) -copy (oldlist) ...\n");
      return (FALSE);
    }
    
    // old list must exist, or give an error
    snprintf_nowarn (line, MAX_LINE_LENGTH, "%s:n", argv[3]);
    N = get_int_variable (line, &found);
    if (!found) {
      gprint (GP_ERR, "USAGE: list (newlist) -copy (oldlist) ...\n");
      gprint (GP_ERR, "ERROR: missing input list\n");
      return (FALSE);
    }
      

    snprintf_nowarn (line, MAX_LINE_LENGTH, "%s:n", argv[1]);
    set_int_variable (line, N);
    for (i = 0; i < N; i++) {
      snprintf_nowarn (line, MAX_LINE_LENGTH, "%s:%d", argv[3], i);
      value = get_variable (line);
      // snprintf_nowarn (line, MAX_LINE_LENGTH, "%s:%d", argv[1], i);
      set_list_varname (line, argv[1], i, EXCEL_STYLE);
      set_str_variable (line, value);
    }
    return (TRUE);
  }

  // return an error if -add is given with no other args
  if ((argc > 2) && (!strcmp (argv[2], "-add"))) {
    if (argc == 3) {
      gprint (GP_ERR, "USAGE: list (root) -add (word) (word) ...\n");
      return (FALSE);
    }
    
    snprintf_nowarn (line, MAX_LINE_LENGTH, "%s:n", argv[1]);
    N = get_int_variable (line, &found);
    for (i = 0; i < argc - 3; i++) {
      // snprintf_nowarn (line, MAX_LINE_LENGTH, "%s:%d", argv[1], N + i);
      set_list_varname (line, argv[1], N + i, EXCEL_STYLE);
      set_str_variable (line, argv[i+3]);
    }
    snprintf_nowarn (line, MAX_LINE_LENGTH, "%s:n", argv[1]);
    set_int_variable (line, N + i);

    return (TRUE);
  }

  // remove the single named entry from the list (finds entry with given name, reduces list length by one)
  if ((argc > 2) && (!strcmp (argv[2], "-del"))) {
    if (argc != 4) {
      gprint (GP_ERR, "USAGE: list (root) -del (word)\n");
      return (FALSE);
    }
    
    int j;
    char *value, *next_value;
    char line2[MAX_LINE_LENGTH];

    snprintf_nowarn (line, MAX_LINE_LENGTH, "%s:n", argv[1]); // line = LIST:n
    N = get_int_variable (line, &found);

    // loop over all list elements:
    for (i = 0; i < N; i++) {
      // snprintf_nowarn (line, MAX_LINE_LENGTH, "%s:%d", argv[1], i);
      set_list_varname (line, argv[1], i, EXCEL_STYLE); // line = LIST:i
      value = get_variable (line);
      if (value == NULL) continue;

      // if this is the entry we want, delete and shift the rest down
      if (!strcmp (value, argv[3])) {
	free (value);
	for (j = i + 1; j < N; j++) {
	  // snprintf_nowarn (line, MAX_LINE_LENGTH, "%s:%d", argv[1], j);
	  set_list_varname (line2, argv[1], j, EXCEL_STYLE); // line2 = LIST:j
	  next_value = get_variable (line2);
	  set_str_variable (line, next_value);
	  strcpy (line, line2); // line = LIST:j (will be j-1 next loop)
	}
	DeleteNamedScalar (line); // line = LIST:(N-1)
	snprintf_nowarn (line, MAX_LINE_LENGTH, "%s:n", argv[1]); // line = LIST:n (new value of n)
	set_int_variable (line, N - 1);
	return (TRUE);
      }
      free (value);
    }      
    gprint (GP_ERR, "value %s not found in list\n", argv[3]);
    return (FALSE);
  }

  if (argc != 2) {
    gprint (GP_ERR, "USAGE: list (root)                			       : supply list data, terminate with 'END'\n");
    gprint (GP_ERR, "USAGE: list (root) -x (command)   			       : create list from shell output\n");
    gprint (GP_ERR, "USAGE: list (root) -exec (command)   		       : create list from shell output\n");
    gprint (GP_ERR, "USAGE: list (root) -vectors   			       : create list from vector names\n");
    gprint (GP_ERR, "USAGE: list (root) -buffers   			       : create list from buffer names\n");
    gprint (GP_ERR, "USAGE: list (root) -split (words) 			       : create list from words\n");
    gprint (GP_ERR, "USAGE: list (root) -splitbychar (char) (word) [(words)..] : create list from words\n");
    gprint (GP_ERR, "USAGE: list (root) -join (varname) 		       : convert a list to a single variable (space separated)\n");
    gprint (GP_ERR, "USAGE: list (root) -add (words)   			       : extend a list\n");    
    gprint (GP_ERR, "USAGE: list (root) -copy (list)   			       : copy a list to a new name\n");
    gprint (GP_ERR, "USAGE: list (root) -del (word)   			       : delete the entry by value\n");
    gprint (GP_ERR, "USAGE: list (root) -glob (word)   			       : create a list from a file glob\n");
    gprint (GP_ERR, "USAGE: list (root) -file (word)   			       : create a list from lines in a file\n");

    gprint (GP_ERR, "OPTIONS: -excel   			                       : generate names of the for foo:A\n");
    gprint (GP_ERR, "OPTIONS: -excel-style  	                               : generate names of the for foo:A\n");
    return (FALSE);
  }

  /* read in loop */
  depth = 0;
  ThisList = current_list_depth();
  for (i = 0, done = FALSE; !done; ) {

    /* get the next line (from correct place) */
    if (ThisList == 0) {
      input = opihi_readline (prompt);
    } else {
      input = get_next_listentry (ThisList);
    }

    if ((ThisList == 0) && (input == NULL)) {
      gprint (GP_ERR, "end list with 'END'\n");
      continue;
    }
    if ((ThisList >  0) && (input == NULL)) {
      gprint (GP_ERR, "missing 'END' in list\n");
      input = strcreate ("end");
    }

    stripwhite (input);

    /* test for end of nested list -- if not nested, END refers to this macro */
    if (!strncasecmp (input, "END", 3)) {
      depth --;
      if (depth < 0) { /* we hit the last "END", loop is done */
	snprintf_nowarn (line, MAX_LINE_LENGTH, "%s:n", argv[1]);
	set_int_variable (line, i);
	free (input);
	return (TRUE);
      }
    }

    if (*input) { 
      // snprintf_nowarn (line, MAX_LINE_LENGTH, "%s:%d", argv[1], i);
      set_list_varname (line, argv[1], i, EXCEL_STYLE);
      set_str_variable (line, input);
      free (input);
      i++;
    }
  }
  return (TRUE);
}

int CommandToList (char *listname, char *Command, int EXCEL_STYLE) {

  char line[MAX_LINE_LENGTH];

  /* val will hold the result */
  int NBYTES = MAX_LINE_LENGTH;
  char *val;
  ALLOCATE (val, char, NBYTES);
    
  /* need to loop until command produces no more output, 
     REALLOCATING as needed. */
  FILE *f = popen (Command, "r");

  int done = FALSE;
  int Nbytes = 0;
  while (!done) {
    int Nread = fread (&val[Nbytes], 1, 1023, f);
    if (Nread < 0) { 
      gprint (GP_ERR, "error reading from command\n");
      done = TRUE;
    }
    if (Nread > 0) {
      Nbytes += Nread;
      NBYTES = MAX_LINE_LENGTH + Nbytes;
      REALLOCATE (val, char, NBYTES);
    }
    if (Nread == 0) {
      done = TRUE;
    }
  }
  val[Nbytes] = 0;
  int status = pclose (f);
  free (Command);
    
  if (status) {
    gprint (GP_ERR, "warning: exit status of command %d\n", status);
  }
      
  int i;
  char *A = val;
  char *B = val;
  for (i = 0; B != (char *) NULL;) {
    while (isspace (*A) && (*A != 0)) A++;
    B = strchr (A, '\n');
    if (B != (char *) NULL) { *B = 0; }
    if (*A != 0) {
      set_list_varname (line, listname, i, EXCEL_STYLE);
      set_str_variable (line, A);
      A = B + 1;
      i++;
    }
  }      
  free (val);
    
  snprintf_nowarn (line, MAX_LINE_LENGTH, "%s:n", listname);
  set_int_variable (line, i);
  return (TRUE);
}

int GlobToList (char *listname, char *Glob, int EXCEL_STYLE) {

  int i;
  char line[MAX_LINE_LENGTH];
  glob_t globList;

  // parse the filename as a glob
  globList.gl_offs = 0;
  glob (Glob, 0, NULL, &globList);

  // if the glob does not match, save the literal word:
  // otherwise save all glob matches
  if (globList.gl_pathc == 0) {
    snprintf_nowarn (line, MAX_LINE_LENGTH, "%s:n", listname);
    set_int_variable (line, 0);
    return TRUE;
  }

  int Nfile = globList.gl_pathc;
  for (i = 0; i < Nfile; i++) {
    set_list_varname (line, listname, i, EXCEL_STYLE);
    set_str_variable (line, globList.gl_pathv[i]);
  }
  snprintf_nowarn (line, MAX_LINE_LENGTH, "%s:n", listname);
  set_int_variable (line, Nfile);
  return (TRUE);
}

int FileToList (char *listname, char *Filename, int EXCEL_STYLE) {

  char line[MAX_LINE_LENGTH], varname[MAX_LINE_LENGTH];

  FILE *f = fopen (Filename, "r");
  if (!f) {
    gprint (GP_ERR, "cannot open file %s for list\n", Filename);
    return FALSE;
  }

  int Nline = 0;
  while (scan_line_maxlen (f, line, MAX_LINE_LENGTH) != EOF) {
    set_list_varname (varname, listname, Nline, EXCEL_STYLE);
    set_str_variable (varname, line);
    Nline ++;
  }
  snprintf_nowarn (varname, MAX_LINE_LENGTH, "%s:n", listname);
  set_int_variable (varname, Nline);
  return TRUE;
}

int SplitByCharToList (int argc, char **argv, int EXCEL_STYLE) {

  int i, j;
  char line[MAX_LINE_LENGTH];

  int nWords = 0;
  char splitter = argv[3][0];

  for (i = 0; i < argc - 3; i++) {
    char *new = strcreate (argv[i+3]);
    for (j = 0; new[j]; j++) {
      if (new[j] == splitter) new[j] = ' ';
    }

    char *ptr = new;
    while (ptr) {
      char *word = thisword (ptr);
      if (!word) break;
	
      set_list_varname (line, argv[1], nWords, EXCEL_STYLE);

      set_str_variable (line, word);
      FREE (word);
      ptr = nextword (ptr);
      nWords ++;
    }
    FREE (new);
  }
  snprintf_nowarn (line, MAX_LINE_LENGTH, "%s:n", argv[1]);
  set_int_variable (line, nWords);

  return (TRUE);
}

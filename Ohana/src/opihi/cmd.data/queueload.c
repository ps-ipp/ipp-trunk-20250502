# include "data.h"

int queueload (int argc, char **argv) {
  
  int N;

  // variable name to save the value
  int TrapFailure = FALSE;
  if ((N = get_argument (argc, argv, "-trap-failure"))) {
    remove_argument (N, &argc, argv);
    TrapFailure = TRUE;
  }
  if ((N = get_argument (argc, argv, "-trap"))) {
    remove_argument (N, &argc, argv);
    TrapFailure = TRUE;
  }

  if (argc != 4) goto usage;
  if (strcmp(argv[2], "-x")) goto usage;
  
  /* will create a queue if none exists */
  Queue *queue = CreateQueue (argv[1]);

  /* val will hold the result of the command */
  int NBYTES = 1024;
  ALLOCATE_PTR (val, char, NBYTES);
    
  /* loop until command produces no more output,  REALLOCATE as needed. */
  FILE *f = popen (argv[3], "r");

  int Nbytes = 0;
  int Nread = 1;
  while (Nread > 0) {
    Nread = fread (&val[Nbytes], 1, 1023, f);
    if (Nread < 0) { 
      gprint (GP_ERR, "error reading from command\n");
    }
    if (Nread > 0) {
      Nbytes += Nread;
      NBYTES = 1024 + Nbytes;
      REALLOCATE (val, char, NBYTES);
    }
  }
  val[Nbytes] = 0;
  int status = pclose (f);
    
  // XXX 
  if (status) {
    gprint (GP_ERR, "warning: exit status of command %d\n", status);
    if (TrapFailure) { free (val); return FALSE; }
  }
      
  char *A = val, *B = val;
  for (int i = 0; B != (char *) NULL;) {
    while (isspace (*A) && (*A != 0)) A++;
    B = strchr (A, '\n');
    if (B != (char *) NULL) { *B = 0; }
    if (*A != 0) {
      PushQueue (queue, A);
      A = B + 1;
      i++;
    }
  }      
  free (val);
    
  return (TRUE);

usage:
  gprint (GP_ERR, "USAGE: queueload (queue) -x (command)\n");
  return (FALSE);
}


/* 
 * -key only needed for replace or unique : give an error otherwise
 * -uniq searched for a match and does NOT replace if matched
 * -replace searches for a match and replaces if matched
 * should trigger an error if -uniq and -replace...
 */

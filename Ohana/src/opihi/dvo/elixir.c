# include "dvoshell.h"

int WriteMsg (char *fifo, char *message);
int ReadMsg (char *fifo, char **message);

# define MY_MAX_PATH 256

int elixir (int argc, char **argv) {
  
  char message[MY_MAX_PATH], cmd[MY_MAX_PATH], ElixirBase[MY_MAX_PATH];
  char fifo[MY_MAX_PATH], fifodir[MY_MAX_PATH], msgfile[MY_MAX_PATH];
  char *answer;
  int N;

  sprintf (cmd, "STATUS");
  if ((N = get_argument (argc, argv, "-time"))) {
    remove_argument (N, &argc, argv);
    sprintf (cmd, "TIMES");
  }
  if ((N = get_argument (argc, argv, "-live"))) {
    remove_argument (N, &argc, argv);
    sprintf (cmd, "ALIVE");
  }
  if ((N = get_argument (argc, argv, "-stop"))) {
    remove_argument (N, &argc, argv);
    sprintf (cmd, "STOP");
  }
  if ((N = get_argument (argc, argv, "-kill"))) {
    remove_argument (N, &argc, argv);
    sprintf (cmd, "ABORT");
  }
 
  if (argc != 2) {
    gprint (GP_ERR, "USAGE: elixir (elixir) [-time] [-live]\n");
    return (FALSE);
  }

  if (!VarConfig (argv[1], "%s", ElixirBase)) {
    gprint (GP_ERR, "elixir %s not in config file\n", argv[1]);
    return (FALSE);
  }
  snprintf_nowarn (fifo, MY_MAX_PATH, "%s.msg", ElixirBase);
  if (!VarConfig ("FIFOS", "%s", fifodir)) {
    gprint (GP_ERR, "FIFOS not in config, using local /tmp\n");
    strcpy (fifodir, "/tmp");
  }
  snprintf_nowarn (fifo, MY_MAX_PATH, "%s.msg", ElixirBase);

  snprintf_nowarn (msgfile, MY_MAX_PATH, "%s/EMsg.XXXXXX", fifodir);
  if (mkstemp (msgfile) == -1) {
    gprint (GP_ERR, "can't create fifo\n");
    return (FALSE);
  }
  snprintf_nowarn (message, MY_MAX_PATH, "%s %s", cmd, msgfile);
  unlink (msgfile);

  if (!WriteMsg (fifo, message)) {
    gprint (GP_ERR, "can't access fifo %s\n", fifo);
    return (FALSE);
  }

  if (ReadMsg (msgfile, &answer)) {
    gprint (GP_ERR, "%s\n", answer);
  } 
  unlink (msgfile);
  return (TRUE);
  
}

int WriteMsg (char *fifo, char *message) {

  int state, mode;
  FILE *f;

  /* check lockfile */
  f = fsetlockfile (fifo, 2.0, LCK_XCLD, &state);
  if (f == NULL) return (0);

  /* write message to end of file */
  fseeko (f, 0LL, SEEK_END);
  fprintf (f, "%s\n", message);

  mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
  chmod (fifo, mode);

  fclearlockfile (fifo, f, LCK_XCLD, &state);
  return (1);
}

int ReadMsg (char *fifo, char **message) {

  int i, nbytes, Nbytes, NBYTES;
  char *buffer;
  int state, mode;
  FILE *f;
  struct stat filestat;

  /* wait (2 sec) for file to exist, then try to read it */
  for (i = 0; (stat (fifo, &filestat) == -1) && (i < 20); i++) {
    usleep (100000);
  }
  if (i >= 20) {
    gprint (GP_ERR, "no response\n");
    return (0);
  }

  /* check lockfile */
  f = fsetlockfile (fifo, 2.0, LCK_XCLD, &state);
  if (f == NULL) {
    gprint (GP_ERR, "message locked (%d)\n", state);
    return (0);
  }

  /* if file is empty, return 0 */
  if (state == LCK_EMPTY) {
    mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
    chmod (fifo, mode);
    fclearlockfile (fifo, f, LCK_XCLD, &state);
    return (0);
  }  

  Nbytes = 0;
  NBYTES = 0x1000;
  ALLOCATE (buffer, char, NBYTES);
  while (TRUE) {
    nbytes = fread (&buffer[Nbytes], 1, 0x1000, f);
    if (nbytes < 0) { 
      gprint (GP_ERR, "error in ReadMsg -- got -1 bytes\n");
      return (0);
    }
    if (nbytes == 0) break;
    Nbytes += nbytes;
    NBYTES += 0x1000;
    REALLOCATE (buffer, char, NBYTES);
  }
  buffer[Nbytes] = 0;

  mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
  chmod (fifo, mode);
  fclearlockfile (fifo, f, LCK_XCLD, &state);

  if (Nbytes == 0) {
    free (buffer);
    return (0);
  }

  *message = buffer;
  return (1);

}

# include "elixir.h"

int WaitMsg (char *fifo, char **message, double maxdelay) {

  int status, Nsleep;
  struct timeval now, then;
  double dtime;
  struct stat filestats;

  /* maxdelay is the longest we will wait if the file is locked.  
     if the file doesn't exist, or is empty, skip it */
  /* limit our reads to only 10 tries, waiting for a little while in between */
  Nsleep = 100000 * maxdelay;

  status = stat (fifo, &filestats);
  if (status == -1) return (0);
  if (filestats.st_size == 0) return (0);

  gettimeofday (&then, (void *) NULL);
  while (TRUE) {
    status = ReadMsg (fifo, message);
    switch (status) {
    case 1:
      return (1);
    default:
      gettimeofday (&now, (void *) NULL);
      dtime = DTIME (now, then);
      if (dtime > maxdelay) return (0);
    }
    usleep (Nsleep);
  }
}

int ReadMsg (char *fifo, char **message) {

  int nbytes, Nbytes, NBYTES;
  char *buffer;
  int state, mode;
  FILE *f;

  /* check lockfile */
  f = fsetlockfile (fifo, 0.1, LCK_XCLD, &state);
  if (f == NULL) return (2);
  
  /* if file is empty, return 0 */
  if (state == LCK_EMPTY) {
    mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
    chmod (fifo, mode);
    fclearlockfile (fifo, f, LCK_XCLD, &state);
    return (0);
  }  
  
  /* read data from file */
  Nbytes = 0;
  NBYTES = 0x1000;
  ALLOCATE (buffer, char, NBYTES);
  while (TRUE) {
    nbytes = fread (&buffer[Nbytes], 1, 0x1000, f);
    if (nbytes < 0) { 
      fprintf (stderr, "error in ReadMsg -- got -1 bytes\n");
      exit (0);
    }
    if (nbytes == 0) break;
    Nbytes += nbytes;
    NBYTES += 0x1000;
    REALLOCATE (buffer, char, NBYTES);
  }
  buffer[Nbytes] = 0;
  
  /* file will remain until unlocked and fclosed below, 
     but cannot be written to because it is locked */
  if (truncate (fifo, 0)) {
    fprintf (stderr, "failed to clear fifo file %s (errno: %d)\n", fifo, errno);
    Shutdown (1);
  }
  
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

int WriteMsg (char *fifo, char *message) {

  int state, mode;
  FILE *f;

  /* check lockfile */
  f = fsetlockfile (fifo, 0.1, LCK_XCLD, &state);
  if (f == NULL) return (2);

  /* write message to end of file */
  fseeko (f, 0, SEEK_END);
  fprintf (f, "%s\n", message);

  mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
  chmod (fifo, mode);
  fclearlockfile (fifo, f, LCK_XCLD, &state);
  return (1);
}

/*  possible return states:
    0 - no message (fifo file empty or non-existent) 
    1 - message received
    2 - busy (fifo file locked)
*/

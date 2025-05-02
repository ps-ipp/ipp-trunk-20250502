# include "elixir.h"

int InitFifo (Fifo *fifo, int Nalloc, int Nextra) {

  if (Nextra >= Nalloc) {
    fprintf (stderr, "absurd fifo definition\n");
    return (FALSE);
  }

  fifo[0].Nalloc = Nalloc;
  fifo[0].Nextra = Nextra;
  fifo[0].Nmaxread = Nalloc - Nextra;
  fifo[0].Nlast = 0;
  fifo[0].Nbuffer = 0;

  ALLOCATE (fifo[0].buffer, char, fifo[0].Nalloc);

  return (TRUE);

}

int FlushFifo (Fifo *fifo) {

  fifo[0].Nlast = 0;
  fifo[0].Nbuffer = 0;

  return (TRUE);

}

/* after a shift, we can always read 
   fifo[0].Nmaxread 
   bytes into 
   &fifo[0].buffer[Nbuffer] 
   which is the byte after then end of existing data */

int ShiftFifo (Fifo *fifo) {

  int Nextra, Nshift;

  Nextra = fifo[0].Nextra;
  Nshift = fifo[0].Nbuffer - fifo[0].Nextra;
  if (Nshift <= 0) return (TRUE);

  memcpy (fifo[0].buffer, &fifo[0].buffer[Nshift], Nextra);
  fifo[0].Nbuffer = Nextra;
  fifo[0].Nlast = Nextra;

  return (TRUE);
}

/* like a standard read, ReadtoFifo returns Nbytes read,
   -1 for sock busy, or 0 for sock closed */

int ReadtoFifo (Fifo *fifo, int sock) {

  int Nread;
  int Nbuffer, Nmaxread;

  if (sock == 0) {
    fprintf (stderr, "error with socket?\n");
    return (0);
  }

  fifo[0].Nlast = fifo[0].Nbuffer;

  Nbuffer = fifo[0].Nbuffer;
  Nmaxread = fifo[0].Nmaxread;
  Nread = read (sock, &fifo[0].buffer[Nbuffer], Nmaxread);

  if (Nread > 0) fifo[0].Nbuffer += Nread;

  if (Nread == -1) {
    /* check for possible errors.  anything other than EAGAIN
       is bad and should indicate the connection is down */
    switch (errno) {
    case EAGAIN:
    case EIO:
      Nread = -1;
      break;
    default:
      fprintf (stderr, "read error: %d\n", errno);
      Nread = 0;
      break;
    }
  }

  return (Nread);
}

void FreeFifo (Fifo *fifo) {

  if (fifo[0].buffer != (char *) NULL) {
    free (fifo[0].buffer);
  }
}

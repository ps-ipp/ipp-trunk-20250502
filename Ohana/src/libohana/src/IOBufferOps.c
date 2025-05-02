# include <ohana.h>
# define DEBUG 0

int InitIOBuffer (IOBuffer *buffer, int Nalloc) {

  buffer[0].Nalloc = Nalloc;
  buffer[0].Nreset = Nalloc;
  buffer[0].Nblock = Nalloc / 2;
  buffer[0].Nbuffer = 0;

  ALLOCATE (buffer[0].buffer, char, buffer[0].Nalloc);
  bzero (buffer[0].buffer, buffer[0].Nalloc);

  return (TRUE);
}

int FlushIOBuffer (IOBuffer *buffer) {

  buffer[0].Nbuffer = 0;
  buffer[0].Nalloc = buffer[0].Nreset;
  REALLOCATE (buffer[0].buffer, char, buffer[0].Nalloc);
  bzero (buffer[0].buffer, buffer[0].Nalloc);

  return (TRUE);
}

int ReadtoIOBuffer (IOBuffer *buffer, int fd) {

  int Nread, Nfree, Nwant;

  if (fd == 0) {
    /* pipe is closed */
    return (0);
  }

  // if we run out of space, double Nblock
  Nfree = buffer[0].Nalloc - buffer[0].Nbuffer;
  if (Nfree < buffer[0].Nblock) {
    buffer[0].Nblock *= 2;
    buffer[0].Nblock = MIN (buffer[0].Nblock, 0x10000);
    buffer[0].Nalloc += 2*buffer[0].Nblock;
    REALLOCATE (buffer[0].buffer, char, buffer[0].Nalloc);
    Nfree = buffer[0].Nalloc - buffer[0].Nbuffer;
    bzero (buffer[0].buffer + buffer[0].Nbuffer, Nfree);
  }

  // ensure we never read more than space available
  Nwant = MIN (Nfree, buffer[0].Nblock);
  Nread = read (fd, &buffer[0].buffer[buffer[0].Nbuffer], Nwant);
  if (DEBUG) fprintf (stderr, "read IO buffer: (%lx) %d from %d\n", (unsigned long) buffer, Nread, Nwant); 

  /* on success, increase the block size for the next read */
  
  if (Nread >= 0) {
    buffer[0].Nbuffer += Nread;
    return (Nread);
  }

  if (Nread == -1) {
    switch (errno) {
    case EAGAIN:
    case EINTR:
      /** data not available in pipe or read interrupted : just try again **/
      return (-1);
    default:
      /** serious error (buffer overflow, invalid fd, etc **/
      perror ("ReadtoIOBuffer read error");
      return (-2);
    }
  }
  return (Nread);
}

/* read until buffer is empty (Nmax retries) */
int EmptyIOBuffer (IOBuffer *buffer, int Nmax, int fd) {

  int i, status;

  status = -1;
  for (i = 0; (status != 0) && (i < Nmax); i++) {
    status = ReadtoIOBuffer (buffer, fd);
    if (status == -1) usleep (10000);
    if (status > 0) i = 0;
  }
  if (status == -1) return (FALSE);
  return (TRUE);
}

void FreeIOBuffer (IOBuffer *buffer) {

  if (buffer[0].buffer != (char *) NULL) {
    free (buffer[0].buffer);
  }
}

/* print to an IOBuffer (varargs form) */
int PrintIOBuffer (IOBuffer *buffer, char *format, ...) {

  int status;
  va_list argp;  

  va_start (argp, format);
  status = vPrintIOBuffer (buffer, format, argp);
  va_end (argp);
  return (status);
}

/* print to an IOBuffer (va_list form) */
int vPrintIOBuffer (IOBuffer *buffer, char *format, va_list argp) {

  /* add the output line to the given IOBuffer */
  
  int Nbyte;
  char tmp;
  va_list argp2;

  va_copy (argp2, argp);

  Nbyte = vsnprintf (&tmp, 0, format, argp2);

  if (buffer[0].Nbuffer + Nbyte + 1>= buffer[0].Nalloc) {
    buffer[0].Nalloc = buffer[0].Nbuffer + Nbyte + 64;
    REALLOCATE (buffer[0].buffer, char, buffer[0].Nalloc);
  }

  vsnprintf (&buffer[0].buffer[buffer[0].Nbuffer], Nbyte + 1, format, argp);
  buffer[0].Nbuffer += Nbyte;
  return (TRUE);
}
  
/* write the bytes to the IOBuffer */
int WriteToIOBuffer (IOBuffer *buffer, char *input, int Ninput) {

  // extend the buffer if needed
  if (buffer[0].Nbuffer + Ninput + 1 >= buffer[0].Nalloc) {
    buffer[0].Nalloc = buffer[0].Nbuffer + Ninput + 64;
    REALLOCATE (buffer[0].buffer, char, buffer[0].Nalloc);
  }

  memcpy (&buffer[0].buffer[buffer[0].Nbuffer], input, Ninput);
  buffer[0].Nbuffer += Ninput;
  buffer[0].buffer[buffer[0].Nbuffer] = 0;

  return (TRUE);
}
  

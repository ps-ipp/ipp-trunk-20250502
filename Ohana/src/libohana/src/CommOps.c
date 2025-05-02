# include <ohana.h>

// XXX this is somewhat poor: the Send commands return TRUE / FALSE for success/failure
// the Expect commands return 0 for success, -N for different errors

int ExpectMessage (int device, double timeout, IOBuffer *message) {

  int status, length;
  IOBuffer command;

  status = ExpectCommand (device, 16, timeout, &command);
  if (status) {
    FreeIOBuffer (&command);
    return (status);
  }
  // fprintf (stderr, "resp: (%d) %s\n", command.Nbuffer, command.buffer);

  /* buffer contains an EOL NULL, we can just sscan it */
  sscanf (command.buffer, "%*s %d", &length);
  FreeIOBuffer (&command);
  
  status = ExpectCommand (device, length, timeout, message);

  return (status);
}

int ExpectCommand (int device, int length, double timeout, IOBuffer *buffer) {

  /* read from device until we have length bytes, or timeout */

  int Nread;
  double dtime;
  struct timespec request, remain;
  struct timeval start, stop;

  gettimeofday (&start, NULL);

  /* avoid blocking on waitpid, test every 1000 usec, up to timeout msec */
  request.tv_sec = 0;
  request.tv_nsec = 1000000;

  InitIOBuffer (buffer, length + 1);

  while (buffer[0].Nbuffer < length) {
    Nread = read (device, &buffer[0].buffer[buffer[0].Nbuffer], length - buffer[0].Nbuffer);
    
    if (Nread > 0) {
      // fprintf (stderr, "read %d of %d\n", Nread, length - buffer[0].Nbuffer);
      buffer[0].Nbuffer += Nread;
      continue;
    }

    if (Nread == -1) {
      // fprintf (stderr, "errno: %d\n", errno);
      // perror ("read error");
      switch (errno) {
	case EAGAIN:
	case EIO:
	  /** no data available in pipe, wait a bit, check for timeout **/
	  nanosleep (&request, &remain);
	  break;
	default:
	  /** error reading from pipe **/
	  perror ("ReadtoIOBuffer read error");
	  return (-2);
      }
    }

    if (Nread == 0) return (-3);

    gettimeofday (&stop, NULL);
    dtime = DTIME (stop, start);
    if (dtime > timeout) return (-1);
  }
  return (0);
}

/* send a message of arbitrary size, sending the size first */
int SendMessage (int device, char *format, ...) {

  int Nbyte;
  char tmp;
  va_list argp;  

  va_start (argp, format);
  Nbyte = vsnprintf (&tmp, 0, format, argp);
  va_end (argp);

  if (!Nbyte) return (FALSE);

  va_start (argp, format);
  if (!SendCommand (device, 16, "NBYTES: %6d", Nbyte)) goto escape;
  if (!SendCommandV (device, Nbyte, format, argp)) goto escape;
  va_end (argp);
  return TRUE;

escape:
  va_end (argp);
  return FALSE;
}

/* send a message of known size, sending the size first */
int SendMessageFixed (int device, int length, char *message) {

  int Nbytes, Nsent;
  struct timespec request, remain;

  // fprintf (stderr, "send fixed message, length = %d\n", length);

  if (!SendCommand (device, 16, "NBYTES: %6d", length)) return FALSE;

  /* avoid blocking on waitpid, test every 1000 usec, up to timeout msec */
  request.tv_sec = 0;
  request.tv_nsec = 1000000;

  Nsent = 0;
  while (Nsent < length) {
    Nbytes = write (device, &message[Nsent], length - Nsent);
    
    // fprintf (stderr, "sent %d of %d\n", Nbytes, length - Nsent);

    if (Nbytes > 0) {
      Nsent += Nbytes;
      continue;
    }

    if (Nbytes == -1) {
      // fprintf (stderr, "errno: %d\n", errno);
      // perror ("send error");
      switch (errno) {
	case EAGAIN:
	case EIO:
	  /** no data available in pipe, wait a bit, check for timeout **/
	  nanosleep (&request, &remain);
	  break;
	default:
	  /** error reading from pipe **/
	  perror ("SendMessageFixed write error");
	  return FALSE;
      }
    }
    if (Nbytes == 0) return FALSE;
  }
  // if (!SendCommand (device, length, message)) return FALSE;

  return TRUE;
}

int SendCommand (int device, int length, char *format, ...) {

  int status;
  va_list argp;  

  va_start (argp, format);
  status = SendCommandV (device, length, format, argp);
  va_end (argp);
  return (status);
}
  
int SendCommandV (int device, int length, char *format, va_list argp) {

  int status;
  char *string;

  /* I allocated and zero 1 extra byte */
  ALLOCATE (string, char, length + 1);
  memset (string, 0, length + 1);
  vsnprintf (string, length + 1, format, argp);

  // fprintf (stderr, "write command, length = %d\n", length);

  status = write (device, string, length);
  free (string);

  if (status == -1) return FALSE;
  return (TRUE);
}

/* 

 I need an alternative ExpectCommand function which appends to an existing buffer
 until the complete message is ready.  

 A command looks like this (pre-determined length):
 NN bytes: XXXXX\0 

 A message looks like this (preceded by NBYTES command) :
 16 bytes: WORD NNN\0
 NNN bytes: message.... \0

 the expect function needs to monitor the number of bytes already received
 we need to be able to call it repeatedly until the buffer is full.

*/

/* check if the first entry in the buffer corresponds to a message:
   A message looks like this (preceded by NBYTES command) :
    16 bytes: WORD NNN
    NNN bytes: message....
    note that the NULL bytes are not sent
  If a message is found, it is popped off the buffer and sent back as a
  complete line (the length portion is dropped) 
*/

char *CheckForMessage (IOBuffer *buffer) {

  int Nbytes;
  char command[20], *line;

  if (buffer[0].Nbuffer < 16) return NULL;
  memcpy (command, buffer[0].buffer, 16); // SAFE
  command[16] = 0;

  sscanf (command, "NBYTES: %d", &Nbytes);

  if (buffer[0].Nbuffer < Nbytes + 16) return NULL;

  ALLOCATE (line, char, Nbytes + 1);
  memcpy (line, &buffer[0].buffer[16], Nbytes); // SAFE
  line[Nbytes] = 0;

  buffer[0].Nbuffer -= Nbytes + 16;
  memmove (buffer[0].buffer, &buffer[0].buffer[Nbytes+16], buffer[0].Nbuffer);
  return (line);
}

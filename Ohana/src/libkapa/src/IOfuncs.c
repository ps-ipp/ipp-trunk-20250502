# include <kapa_internal.h>
# define DEBUG 0

/** these function expect to operate with a BLOCKing socket **/

/* why is this not defined in stdarg.h for linux/x64? */
int vsscanf(const char *str, const char *format, va_list ap);

int KiiSendData (int device, char *data, int Nbytes) {

  int Nwrite;

  KiiSendCommand (device, 16, "LEN: %11d", Nbytes);
  Nwrite = write (device, data, Nbytes);
  if (Nwrite != Nbytes) return (FALSE);
  return (TRUE);
} 

char *KiiRecvData (int device) {

  int status, Nbytes;
  char *data, buffer[20];

  /* read 16 bytes: LEN (length) */
  status = read (device, buffer, 16);
  if (status != 16) return NULL;
  buffer[16] = 0;

  /* find the message length, allocate space */
  sscanf (buffer, "%*s %d", &Nbytes);
  ALLOCATE (data, char, Nbytes + 1);
  memset (data, 0, Nbytes + 1);

  status = read (device, data, Nbytes);
  if (status != Nbytes) {
    free (data);
    return NULL;
  }

  data[Nbytes] = 0;

  return (data);
} 

/* send a message of arbitrary size, sending the size first */
int KiiSendMessage (int device, char *format, ...) {

  int Nbyte, status;
  char tmp;
  va_list argp;  

  va_start (argp, format);
  Nbyte = vsnprintf (&tmp, 0, format, argp);
  va_end (argp);

  if (!Nbyte) {
    KiiSendCommand (device, 16, "LEN: %11d", 0);
    return (FALSE);
  }

  /* the message may contain up to 99,999,999,999 bytes (100MB) */
  va_start (argp, format);
  KiiSendCommand (device, 16, "LEN: %11d", Nbyte);
  status = KiiSendCommandV (device, Nbyte, format, argp);
  va_end (argp);

  return (status);
}

/* scan a message of arbitrary size, accepting the size first */
int KiiScanMessage (int device, char *format, ...) {

  int Nbytes, status;
  char buffer[20], *message;
  va_list argp;  

  /* read 16 bytes: LEN (length) */
  status = read (device, buffer, 16);
  buffer[16] = 0;
  if (status != 16) {
      fprintf (stderr, "dropped message length\n");
      // XXX if this happens, I need to give up and return FALSE
      // XXX BUT: I need to think a bit more about hand-shaking.
  }
  if (DEBUG) fprintf (stderr, "recv buffer: %s...\n", buffer);

  /* find the message length, allocate space */
  sscanf (buffer, "%*s %d", &Nbytes);
  if (Nbytes == 0) {
    return TRUE;
  }

  ALLOCATE (message, char, Nbytes + 1);
  memset (message, 0, Nbytes + 1);


  /* read Nbytes from the device */
  status = read (device, message, Nbytes);
  if (status != Nbytes) {
      fprintf (stderr, "Kii/Kapa comm error\n");
  }
  message[status] = 0;
  /* make the string easy to parse */

  if (DEBUG) fprintf (stderr, "recv: %s...\n", message);

  /* scan the incoming message */
  va_start (argp, format);
  Nbytes = vsscanf (message, format, argp);
  va_end (argp);

  free (message);

  return (status);
}

/* send a command of fixed size */
int KiiSendCommand (int device, int length, char *format, ...) {

  int status;
  va_list argp;  

  va_start (argp, format);
  status = KiiSendCommandV (device, length, format, argp);
  va_end (argp);

  return (status);
}
  
int KiiSendCommandV (int device, int length, char *format, va_list argp) {

  int Nwrite;
  char *string;

  /* string is sent WITHOUT ending NULL char */
  /* allocate and zero length + 1 extra byte */
  ALLOCATE (string, char, length + 1);
  memset (string, 0, length + 1);
  vsnprintf (string, length + 1, format, argp);

  Nwrite = write (device, string, length);
  if (Nwrite != length) return (FALSE);

  if (DEBUG) fprintf (stderr, "send: %s...\n", string);

  free (string);
  return (TRUE);
}

/* scan a command of fixed size */
int KiiScanCommand (int device, int length, char *format, ...) {

  int status;
  char *message;
  va_list argp;  

  ALLOCATE (message, char, length + 1);
  memset (message, 0, length + 1);

  /* read Nbytes from the device */
  status = read (device, message, length);
  if (DEBUG) fprintf (stderr, "recv message: %s...\n", message);

  if (status != length) {
      fprintf (stderr, "Kii/Kapa comm error\n");
      return (0);
  }
  message[status] = 0; // make the string easy to parse

  /* scan the incoming message */
  va_start (argp, format);
  vsscanf (message, format, argp);
  va_end (argp);

  free (message);

  return (1);
}

int KiiWaitAnswer (int device, char *expect) {

  int Nbytes;
  char *answer;

  Nbytes = strlen (expect);
  ALLOCATE (answer, char, Nbytes + 1);
  memset (answer, 0, Nbytes + 1);

  KiiScanCommand (device, Nbytes, "%s", answer);
  if (strcmp (answer, expect)) {
      fprintf (stderr, "unexpected response %s, expected %s\n", answer, expect); 
      REALLOCATE (answer, char, 128);
      Nbytes = read (device, answer, 127);
      answer[Nbytes] = 0;
      fprintf (stderr, "extra data in buffer: %d bytes\n", Nbytes);
      fprintf (stderr, "garbage: %s\n", answer);
      free (answer);
      return (FALSE);
  }
  free (answer);
  return (TRUE);
}

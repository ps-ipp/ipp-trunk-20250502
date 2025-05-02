# include "dvodist.h"
# define PCLIENT_TIMEOUT 1000
# define DEBUG 0

// send a command and check for errors; ignore output
int PclientCommand (HostInfo *host, char *command, IOBuffer *buffer) {

  int status;

  // flush the stdout and stderr buffers here
  ReadtoIOBuffer (buffer, host->stdio[1]);
  FlushIOBuffer (buffer);
  ReadtoIOBuffer (buffer, host->stdio[2]);
  FlushIOBuffer (buffer);

  /* send command to client (adding on \n) */
  status = write_fmt (host->stdio[0], "%s\n", command);

  /* is pipe still open? */
  if ((status == -1) && (errno == EPIPE)) {
    fprintf (stderr, "pclient read gives pipe error for %s\n", command);
    return FALSE;
  }

  FlushIOBuffer (buffer);

  return TRUE;
}
  
// check for response; message must end with specified string.
// accumulate the response in the buffer
int PclientResponse (HostInfo *host, char *response, IOBuffer *buffer) {

  int i;
  int status;
  char *line;
  struct timespec request, remain;

  /* avoid blocking very long on read, test every 100 usec, up to 0.1 sec */
  request.tv_sec = 0;
  request.tv_nsec = 100000;

  /* watch for response - wait up to 1 second */
  line = NULL;
  status = -1;

  // how long does each cycle really take?
  for (i = 0; (i < PCLIENT_TIMEOUT) && (status != 0) && (line == NULL); i++) {
    status = ReadtoIOBuffer (buffer, host->stdio[1]);
    line = memstr (buffer->buffer, response, buffer->Nbuffer);
    if (status == -1) nanosleep (&request, &remain);
  }
  if (status ==  0) {
    fprintf (stderr, "pclient read returns 0 for %s\n", response);
    return FALSE;
  }
  if (!line) {
      fprintf (stderr, "client hung\n");
      return FALSE;
  }
  if (status == -1) {
      fprintf (stderr, "client hung\n");
      return FALSE;
  }

  // fprintf (stderr, "response: %s\n", buffer->buffer);

  return TRUE;
}

int GetJobOutput (char *command, HostInfo *host, IOBuffer *output, int size) {
  
    char *line;
    int i, status;
    struct timespec request, remain;

    /* avoid blocking on waitpid, test every 100 usec, up to 10 msec */
    request.tv_sec = 0;
    request.tv_nsec = 100000;

    status = write_fmt (host->stdio[0], "%s\n", command);

    if ((status == -1) && (errno == EPIPE)) {
	fprintf (stderr, "host crashed?\n");
	goto escape;
    }

    // attempt to read the output->size bytes from the host 
    for (i = 0; (i < PCLIENT_TIMEOUT) && (status != 0) && (output->Nbuffer < size); i++) {
	status = ReadtoIOBuffer (output, host->stdio[1]);
	if (status == -1) nanosleep (&request, &remain);
    }
    if (DEBUG) fprintf (stderr, "Read %d of %d bytes so far\n", output->Nbuffer, size);
    if (status == 0) {
	fprintf (stderr, "client down\n");
	goto escape;
    }
    if (output->Nbuffer < size) {
	fprintf (stderr, "client hung\n");
	goto escape;
    }

    // keep trying to read until we get the prompt
    line = NULL;
    status = -1;
    for (i = 0; (i < PCLIENT_TIMEOUT) && (status != 0) && (line == NULL); i++) {
	status = ReadtoIOBuffer (output, host->stdio[1]);
	line = memstr (output->buffer, PCLIENT_PROMPT, output->Nbuffer);
	if (status == -1) nanosleep (&request, &remain);
    }
    if (DEBUG) fprintf (stderr, "Read %d of %d bytes so far\n", output->Nbuffer, size);
    if (!status) {
	fprintf (stderr, "pclient down?\n");	      // return PCLIENT_DOWN;
	goto escape;
    }
    if (!line) {
	fprintf (stderr, "pclient hung?\n");		      // return PCLIENT_HUNG;
	goto escape;
    }
    return TRUE;

escape:
    return FALSE;
}

int CheckBusyJob (HostInfo *host, IOBuffer *stdout_buf, IOBuffer *stderr_buf) {

  char *p;
  char string[64];
  IOBuffer buffer;

  InitIOBuffer (&buffer, 100);
  PclientCommand (host, "status", &buffer);
  PclientResponse (host, PCLIENT_PROMPT, &buffer);

  /** need to parse message **/
  p = memstr (buffer.buffer, "STATUS", buffer.Nbuffer);
  if (p == NULL) {
    if (DEBUG) fprintf (stderr, "missing STATUS in response; try again?\n");
    FreeIOBuffer (&buffer);
    return FALSE;
  }

  sscanf (p, "%*s %s", string);
  if (!strcmp(string, "NONE")) {
    if (DEBUG) fprintf (stderr, "not started?\n");
    FreeIOBuffer (&buffer);
    return FALSE;
  }
  
  /** no status change, return to BUSY stack **/
  if (!strcmp(string, "BUSY")) {
    if (DEBUG) fprintf (stderr, "not yet done...\n");
    FreeIOBuffer (&buffer);
    return FALSE;
  }
  
  /* exit status better be either EXIT or CRASH */
  if (!strcmp(string, "CRASH")) {
    fprintf (stderr, "crash on host...\n");
    FreeIOBuffer (&buffer);
    return FALSE;
  }

  if (strcmp(string, "EXIT")) {
    fprintf (stderr, "error in status string?\n");
    FreeIOBuffer (&buffer);
    return FALSE;
  }

  int exit_status, stdout_buf_size, stderr_buf_size;

  /* parse the exit status and sizes of output buffers */
  p = memstr (buffer.buffer, "EXITST", buffer.Nbuffer);
  sscanf (p, "%*s %d", &exit_status);
  p = memstr (buffer.buffer, "STDOUT", buffer.Nbuffer);
  sscanf (p, "%*s %d", &stdout_buf_size);
  p = memstr (buffer.buffer, "STDERR", buffer.Nbuffer);
  sscanf (p, "%*s %d", &stderr_buf_size);

  // XXX runaway job if output too large?
  if (stdout_buf_size > 0x1000000) abort();
  if (stderr_buf_size > 0x1000000) abort();

  if (stdout_buf_size) {
    GetJobOutput ("stdout", host, stdout_buf, stdout_buf_size);
  } else {
    ReadtoIOBuffer (&buffer, host->stdio[1]);
  }
  if (stderr_buf_size) {
    GetJobOutput ("stderr", host, stderr_buf, stderr_buf_size);
  } else {
    ReadtoIOBuffer (&buffer, host->stdio[2]);
  } 

  FreeIOBuffer (&buffer);
  return TRUE;
}

/* memstr returns a view, not an allocated string : don't free */
/* ReadtoIOBuffer returns : 
    0 - pipe closed
   -1 - no more data in pipe, data not ready
   -2 - serious error reading from pipe
   >0 - data read from pipe
*/


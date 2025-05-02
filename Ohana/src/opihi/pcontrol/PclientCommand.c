# include "pcontrol.h"
# define PCLIENT_TIMEOUT 100

// send a command and check for errors; ignore output
int PclientCommand (Host *host, char *command, char *response, HostResp response_state) {

  int status;

  ASSERT (host != NULL, "host missing");
  ASSERT (command != NULL, "command missing");

  // flush the stdout and stderr buffers here
  // recycle comms_buffer to minimize page thrashing
  ReadtoIOBuffer (&host[0].comms_buffer, host[0].stdout_fd);
  FlushIOBuffer (&host[0].comms_buffer);
  ReadtoIOBuffer (&host[0].comms_buffer, host[0].stderr_fd);
  FlushIOBuffer (&host[0].comms_buffer);

  /* send command to client (adding on \n) */
  status = write_fmt (host[0].stdin_fd, "%s\n", command);

  /* is pipe still open? */
  if ((status == -1) && (errno == EPIPE)) {
    gprint (GP_ERR, "pclient read gives pipe error for %s\n", command);
    return (PCLIENT_DOWN);
  }

  // prepare host to accept response
  host[0].response_state = response_state;
  host[0].response = response;
  FlushIOBuffer (&host[0].comms_buffer);

  // fprintf (stderr, "command: %s\n", command);

  return (PCLIENT_GOOD);
}

// check for response; message must end with specified string.
// accumulate the response in the buffer
int PclientResponse (Host *host, char *response, IOBuffer *buffer) {

  int i;
  int status;
  char *line;
  struct timespec request, remain;

  ASSERT (response != NULL, "response missing");
  ASSERT (buffer != NULL, "buffer missing");

  // INITTIME;

  /* avoid blocking very long on read, test every 100 usec, up to 0.1 sec */
  request.tv_sec = 0;
  request.tv_nsec = 100000;

  /* watch for response - wait up to 1 second */
  line = NULL;
  status = -1;

  // how long does each cycle really take?
  for (i = 0; (i < PCLIENT_TIMEOUT) && (status != 0) && (line == NULL); i++) {
    status = ReadtoIOBuffer (buffer, host[0].stdout_fd);
    line = memstr (buffer[0].buffer, response, buffer[0].Nbuffer);
    if (status == -1) nanosleep (&request, &remain);
  }
  if (status ==  0) {
    gprint (GP_ERR, "pclient read returns 0 for %s\n", response);
    return (PCLIENT_DOWN);
  }
  if (line == NULL) {
      // MARKTIME ("-- client hung (line NULL): %s : %f sec\n", host[0].hostname, dtime);
      return (PCLIENT_HUNG);
  }
  if (status == -1) {
      // MARKTIME ("-- client hung (status -1): %s : %f sec\n", host[0].hostname, dtime);
      return (PCLIENT_HUNG);
  }

  // fprintf (stderr, "response: %s\n", buffer[0].buffer);

  return (PCLIENT_GOOD);
}

/* memstr returns a view, not an allocated string : don't free */
/* ReadtoIOBuffer returns :
    0 - pipe closed
   -1 - no more data in pipe, data not ready
   -2 - serious error reading from pipe
   >0 - data read from pipe
*/

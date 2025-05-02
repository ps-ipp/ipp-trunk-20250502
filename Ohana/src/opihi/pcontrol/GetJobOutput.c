# include "pcontrol.h"
# define PCLIENT_TIMEOUT 100
# define DEBUG 0

// we are trying to read a total of Nbytes from the host.  This function may be called
// repeatedly until the buffer has the complete set of data.  We need to read output[0].size
// bytes, then look for the PCLIENT_PROMPT in the output stream

int GetJobOutput (char *command, Host *host, JobOutput *output) {
  
  char *line;
  int i, status;
  struct timespec request, remain;

  ASSERT (command, "command missing");
  ASSERT (host, "host missing");
  ASSERT (output, "output missing");

  /* avoid blocking on waitpid, test every 100 usec, up to 10 msec */
  request.tv_sec = 0;
  request.tv_nsec = 100000;

  if (!output[0].requested) {
      /* send command (stdout / stderr) */
      status = write_fmt (host[0].stdin_fd, "%s\n", command);

      /* is pipe still open? */
      if ((status == -1) && (errno == EPIPE)) return PCLIENT_DOWN;
      output[0].requested = TRUE;
  }

  if (output[0].completed) return PCLIENT_GOOD;

  // attempt to read the output->size bytes from the host 
  if (output[0].buffer.Nbuffer < output[0].size) {
      status = -1;
      for (i = 0; (i < PCLIENT_TIMEOUT) && (status != 0) && (output[0].buffer.Nbuffer < output[0].size); i++) {
	  status = ReadtoIOBuffer (&output[0].buffer, host[0].stdout_fd);
	  if (status == -1) nanosleep (&request, &remain);
      }
      if (VerboseMode()) gprint (GP_ERR, "%s\n Read %d of %d bytes so far\n", output[0].buffer.buffer, output[0].buffer.Nbuffer, output[0].size);
      if (status == 0) {
	  if (VerboseMode()) gprint (GP_ERR, "host %s is down\n", host[0].hostname);
	  return PCLIENT_DOWN;
      }
      if (output[0].buffer.Nbuffer < output[0].size) {
	  if (VerboseMode()) gprint (GP_ERR, "host %s still has data, keep trying\n", host[0].hostname);
	  return PCLIENT_HUNG;
      }
  }

  // keep trying to read until we get the prompt
  line = NULL;
  status = -1;
  for (i = 0; (i < PCLIENT_TIMEOUT) && (status != 0) && (line == NULL); i++) {
    status = ReadtoIOBuffer (&output[0].buffer, host[0].stdout_fd);
    line = memstr (output[0].buffer.buffer, PCLIENT_PROMPT, output[0].buffer.Nbuffer);
    if (status == -1) nanosleep (&request, &remain);
  }
  if (VerboseMode()) gprint (GP_ERR, "%s\n Read %d of %d bytes so far\n", output[0].buffer.buffer, output[0].buffer.Nbuffer, output[0].size);
  if (status == 0) {
    if (VerboseMode()) gprint (GP_ERR, "host %s is down\n", host[0].hostname);
    return PCLIENT_DOWN;
  }
  if (line == NULL) {
    if (VerboseMode()) gprint (GP_ERR, "host %s not yet at prompt, keep trying\n", host[0].hostname);
    return PCLIENT_HUNG;
  }

  output[0].completed = TRUE;
  return PCLIENT_GOOD;
}

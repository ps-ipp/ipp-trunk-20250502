# include "dvodist.h"

// these are md5 operations that could probably be moved to libohana

# define NBUFFER 512

// read a file, get the md5 sum, optionally write to 'output'
int get_md5_with_copy (char *input, char *output, md5_byte_t *digest) {

  // open the src file
  FILE *fInput = fopen (input, "r");
  int fdInput = fileno (fInput);
	
  // open the tgt file
  FILE *fOutput = NULL;
  int fdOutput = 0;
  if (output) {
    fOutput = fopen (output, "w");
    fdOutput = fileno (fOutput);
  }

  int nbytes;
  md5_state_t state;
  md5_byte_t buffer[NBUFFER];

  md5_init (&state);
  while ((nbytes = read (fdInput, buffer, NBUFFER)) > 0) {
    md5_append (&state, buffer, nbytes);
    if (output) {
      int nbytesOut = write (fdOutput, buffer, nbytes);
      if (nbytesOut != nbytes) {
	return FALSE;
      }
    }
  }
  md5_finish (&state, digest);

# if (0)
  int i;
  for (i = 0; i < NDIGEST; i++) {
    fprintf (stderr, "%02x", digest[i]);
  }
  fprintf (stderr, " : %s\n", input);
# endif
	
  fclose (fInput);
  fflush (fInput);

  if (output) {
    fclose (fOutput);
    fflush (fOutput);
  }
  return TRUE;
}

// XXX this would be better controlled if we used a remote pclient.  then we could
// check things like the exit status and 
int get_md5_from_remote (HostInfo *host, char *filename, md5_byte_t *digest) {

  IOBuffer buffer;

  char line[DVO_MAX_PATH];
  int Nline = snprintf (line, DVO_MAX_PATH, "md5sum %s\n", filename);
  assert (Nline < DVO_MAX_PATH);

  int Nout = write (host->stdio[0], line, Nline);
  if (Nout != Nline) {
    fprintf (stderr, "failed to execute md5sum\n");
    return FALSE;
  } 

  InitIOBuffer (&buffer, 100);
  int status = EmptyIOBuffer (&buffer, 10000, host->stdio[1]);
  fprintf (stderr, "md5sum status: %d\n", status);
      
  fprintf (stderr, "buffer: %s\n", buffer.buffer);

  char result[DVO_MAX_PATH], fileout[DVO_MAX_PATH];
  int Nscan = sscanf (buffer.buffer, "%s %s", result, fileout);
  assert (Nscan == 2);
  assert (strlen(result) == NDIGEST);
  
  memcpy (digest, result, NDIGEST);
  return TRUE;
}

// pclient version of remote client
int get_md5_from_pclient (HostInfo *host, char *filename, md5_byte_t *digest) {

  IOBuffer buffer, stdout_buf, stderr_buf;

  char line[DVO_MAX_PATH];
  int Nline = snprintf (line, DVO_MAX_PATH, "job md5sum %s\n", filename);
  assert (Nline < DVO_MAX_PATH);

  // fprintf (stderr, "command: %s\n", line);

  InitIOBuffer (&buffer, 100);
  InitIOBuffer (&stdout_buf, 100);
  InitIOBuffer (&stderr_buf, 100);

  // need to reset pclient for next command...
  if (!PclientCommand (host, "reset", &buffer)) goto escape;
  if (!PclientResponse (host, PCLIENT_PROMPT, &buffer)) goto escape;
  FlushIOBuffer (&buffer);

  // start the md5sum command
  if (!PclientCommand (host, line, &buffer)) goto escape;
  if (!PclientResponse (host, PCLIENT_PROMPT, &buffer)) goto escape;
  // XXX for the moment, abort if anything goes wrong

  // wait for result
  while (!CheckBusyJob (host, &stdout_buf, &stderr_buf)) {
    FlushIOBuffer (&stdout_buf);
    FlushIOBuffer (&stdout_buf);
    usleep (10000);
  }
  if (stderr_buf.Nbuffer) {
    fprintf (stderr, "error in md5sum: %s\n", stderr_buf.buffer);
    goto escape;
  }

  // fprintf (stderr, "result from md5sum: %s\n", stdout_buf.buffer);

  char result[DVO_MAX_PATH], fileout[DVO_MAX_PATH];
  int Nscan = sscanf (stdout_buf.buffer, "%s %s", result, fileout);
  if (Nscan != 2) {
    fprintf (stderr, "error reading md5sum: %s\n", stdout_buf.buffer);
    goto escape;
  }
  if (strlen(result) != 2*NDIGEST) {
    fprintf (stderr, "error reading md5sum: %s\n", stdout_buf.buffer);
    goto escape;
  }
  
  // result is the set of 0-f nibbles, convert to the digest
  int i;
  for (i = 0; i < NDIGEST; i++) {
    int value1 = hexchar_to_int (result[2*i+0]);
    int value2 = hexchar_to_int (result[2*i+1]);
    digest[i] = (value1 << 4) + value2;
    // fprintf (stderr, "%02x %c %c\n", digest[i], result[2*i], result[2*i+1]);
  }
  
  // need to reset pclient for next command...
  if (!PclientCommand (host, "reset", &buffer)) goto escape;
  if (!PclientResponse (host, PCLIENT_PROMPT, &buffer)) goto escape;

  return TRUE;

escape:
  FreeIOBuffer (&buffer);
  FreeIOBuffer (&stdout_buf);
  FreeIOBuffer (&stderr_buf);
  return FALSE;
}

int hexchar_to_int (char input) {

  switch (input) {
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
      return input - '0';
    case 'a':
    case 'b':
    case 'c':
    case 'd':
    case 'e':
    case 'f':
      return input - 'a' + 10;
    case 'A':
    case 'B':
    case 'C':
    case 'D':
    case 'E':
    case 'F':
      return input - 'A' + 10;
    default:
      abort();
  }
}

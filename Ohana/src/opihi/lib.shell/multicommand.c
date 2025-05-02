# include "opihi.h"

static int server = 0;
static int bufferPending = FALSE;

#define MSG_TIMEOUT 3.0

int getServer () {
  return (server);
}

// XXX this is rather pantasks-specific...
void multicommand_InitServer () {

  char hostname[256], PASSWORD[256], portinfo[256];

  if (server != 0) {
    /* check if down? */
    fprintf (stderr, "error: server fd already defined\n");
    exit (30);
  }

  /* find the defined server hostname */
  if (VarConfig ("PANTASKS_SERVER", "%s", hostname) == NULL) {
    gprint (GP_ERR, "pantasks server host undefined\n");
    exit (31);
  }

  /* is a port range defined? otherwise use the default */
  memset (portinfo, 0, 256);
  VarConfig ("PANTASKS_SERVER_PORT", "%s", portinfo);

  /* attempt to connect to the server */
  server = GetClientSocket (hostname, portinfo);

  /* here we can perform the security handshaking */
  VarConfig ("PASSWORD", "%s", PASSWORD);
  SendCommand (server, strlen(PASSWORD), "%s", PASSWORD);
  
  return;
}

// close connection with remote server
void multicommand_StopServer () {

  if (!server) return;
  close (server);
  server = 0;
  return;
}

/* take input line and split into multiple lines
   at the semi-colons.  send these to 'command' */

int multicommand (char *line) {
 
  int done, status, verbose;
  char *p, *q, *tmpline, *outline;
  IOBuffer message;

  /* if no server is defined, use 'verbose' mode for 'command' */ 
  verbose = (server == 0);

  p = line; 
  int Nline = strlen(line);
  
  done = FALSE;
  status = TRUE;
  interrupt = FALSE;
  while (!done) {
    q = strchr (p, ';');
    if (q == NULL) {
      q = p + strlen(p);
      done = TRUE;
    }
    tmpline = strncreate (p, q - p);
    stripwhite (tmpline);
    myAssert (tmpline, "oops");

    // empty command, free and continue
    if (*tmpline == 0) {
      free (tmpline);
      if (q == line + Nline) {
	done = TRUE;
      } else {
	p = q + 1;
	myAssert (p - line <= Nline, "oops");
      }
      continue;
    }

    if (bufferPending) {
      // flush old messages
      ExpectMessage (server, 0.2, &message);
      bufferPending = FALSE;
    }

    // input tmpline is freed by command
    status = command (tmpline, &outline, verbose);

    if (status == -1) {
      if (server) {
	// send the command to the server instead
	if (!SendMessage (server, "%s", outline)) {
	  switch (errno) {
	    case EPIPE:
	      gprint (GP_ERR, "server connection has died\n");
	      exit (32);
	    default:
	      gprint (GP_ERR, "server is busy...32\n");
	      bufferPending = TRUE;
	      goto escape;
	  }
	}

	// receive the command exit status
	if (ExpectMessage (server, MSG_TIMEOUT, &message)) {
	  switch (errno) {
	    case EPIPE:
	      gprint (GP_ERR, "server connection has died\n");
	      exit (33);
	    default:
	      gprint (GP_ERR, "server is busy...33\n");
	      bufferPending = TRUE;
	      goto escape;
	  }
	} else {
	  sscanf (message.buffer, "STATUS %d", &status);
	}

	// receive the resulting stderr
	if (ExpectMessage (server, MSG_TIMEOUT, &message)) {
	  switch (errno) {
	    case EPIPE:
	      gprint (GP_ERR, "server connection has died\n");
	      exit (34);
	    default:
	      gprint (GP_ERR, "server is busy...34\n");
	      bufferPending = TRUE;
	      goto escape;
	  }
	} else {
	  gwrite (message.buffer, 1, message.Nbuffer, GP_ERR);
	}

	// receive the resulting stdout
	if (ExpectMessage (server, MSG_TIMEOUT, &message)) {
	  switch (errno) {
	    case EPIPE:
	      gprint (GP_ERR, "server connection has died\n");
	      exit (35);
	    default:
	      gprint (GP_ERR, "server is busy...35\n");
	      bufferPending = TRUE;
	      goto escape;
	  }
	} else {
	  gwrite (message.buffer, 1, message.Nbuffer, GP_LOG);
	}
      } else {
	// if no server is defined, we treat an unknown command as an error
	status = FALSE;
      }	
    }
  escape:
    if (outline != NULL) free (outline);
    if (!status && auto_break) done = TRUE;

    p = q + 1;
  }
  return (status);
}

/* should skip ; surrounded by "" */

/* commands which are not found are sent to the server socket if 
   defined. commands executed by the server return their stderr
   and stdout streams.  server commands should probably not block
   the server for very long or multiple clients will interfere
   with each other.  
*/

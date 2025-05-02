# include "pantasks.h"
# define DEBUG 0

// XXX make the calling functions thread-safe
static int NCLIENTS;
static int Nclients;
static int *clients;
static IOBuffer **buffers;

static int ClientThreadRuns = TRUE;

void QuitClientThread (void) {
  ClientThreadRuns = FALSE;
}

void InitClients () {

  ClientLock();
  Nclients = 0;
  NCLIENTS = 10;
  ALLOCATE (clients, int, NCLIENTS);
  ALLOCATE (buffers, IOBuffer *, NCLIENTS);
  ClientUnlock();
}

/* add this client to the client table, create an IOBuffer for it */
void AddNewClient (int client) {

  ClientLock();
  if (DEBUG) fprintf (stderr, "adding a new client (%d)\n", client);
  clients[Nclients] = client;
  ALLOCATE (buffers[Nclients], IOBuffer, 1);
  InitIOBuffer(buffers[Nclients], 256);
  Nclients ++;
  if (Nclients >= NCLIENTS - 1) {
    NCLIENTS += 10;
    REALLOCATE (clients, int, NCLIENTS);
    REALLOCATE (buffers, IOBuffer *, NCLIENTS);
  }
  ClientUnlock();
}

/* add this client to the client table, create an IOBuffer for it */
int DeleteClient (int client) {

  int i, j;

  ClientLock();
  if (DEBUG) fprintf (stderr, "deleting a client (%d)\n", client);
  for (i = 0; i < Nclients; i++) {
    if (clients[i] == client) {
      FreeIOBuffer (buffers[i]);
      free (buffers[i]);
      close (clients[i]);
      for (j = i; j < Nclients - 1; j++) {
	clients[j] = clients[j+1];
	buffers[j] = buffers[j+1];
      }
      Nclients --;
      if ((Nclients > 10) && (Nclients / 2 < NCLIENTS)) {
	NCLIENTS = Nclients + 10;
	REALLOCATE (clients, int, NCLIENTS);
	REALLOCATE (buffers, IOBuffer *, NCLIENTS);
      }
      ClientUnlock();
      return TRUE;
    }
  }
  // did not find the client
  ClientUnlock();
  return FALSE;
}

/* select for messages from the current clients; wait for 0.5s before updating client list */
void *ListenClients (void *data) {
  OHANA_UNUSED_PARAM(data);
  
  int i, Ncurrent, Nmax, status, Nread;
  char *line;
  fd_set fdSet;
  struct timeval timeout;
  IOBuffer *outbuffer;

  InitClients ();
  gprintInit ();  // each thread needs to init the printing system

  /* set buffers for the output for this client */
  gprintSetBuffer (GP_LOG);
  gprintSetBuffer (GP_ERR);

  while (ClientThreadRuns) {

    /* Wait up to 0.5 second - need to timeout to update client list */
    /* timeout gets mucked: need to reset before each select */
    timeout.tv_sec = 0;
    timeout.tv_usec = 500000;

    /* place all of the clients in the fdSet */
    Nmax = 0;
    FD_ZERO (&fdSet);
    ClientLock();
    Ncurrent = Nclients;
    ClientUnlock();
    for (i = 0; i < Ncurrent; i++) {
      Nmax = MAX (Nmax, clients[i]);
      FD_SET (clients[i], &fdSet);
    }    
    Nmax ++;

    /* block until we have some data on the pipes (or timeout) */
    if (DEBUG) fprintf (stderr, "listening to %d clients\n", Ncurrent);
    status = select (Nmax, &fdSet, NULL, NULL, &timeout);
    if (status == -1) {
      perror("select()");
      return (FALSE);
    }

    /* if no data, update client list, wait for another select */
    if (status <= 0) continue;

    /* loop over the clients with data */
    for (i = 0; i < Ncurrent; i++) {
      /* if client has no data, skip it */
      if (!FD_ISSET(clients[i], &fdSet)) continue;

      /* read until the pipe is empty: 0 is closed, -1 is empty, -2 is error */
      Nread = 1;
      while (Nread > 0) {
	Nread = ReadtoIOBuffer (buffers[i], clients[i]);	
      }
      if ((Nread == 0) || (Nread == -2)) {
	/* error: do something */
	if (DEBUG && (Nread == 0)) fprintf (stderr, "socket is closed\n");
	if (DEBUG && (Nread == -2)) fprintf (stderr, "error reading from socket\n");
	DeleteClient (clients[i]);
	break;  // the other thread could also have modified the list; restart with new Ncurrent
      }

      if (DEBUG) fprintf (stderr, "read %d total bytes\n", buffers[i][0].Nbuffer);

      /* see if we have a complete message waiting; if not, keep waiting for messages */
      line = CheckForMessage (buffers[i]);
      if (line == NULL) continue;

      /* we now have a possible command from the client: run it */
      /* in this thread, we set the print output destination to be an
	 internal buffer, which we dump at the end of the execution */
      /* the commands sent to the server should not have ; */
      stripwhite (line);
      if (*line) {

	/* run the command, return the exit status */
	CommandLock();
	status = multicommand (line);
	CommandUnlock();
	SendMessage (clients[i], "STATUS %d", status);

	// return the stderr messages first
	outbuffer = gprintGetBuffer (GP_ERR);
	if (outbuffer) {
	  SendMessageFixed (clients[i], outbuffer[0].Nbuffer, outbuffer[0].buffer);
	} else {
	  SendMessageFixed (clients[i], 0, "");
	}	  
	FlushIOBuffer (outbuffer);

	// return the stdout messages first
	outbuffer = gprintGetBuffer (GP_LOG);
	if (outbuffer) {
	  SendMessageFixed (clients[i], outbuffer[0].Nbuffer, outbuffer[0].buffer);
	} else {
	  SendMessageFixed (clients[i], 0, "");
	}	  
	FlushIOBuffer (outbuffer);
      }
      free (line);
    }
    /* if Nread == -2, we probably need to drop the client */
    /* check if we need to drop / remove any clients */
    /* check if we need to shut down the thread */
  }
  return NULL;
}

/* the AddClient commands are issued by the parent thread
   the value of Nclients may increase after we check it here. 
   only this thread is allowed to decrease Nclients and remove
   a client from the table */

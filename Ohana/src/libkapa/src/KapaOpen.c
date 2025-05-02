# include "kapa_internal.h"

// kapa connection timeout is N_RETRY * 10000 usec
# define N_RETRY 5000

# define MY_PORT 2500
# define MY_PORT_MAX 2520
# define MY_WAIT 1000000
# define DEBUG 0

static int Nvalid = 0;
static u_int32_t *VALID = NULL;

int KapaLaunchCommand (char *line);

int KapaServerInit (KapaSockAddress *Address) {

  int status, InitSocket, length;

  Address[0].sin_family = AF_INET;
  Address[0].sin_port   = MY_PORT;
  Address[0].sin_addr.s_addr = INADDR_ANY; // use this line to bind any address / port?

retry_server:

  length = sizeof(Address[0]);

  InitSocket = socket (PF_INET, SOCK_STREAM, 0);
  if (InitSocket == -1) {
    perror ("socket: ");
    exit (2);
  }

  if (DEBUG) fprintf (stderr, "init sock: %d, len: %d, port %d\n", InitSocket, length, Address[0].sin_port);
  status = bind (InitSocket, (struct sockaddr *) Address, length);
  if (status == -1) {
    if (errno == EADDRINUSE) {
        close (InitSocket);
        Address[0].sin_port ++;
        if (Address[0].sin_port > MY_PORT_MAX) exit (2);
        goto retry_server;
    }
    perror ("bind: ");
    exit (2);
  }

  /* repeated starts of the server are limited by xinetd or something:
     requires 60sec timeout of the selected socket */

  if (DEBUG) fprintf (stderr, "bound to port: %d\n", Address[0].sin_port);
  status = listen (InitSocket, 10);
  if (status == -1) {
    perror ("listen: ");
    exit (2);
  }
  return (InitSocket);
}

int KapaServerWait (int InitSocket, KapaSockAddress *Address) {

  int i, status, BindSocket;
  KapaSockAddress Address_in;
  socklen_t length;
  u_int32_t addr;
  fd_set rfds;
  struct timeval wait;

  Address_in = Address[0];

  length = sizeof(Address_in);

  wait.tv_sec = 0;
  wait.tv_usec = MY_WAIT;

  /* do I need to clear rfds on each pass? */
  FD_ZERO(&rfds);
  FD_SET (InitSocket, &rfds);
  status = select (InitSocket + 1, &rfds, NULL, NULL, &wait);
  if (status == -1) {
    perror ("select");
    abort ();
  }
  if (!status) return (-1);

  if (DEBUG) fprintf (stderr, "init sock: %d, len: %d\n", InitSocket, length);
  BindSocket = accept (InitSocket, (struct sockaddr *) &Address_in, &length);
  if (DEBUG) fprintf (stderr, "bind sock: %d\n", BindSocket);
  if (BindSocket == -1) {
    perror ("accept: ");
    exit (2);
  }

  addr = Address_in.sin_addr.s_addr;
  if (DEBUG) {
    fprintf (stderr, "incoming connection from: ");
    fprintf (stderr, " %u", (0xff & (addr >>  0)));
    fprintf (stderr, ".%u", (0xff & (addr >>  8)));
    fprintf (stderr, ".%u", (0xff & (addr >> 16)));
    fprintf (stderr, ".%u", (0xff & (addr >> 24)));
    fprintf (stderr, "\n");
  }

  if (Nvalid == 0) goto accepted;

  for (i = 0; i < Nvalid; i++) {
    /* valid IP addresses may be machines (120.90.121.142) or
       class C networks (120.90.121.0) */

    /* for machine, address must match */
    if ((0xff & (VALID[i] >> 24)) != 0) {
      if (addr == VALID[i]) goto accepted;
    }

    /* for network, lower three bytes of address must match */
    if ((0xff & (VALID[i] >> 24)) == 0) {
      if ((0x00ffffff & addr) == VALID[i]) goto accepted;
    }
  }

  if (DEBUG) fprintf (stderr, "connection rejected\n");
  close (BindSocket);
  return (-1);

 accepted:
  {
    // we need to do some minimal handshake here.  I will send out 
    // a 4 char message : KAPA
    int Nout = write (BindSocket, "KAPA", 4);
    if (Nout != 4) {
      if (DEBUG) fprintf (stderr, "connection failed\n");
      close (BindSocket);
      return (-1);
    }
  }
  
  if (DEBUG) fprintf (stderr, "connection accepted\n");
  return (BindSocket);
}

/* load valid ip list */
int KapaDefineValidIP (char *ipstring) {

  int ip1, ip2, ip3, ip4, test, status;
  char string[80];

  if (Nvalid == 0) {
    Nvalid = 1;
    ALLOCATE (VALID, u_int32_t, Nvalid);
  } else {
    Nvalid ++;
    REALLOCATE (VALID, u_int32_t, Nvalid);
  }

  status = sscanf (ipstring, "%d.%d.%d.%d", &ip1, &ip2, &ip3, &ip4);
  test = TRUE;
  test &= (status == 4);
  test &= ((ip1 > 0) && (ip1 < 256));
  test &= ((ip2 > 0) && (ip2 < 256));
  test &= ((ip3 > 0) && (ip3 < 256));
  test &= ((ip4 >=0) && (ip4 < 256));
  if (!test) {
    fprintf (stderr, "invalid IP address %s\n", string);
    exit (2);
  }
  VALID[Nvalid-1] = ip1 | (ip2 << 8) | (ip3 << 16) | (ip4 << 24);
  return (TRUE);
}

/* connect to a running server on the specified host */
int KapaClientSocket (char *hostname) {

  int i, status, InitSocket, length;
  KapaSockAddress Address;
  struct hostent  *host;
  char tmpline[80], hostip[80];

  host = gethostbyname (hostname);
  bzero (hostip, 80);
  for (i = 0; i < host[0].h_length; i++) {
    sprintf (tmpline, "%u", (0xff & host[0].h_addr[i]));
    strcat (hostip, tmpline);
    if (i < host[0].h_length - 1) strcat (hostip, ".");
  }

  if (DEBUG) {
    fprintf (stderr, "trying %s (%s:%d)...", host[0].h_name, hostip, MY_PORT);
  }

  Address.sin_family = AF_INET;
  Address.sin_port   = MY_PORT;

retry_client:
  status = inet_aton (hostip, &Address.sin_addr);
  if (!status) {
    fprintf (stderr, "invalid address\n");
    exit (2);
  }

  length = sizeof(Address);

  InitSocket = socket (PF_INET, SOCK_STREAM, 0);
  if (InitSocket == -1) {
    perror ("socket: ");
    exit (2);
  }

  status = connect (InitSocket, (struct sockaddr *) &Address, length);
  if (status == -1) {
    if (DEBUG) fprintf (stderr, "error connecting: %d\n", errno);
    if (errno == ECONNREFUSED) {
      close (InitSocket);
      Address.sin_port ++;
      if (Address.sin_port > MY_PORT_MAX) return (-1);
      goto retry_client;
    }
    perror ("connect: ");
    exit (2);
  }

  // apparently, I can connect on someone else's port (eg GoogleTalkPlugin)
  // do a simple handshake before we set !NONBLOCK:

  // ensure the socket is NONBLOCK first...
  fcntl (InitSocket, F_SETFL, O_NONBLOCK);

  int Ntry = 0;
retry_message:
  { 
    char line[5];
    int Nout = read (InitSocket, line, 4);
    if ((Nout == -1) && (errno == EAGAIN)) {
      Ntry ++;
      if (Ntry > 500) {
	if (DEBUG) fprintf (stderr, "handshake failure\n");
	close (InitSocket);
	return (-1);
      }
      usleep (10000);
      goto retry_message;
    }
    if (Nout != 4) {
      if (DEBUG) fprintf (stderr, "connection failed\n");
      close (InitSocket);
      return (-1);
    }
    if (strncmp (line, "KAPA", 4)) {
      if (DEBUG) fprintf (stderr, "connection to the wrong server\n");
      close (InitSocket);
      return (-1);
    }
  }

  if (DEBUG) fprintf (stderr, "connected on port: %d\n", Address.sin_port);
  if (DEBUG) fprintf (stderr, "connected\n");

  // the client uses a BLOCKing socket by default
  fcntl (InitSocket, F_SETFL, !O_NONBLOCK);
  return (InitSocket);
}

int KapaOpen (char *kapa_exec, char *kapa_name) {

  // kapa_exec may be kapa://host, in which case we attempt to connect to an
  // already running kapa, or the program path, in which case we are supposed
  // to launch it locally, then connect to it.

  int sock, Ntry;
  char line[128];

  if (!strncmp (kapa_exec, "kapa://", 7)) {
    sock = KapaClientSocket (&kapa_exec[7]);
    return (sock);
  }

  if (kapa_name == NULL) {
    sprintf (line, "%s", kapa_exec);
  } else {
    sprintf (line, "%s -name '%s'", kapa_exec, kapa_name);
  }

  int pid = KapaLaunchCommand (line);
  if (!pid) {
    fprintf (stderr, "failed to launch kapa\n");
    return (-1);
  }

  INITTIME;

  Ntry = 0;
  while (Ntry < N_RETRY) {
    sock = KapaClientSocket ("localhost");
    if (sock != -1) break;
    if (errno != ECONNREFUSED) {
      perror ("KapaOpen");
      break;
    }
    // no connection yet. try again, but first check 
    // if the kapa job has exited
    int waitstatus;
    int result = waitpid (pid, &waitstatus, WNOHANG);
    if (result == -1) {
      fprintf (stderr, "problem checking for kapa\n");
      return (-1);
    }
    if (result > 0) {
      fprintf (stderr, "kapa exited\n");
      return (-1);
    }
    usleep (10000);
    Ntry ++;
  }

  if (sock < 0) {
    MARKTIME ("failed to connect to kapa after %f seconds\n", dtime);
    int killStatus = kill (pid, SIGKILL);
    if (killStatus) {
      perror ("failed to kill process");
    }
    int waitStatus = waitpid (pid, NULL, 0);
    if (waitStatus == pid) {
      fprintf (stderr, "harvested process %d\n", pid);
    } else if (waitStatus < 0) {
      fprintf (stderr, "failed to harvest process %d\n", pid);
    } else if (waitStatus == 0) {
      fprintf (stderr, "process not exited: %d\n", pid);
    } else {
      fprintf (stderr, "odd exit status: %d vs pid %d\n", waitStatus, pid);
    }
    return (-1);
  }

  return (sock);
}

/* start socketed connection (UNIX Socket) */
int KapaOpenNamedSocket (char *kapa_exec, char *name) {

  int InitSocket;
  struct sockaddr_un Address;
  socklen_t AddressLength;
  char temp[128], socket_name[64];
  int Ntry, fd;

  sprintf (socket_name, "/tmp/Kapa.XXXXXX");
  if ((fd = mkstemp (socket_name)) == -1) {
    fprintf (stderr, "error starting kapa\n");
    return (-1);
  }
  close (fd);
  unlink (socket_name);

  strcpy (Address.sun_path, socket_name);
  Address.sun_family = AF_UNIX;
  InitSocket = socket (AF_UNIX, SOCK_STREAM, 0);
  bind (InitSocket, (struct sockaddr *) &Address, sizeof (Address));
  listen (InitSocket, 1);

  if (name == NULL) {
    sprintf (temp, "%s -socket %s", kapa_exec, socket_name);
  } else {
    sprintf (temp, "%s -socket %s -name %s", kapa_exec, socket_name, name);
  }

  int pid = KapaLaunchCommand (temp);
  if (!pid) {
    fprintf (stderr, "failed to launch kapa\n");
    return (-1);
  }

  AddressLength =  sizeof (Address);
  fcntl (InitSocket, F_SETFL, O_NONBLOCK);

  INITTIME;

  Ntry = 0;
  while (Ntry < N_RETRY) {
    fd = accept (InitSocket, (struct sockaddr *)&Address, &AddressLength);
    if (fd != -1) break;
    if (errno != EAGAIN) break;
    // no connection yet. try again, but first check 
    // if the kapa job has exited
    int waitstatus;
    int result = waitpid (pid, &waitstatus, WNOHANG);
    if (result == -1) {
      fprintf (stderr, "problem checking for kapa\n");
      return (-1);
    }
    if (result > 0) {
      fprintf (stderr, "kapa exited\n");
      return (-1);
    }
    usleep (10000);
    Ntry ++;
  }

  if (fd < 0) {
    MARKTIME ("failed to connect to kapa after %f seconds\n", dtime);
    kill (pid, SIGKILL);
    waitpid (pid, NULL, WNOHANG);
    return (-1);
  }

  // the client uses a BLOCKing socket by default
  fcntl (fd, F_SETFL, !O_NONBLOCK);
  return (fd);
}

// wait for the initiating process to connect to the socket
int KapaWaitNamedSocket (char *sockpath) {

  int sock, status;
  struct sockaddr_un Address;

  strcpy (Address.sun_path, sockpath);
  Address.sun_family = AF_UNIX;
  sock = socket (AF_UNIX, SOCK_STREAM, 0);
  status = connect (sock, (struct sockaddr *) &Address, sizeof (Address));
  if (status < 0) {
    fprintf (stderr, "unsuccessful connection: %d\n", status);
    exit (0);
  }

  // the server uses an unblocked socket
  fcntl (sock, F_SETFL, O_NONBLOCK);
  unlink (sockpath);
  return (sock);
}

int KapaLaunchCommand (char *line) {

  // use fork & exec to launch the command; return the PID for testing exit
  int i, done, Nalloc, Nargv;
  char **argv, *p, *q;

  i = 0;
  Nalloc = 10;
  ALLOCATE (argv, char *, Nalloc);

  // split out line into unique words
  p = line;
  done = FALSE;
  while (!done) {
    q = parse_nextword (p);
    if (q && *q) {
      argv[i] = strncreate (p, q - p);
      stripwhite (argv[i]);
      p = q;
    } else {
      argv[i] = strcreate (p);
      stripwhite (argv[i]);
      done = TRUE;
    }
    i++;
    if (i == Nalloc - 1) {
      // need to keep one extra for the NULL pointer
      Nalloc += 10;
      REALLOCATE (argv, char *, Nalloc);
    }
  }
  Nargv = i;
  argv[Nargv] = NULL;

  int pid = fork ();
  if (!pid) { /* must be child process */
    execvp (argv[0], argv); 
    return (0);
  }
  for (i = 0; i < Nargv; i++) {
    if (argv[i]) free (argv[i]);
  }
  free (argv);

  if (pid == -1) {
    return (0);
  }

  return (pid);
}

int KapaClose (int fd) {

  if (fd < 1) return (FALSE);
  KiiSendCommand (fd, 4, "QUIT");
  close (fd);
  return (TRUE);
}

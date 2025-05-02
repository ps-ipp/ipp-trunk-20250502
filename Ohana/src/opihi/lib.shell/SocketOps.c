# include "shell.h"
# include <unistd.h>
# define HOST_NAME_MAX 256

# define MY_PORT 2000
# define MY_WAIT 500
# define DEBUG 0

// these three static variables are only modified in the setup command before
// the threads are started.  Thread-safety is not a problem for these.
static int NVALID;
static int Nvalid;
static int *VALID;

int GetPortRange (int *start, int *stop, char *portinfo);

int InitServerSocket (SockAddress *Address, char *hostname, char *portinfo) {

  int start, stop;
  int status, InitSocket, length;
  char myHostname[HOST_NAME_MAX];

  status = gethostname (myHostname, HOST_NAME_MAX);

  if (strcmp (hostname, myHostname)) {
    fprintf (stderr, "target host: %s, real host: %s\n", hostname, myHostname);
    fprintf (stderr, "please run on the correct host\n");
    exit (2);
  }

  GetPortRange (&start, &stop, portinfo);

  fprintf (stderr, "using port range %d - %d\n", start, stop);

  Address[0].sin_family = AF_INET;
  Address[0].sin_port   = start;
  Address[0].sin_addr.s_addr = INADDR_ANY; // use this line to bind any address / port?

retry_server:

  length = sizeof(Address[0]);

  InitSocket = socket (PF_INET, SOCK_STREAM, 0);
  if (InitSocket == -1) {
    perror ("socket: ");
    exit (5);
  }

  if (DEBUG) gprint (GP_ERR, "init sock: %d, len: %d\n", InitSocket, length);
  status = bind (InitSocket, (struct sockaddr *) Address, length);
  if (status == -1) {

# if (DEBUG)
    fprintf (stderr, "errno: %d\n", errno);
    fprintf (stderr, "EACCES: %d\n", EACCES);
    fprintf (stderr, "EBADF: %d\n", EBADF);
    fprintf (stderr, "EINVAL: %d\n", EINVAL);
    fprintf (stderr, "ENOTSOCK: %d\n", ENOTSOCK);
    fprintf (stderr, "EFAULT: %d\n", EFAULT);
    fprintf (stderr, "ELOOP: %d\n", ELOOP);
    fprintf (stderr, "ENAMETOOLONG: %d\n", ENAMETOOLONG);
    fprintf (stderr, "ENOENT: %d\n", ENOENT);
    fprintf (stderr, "ENOMEM: %d\n", ENOMEM);
    fprintf (stderr, "ENOTDIR: %d\n", ENOTDIR);
    fprintf (stderr, "EROFS: %d\n", EROFS);
    fprintf (stderr, "EADDRNOTAVAIL: %d\n", EADDRNOTAVAIL);
    fprintf (stderr, "EADDRINUSE: %d\n", EADDRINUSE);
    fprintf (stderr, "ENOSR: %d\n", ENOSR);
# endif

    if (errno == EADDRINUSE) {
	Address[0].sin_port ++;
	if (Address[0].sin_port > stop) {
	  fprintf (stderr, "failed to find a usable port\n");
	  exit (6);
	}
	goto retry_server;
    }
    perror ("bind: ");
    exit (7);
  }
  /* repeated starts of the server are limited by xinetd or something:
     requires 60sec timeout of the selected socket */

  fprintf (stderr, "bound to port: %d\n", Address[0].sin_port);
  status = listen (InitSocket, 10);
  if (status == -1) {
    perror ("listen: ");
    exit (8);
  }
  return (InitSocket);
}

int WaitServerSocket (int InitSocket, SockAddress *Address) {

  int i, BindSocket;
  SockAddress Address_in;
  socklen_t length;
  u_int32_t addr;

  Address_in = Address[0];

  length = sizeof(Address_in);

  /* this is a blocking wait; use in a separate thread */
  fcntl (InitSocket, F_SETFL, !O_NONBLOCK); 

  if (DEBUG) gprint (GP_ERR, "init sock: %d, len: %d\n", InitSocket, length);
  BindSocket = accept (InitSocket, (struct sockaddr *) &Address_in, &length);
  if (DEBUG) gprint (GP_ERR, "bind sock: %d\n", BindSocket);
  if (BindSocket == -1) {
    perror ("accept: ");
    exit (9);
  }

  addr = Address_in.sin_addr.s_addr;
  if (DEBUG) {
    gprint (GP_ERR, "incoming connection from: ");
    gprint (GP_ERR, " %u", (0xff & (addr >>  0)));
    gprint (GP_ERR, ".%u", (0xff & (addr >>  8)));
    gprint (GP_ERR, ".%u", (0xff & (addr >> 16)));
    gprint (GP_ERR, ".%u", (0xff & (addr >> 24)));
    gprint (GP_ERR, "\n");
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

  if (DEBUG) gprint (GP_ERR, "connection rejected\n");
  close (BindSocket);
  return (-1);

accepted:
  if (DEBUG) gprint (GP_ERR, "connection accepted\n");
  fcntl (BindSocket, F_SETFL, O_NONBLOCK); 
  return (BindSocket);
}

int GetPortRange (int *start, int *stop, char *portinfo) {

  // portinfo is a port or port range of the form NN or NN:MM
  *start = MY_PORT;
  *stop = *start + 10;
  if (!portinfo) return TRUE;
  if (*portinfo == 0) return TRUE;

  char *endptr;
  *start = strtol (portinfo, &endptr, 0);
  *stop = *start + 10; // default range of 10
  if (!endptr) {
    gprint (GP_ERR, "error in port range parsing : %s\n", portinfo);
    exit (20);
  }
  if (endptr == portinfo) {
    gprint (GP_ERR, "error in port range (%s), must be in form NN:MM or NN\n", portinfo);
    exit (20);
  }
  if (*endptr == 0) return TRUE;

  if (*endptr != ':') {
    gprint (GP_ERR, "error in port range %s (wrong range separator, must be in form NN:MM)\n", portinfo);
    exit (20);
  }
  char *ptr = endptr + 1;
  *stop = strtol (ptr, &endptr, 0);
  if (endptr == ptr)  {
    gprint (GP_ERR, "error in port range (%s), must be in form NN:MM or NN\n", portinfo);
    exit (20);
  }
  if (*stop - *start > 20) {
    gprint (GP_ERR, "error in port range (%s), range must be 20 or less\n", portinfo);
    exit (20);
  }
  if (*stop < *start) {
    gprint (GP_ERR, "error in port range (%s), stop must >= start\n", portinfo);
    exit (20);
  }
  return TRUE;
}

int GetClientSocket (char *hostname, char *portinfo) {

  int i, status, InitSocket, length, start, stop;
  SockAddress Address;
  struct hostent  *host;
  char tmpline[80], hostip[80];

  host = gethostbyname (hostname);
  if (!host) {
    gprint (GP_ERR, "cannot connect to pantasks server %s\n", hostname);
    exit (3);
  }

  bzero (hostip, 80);
  for (i = 0; i < host[0].h_length; i++) {
    sprintf (tmpline, "%u", (0xff & host[0].h_addr[i]));
    strcat (hostip, tmpline);
    if (i < host[0].h_length - 1) strcat (hostip, ".");
  }

  GetPortRange (&start, &stop, portinfo);

  if (DEBUG) {
    gprint (GP_ERR, "trying %s (%s:%d)...", host[0].h_name, hostip, start);
  }

  Address.sin_family = AF_INET;
  Address.sin_port   = start;

retry_client:
  status = inet_aton (hostip, &Address.sin_addr);
  if (!status) {
    gprint (GP_ERR, "invalid address\n");
    exit (10);
  }

  length = sizeof(Address);

  InitSocket = socket (PF_INET, SOCK_STREAM, 0);
  if (InitSocket == -1) {
    perror ("socket: ");
    exit (11);
  }

  status = connect (InitSocket, (struct sockaddr *) &Address, length);
  if (status == -1) {
    if (errno == ECONNREFUSED) {
      Address.sin_port ++;
      if (Address.sin_port > stop) exit (12);
      goto retry_client;
    }
    perror ("connect: ");
    exit (13);
  }

  if (DEBUG) gprint (GP_ERR, "connected on port: %d\n", Address.sin_port);
  if (DEBUG) gprint (GP_ERR, "connected\n");
  fcntl (InitSocket, F_SETFL, O_NONBLOCK); 
  return (InitSocket);
}

int InitServerSocket_Named (char *hostname, SockAddress *Address) {

  int i, status, InitSocket, length;
  struct hostent  *host;
  char tmpline[80], hostip[80];

  host = gethostbyname (hostname);
  bzero (hostip, 80);
  for (i = 0; i < host[0].h_length; i++) {
    sprintf (tmpline, "%u", (0xff & host[0].h_addr[i]));
    strcat (hostip, tmpline);
    if (i < host[0].h_length - 1) strcat (hostip, ".");
  }
  
  Address[0].sin_family = AF_INET;
  Address[0].sin_port   = MY_PORT;
  status = inet_aton (hostip, &Address[0].sin_addr);
  if (!status) {
    gprint (GP_ERR, "invalid address\n");
    exit (14);
  }

  length = sizeof(Address[0]);

  InitSocket = socket (PF_INET, SOCK_STREAM, 0);
  if (InitSocket == -1) {
    perror ("socket: ");
    exit (15);
  }

  if (DEBUG) gprint (GP_ERR, "init sock: %d, len: %d\n", InitSocket, length);
  status = bind (InitSocket, (struct sockaddr *) Address, length);
  if (status == -1) {
    perror ("bind: ");
    exit (16);
  }

  status = listen (InitSocket, 10);
  if (status == -1) {
    perror ("listen: ");
    exit (17);
  }

  if (DEBUG) gprint (GP_ERR, "socket listening on %s (%s:%d)\n", host[0].h_name, hostip, MY_PORT);
  return (InitSocket);
}

/* load valid ip list */
int DefineValidIP () {

  int i, Nvalid, ip1, ip2, ip3, ip4, test, status;
  char string[80];

  Nvalid = 0;
  NVALID = 10;
  ALLOCATE (VALID, int, NVALID);
  for (i = 0; VarConfigEntry ("VALID_IP", "%s", i, string) != NULL; i++) {
    status = sscanf (string, "%d.%d.%d.%d", &ip1, &ip2, &ip3, &ip4);
    test = TRUE;
    test &= (status == 4);
    test &= ((ip1 > 0) && (ip1 < 256)); 
    test &= ((ip2 > 0) && (ip2 < 256)); 
    test &= ((ip3 > 0) && (ip3 < 256)); 
    test &= ((ip4 >=0) && (ip4 < 256)); 
    if (!test) {
      gprint (GP_ERR, "invalid IP address %s\n", string);
      exit (18);
    }
    VALID[Nvalid] = ip1 | (ip2 << 8) | (ip3 << 16) | (ip4 << 24);
    Nvalid ++;
    CHECK_REALLOCATE (VALID, int, NVALID, Nvalid, 10);
  }
  NVALID = Nvalid;
  REALLOCATE (VALID, int, NVALID);
  if (NVALID == 0) {
    free (VALID);
    VALID = NULL;
  }
  return (TRUE);
}

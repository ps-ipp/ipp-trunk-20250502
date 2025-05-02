# include "addstar.h"

# define MY_PORT 2000
# define MY_WAIT 500

int InitServerSocket (SockAddress *Address) {

  int status, InitSocket, length;

# if (0)
  struct hostent  *host;
  char tmpline[80], hostip[80];

  host = gethostbyname (hostname);
  memset (hostip, 0, 80);
  for (i = 0; i < host[0].h_length; i++) {
    sprintf (tmpline, "%u", (0xff & host[0].h_addr[i]));
    strcat (hostip, tmpline);
    if (i < host[0].h_length - 1) strcat (hostip, ".");
  }
# endif
  
  Address[0].sin_family = AF_INET;
  Address[0].sin_port   = MY_PORT;
  Address[0].sin_addr.s_addr = INADDR_ANY; // use this line to bind any address / port?

# if (0)  
  status = inet_aton (hostip, &Address[0].sin_addr);
  if (!status) {
    fprintf (stderr, "invalid address\n");
    exit (2);
  }
# endif

  length = sizeof(Address[0]);

  InitSocket = socket (PF_INET, SOCK_STREAM, 0);
  if (InitSocket == -1) {
    perror ("socket: ");
    exit (2);
  }

  fprintf (stderr, "init sock: %d, len: %d\n", InitSocket, length);
  status = bind (InitSocket, (struct sockaddr *) Address, length);
  if (status == -1) {
    perror ("bind: ");
    exit (2);
  }

  status = listen (InitSocket, 10);
  if (status == -1) {
    perror ("listen: ");
    exit (2);
  }

  // if (VERBOSE) fprintf (stderr, "socket listening on %s (%s:%d)\n", host[0].h_name, hostip, MY_PORT);
  return (InitSocket);
}

int WaitServerSocket (int InitSocket, SockAddress *Address, int *validIP, int Nvalid) {

  int i, BindSocket;
  socklen_t length;
  SockAddress Address_in;
  u_int32_t addr;

  Address_in = Address[0];

  length = sizeof(Address_in);

  /* this is a blocking wait; use in a separate thread */
  fcntl (InitSocket, F_SETFL, !O_NONBLOCK); 

  fprintf (stderr, "init sock: %d, len: %d\n", InitSocket, length);
  BindSocket = accept (InitSocket, (struct sockaddr *) &Address_in, &length);
  fprintf (stderr, "bind sock: %d\n", BindSocket);
  if (BindSocket == -1) {
    perror ("accept: ");
    exit (2);
  }

  addr = Address_in.sin_addr.s_addr;
  if (VERBOSE) {
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
    if ((0xff & (validIP[i] >> 24)) != 0) {
      if (addr == validIP[i]) goto accepted;
    }

    /* for network, lower three bytes of address must match */
    if ((0xff & (validIP[i] >> 24)) == 0) {
      if ((0x00ffffff & addr) == validIP[i]) goto accepted;
    }
  }

  if (VERBOSE) fprintf (stderr, "connection rejected\n");
  close (BindSocket);
  return (-1);

accepted:
  if (VERBOSE) fprintf (stderr, "connection accepted\n");
  fcntl (BindSocket, F_SETFL, O_NONBLOCK); 
  return (BindSocket);
}

int GetClientSocket (char *hostname) {

  int i, status, InitSocket, length;
  SockAddress Address;
  struct hostent  *host;
  char tmpline[80], hostip[80];

  host = gethostbyname (hostname);
  memset (hostip, 0, 80);
  for (i = 0; i < host[0].h_length; i++) {
    sprintf (tmpline, "%u", (0xff & host[0].h_addr[i]));
    strcat (hostip, tmpline);
    if (i < host[0].h_length - 1) strcat (hostip, ".");
  }

  if (VERBOSE) {
    fprintf (stderr, "trying %s (%s:%d)...", host[0].h_name, hostip, MY_PORT);
  }

  Address.sin_family = AF_INET;
  Address.sin_port   = MY_PORT;
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
    perror ("connect: ");
    exit (2);
  }

  if (VERBOSE) fprintf (stderr, "connected\n");
  fcntl (InitSocket, F_SETFL, O_NONBLOCK); 
  return (InitSocket);
}

int InitServerSocket_Named (char *hostname, SockAddress *Address) {

  int i, status, InitSocket, length;
  struct hostent  *host;
  char tmpline[80], hostip[80];

  host = gethostbyname (hostname);
  memset (hostip, 0, 80);
  for (i = 0; i < host[0].h_length; i++) {
    sprintf (tmpline, "%u", (0xff & host[0].h_addr[i]));
    strcat (hostip, tmpline);
    if (i < host[0].h_length - 1) strcat (hostip, ".");
  }
  
  Address[0].sin_family = AF_INET;
  Address[0].sin_port   = MY_PORT;
  status = inet_aton (hostip, &Address[0].sin_addr);
  if (!status) {
    fprintf (stderr, "invalid address\n");
    exit (2);
  }

  length = sizeof(Address[0]);

  InitSocket = socket (PF_INET, SOCK_STREAM, 0);
  if (InitSocket == -1) {
    perror ("socket: ");
    exit (2);
  }

  fprintf (stderr, "init sock: %d, len: %d\n", InitSocket, length);
  status = bind (InitSocket, (struct sockaddr *) Address, length);
  if (status == -1) {
    perror ("bind: ");
    exit (2);
  }

  status = listen (InitSocket, 10);
  if (status == -1) {
    perror ("listen: ");
    exit (2);
  }

  if (VERBOSE) fprintf (stderr, "socket listening on %s (%s:%d)\n", host[0].h_name, hostip, MY_PORT);
  return (InitSocket);
}


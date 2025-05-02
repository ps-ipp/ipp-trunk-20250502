# include "display.h"
# include "shell.h"
# include <assert.h>


/* kapa support for the new version of kapa (v2.0), which has both graph and image
 * elements merged into a single display device.  The user may now open an arbitrary
 * number of kapa windows, and the display information is retrieved from kapa across the
 * socket when it is needed.  Communication is now via an INET socket (not a UNIX socket).
 */

/* list of available socket connections */
static int        Active;        // currently active socket entry (index value, not socket value)
static int       *Socket = NULL; // list of available sockets
static char     **Device = NULL; // list of device names for each socket
static int       Ndevice = 0;    // number of available sockets / devices

void InitKapa () {

  Active  = -1;					  // -1 is the INVALID entry
  Ndevice = 0;					  // number of defined sockets
  ALLOCATE (Device, char *, 1);			  // for future REALLOCATE calls
  ALLOCATE (Socket, int, 1);			  // for future REALLOCATE calls
}

void FreeKapa () {
  
  free (Device);
  free (Socket);
}

// add new device name if not found
int AddKapaDevice (char *name) {

  int N;

  N = FindKapaDevice (name);
  if (N != -1) return (N);
  N = Ndevice;
  Ndevice ++;
  REALLOCATE (Device, char *, Ndevice);
  REALLOCATE (Socket, int, Ndevice);
  Device[N] = strcreate (name);
  Socket[N] = -1;
  return (N);
}

// delete device by name, close if not closed
int DelKapaDevice (char *name) {

  int i, N;

  N = FindKapaDevice (name);
  if (N == -1) return (FALSE);

  if (Socket[N] != -1) close (Socket[N]);
  free (Device[N]);
  for (i = N; i < Ndevice - 1; i++) {
    Device[i] = Device[i+1];
    Socket[i] = Socket[i+1];
  }

  if (N == Active) {
    Active = -1;
  }

  Ndevice --;
  REALLOCATE (Device, char *, Ndevice);
  REALLOCATE (Socket, int, Ndevice);

  return (TRUE);
}

// returns the entry of the requested device, or -1 if not found
int FindKapaDevice (char *name) {

  int i;

  if (name == NULL) return (-1); 

  for (i = 0; i < Ndevice; i++) {
    if (!strcmp(Device[i], name)) return (i);
  }
  return (-1);
}

// set the active device to the given device, open if needed
int open_kapa (int entry) {

  int fd;
  char *kapa_exec, *kapa_name;

  // find the given device number, or create. set this to active
  assert (entry >= 0);
  assert (entry <  Ndevice);

  // if the (now) active socket is not open, open it
  if (Socket[entry] < 0) {
    kapa_exec = get_variable ("KAPA");
    if (kapa_exec == (char *) NULL) {
      gprint (GP_ERR, "variable KAPA not found\n");
      return (FALSE);
    }

    // KAPA may be either kapa://host or /path/to/program
    ALLOCATE (kapa_name, char, strlen(Device[entry]) + 5);
    snprintf (kapa_name, strlen(Device[entry]) + 5, "[%s]", Device[entry]);

    if (!strncmp (kapa_exec, "unix://", 7)) {
        fd = KapaOpenNamedSocket (&kapa_exec[7], kapa_name);
    } else {
	fd = KapaOpen (kapa_exec, kapa_name);
    }

    free (kapa_exec);
    free (kapa_name);

    if (fd < 0) {
      gprint (GP_ERR, "error starting kapa device %s\n", Device[entry]);
      return (FALSE);
    } 
    Socket[entry] = fd;
  } 
  Active = entry;
  return (TRUE);
}

/**************** graph specific ops *******************/

// return the current device name, if set 
char *GetKapaName () {
  if (Active < 0) return NULL;
  return Device[Active];
}

/* return pointers for named device or current; open if needed */
// if fd == NULL, don't return the value
// if name == NULL, use the currently active device
int GetGraph (Graphdata *data, int *fd, char *name) {

  int entry;

  if (name == NULL) {
    if (Active < 0) {
      entry = AddKapaDevice ("0");
    } else {
      entry = Active;
    }
  } else {
    entry = AddKapaDevice (name);
  }
  
  if (!open_kapa (entry)) {
    return (FALSE);
  }
  
  if (data != NULL) KapaGetGraphData (Socket[Active], data);
  if (fd != NULL) *fd = Socket[Active];

  return (TRUE);
}

/* return pointers for given kapa, don't set or open */
int GetGraphdata (Graphdata *data, int *fd, char *name) {

  int entry;

  if (name == NULL) {
    if (Active < 0) {
      gprint (GP_ERR, "no active kapa window\n"); 
      return (FALSE);
    }
    entry = Active;
  } else {
    entry = FindKapaDevice (name);
    if (entry < 0) {
      gprint (GP_ERR, "invalid kapa window %s\n", name); 
      return (FALSE);
    }
  }

  if (fd != NULL) *fd = Socket[entry];
  if (data != NULL) KapaGetGraphData (Socket[entry], data);
  return (TRUE);
}

/* assign given values to current kapa */
int SetGraph (Graphdata *data) {
  if (Active < 0) {
    gprint (GP_ERR, "no active kapa window\n"); 
    return (FALSE);
  }
  if (Socket[Active] == -1) {
    gprint (GP_ERR, "no active kapa window\n"); 
    return (FALSE);
  }
  KapaSetGraphData (Socket[Active], data);
  return (TRUE);
}

/************* image ops ***********/

/* return pointers for current Ximage, set if desired, test, open if needed */
int GetImage (KapaImageData *data, int *fd, char *name) {

  int entry;

  if (name == NULL) {
    if (Active < 0) {
      entry = AddKapaDevice ("0");
    } else {
      entry = Active;
    }
  } else {
    entry = AddKapaDevice (name);
  }
  
  if (!open_kapa (entry)) {
    return (FALSE);
  }
  
  if (data != NULL) KapaGetImageData (Socket[Active], data);
  if (fd != NULL) *fd = Socket[Active];

  return (TRUE);
}

/* return pointers for given kapa, don't set or open */
int GetImageData (KapaImageData *data, int *fd, char *name) {

  int entry;

  if (name == NULL) {
    if (Active < 0) {
      gprint (GP_ERR, "no active kapa window\n"); 
      return (FALSE);
    }
    entry = Active;
  } else {
    entry = FindKapaDevice (name);
    if (entry < 0) {
      gprint (GP_ERR, "invalid kapa window %s\n", name); 
      return (FALSE);
    }
  }

  if (fd != NULL) *fd = Socket[entry];
  if (data != NULL) KapaGetImageData (Socket[entry], data);
  return (TRUE);
}

/* assign given values to current kapa */
int SetImage (KapaImageData *data) {
  if (Active < 0) {
    gprint (GP_ERR, "no active kapa window\n"); 
    return (FALSE);
  }
  if (Socket[Active] == -1) {
    gprint (GP_ERR, "no active kapa window\n"); 
    return (FALSE);
  }
  KapaSetImageData (Socket[Active], data);
  return (TRUE);
}

int close_kapa (char *name) {

  struct timespec request, remain;
  int N;

  N = FindKapaDevice (name);
  if (N == -1) {
    if (Active < 0) return (FALSE);
    name = Device[Active];
  }
  DelKapaDevice (name);

  // avoid blocking on waitpid, test every 1 msec, up to 500 msec
  request.tv_sec = 0;
  request.tv_nsec = 1000000;

  // try to harvest the child PID
  int waitstatus = 0;
  int result = waitpid (-1, &waitstatus, WNOHANG);
  for (int i = 0; (i < 500) && (result == 0); i++) {
    nanosleep (&request, &remain);
    result = waitpid (-1, &waitstatus, WNOHANG);
  }

  if ((result == -1) && (errno != ECHILD)) {
    fprintf (stderr, "unexpected error from waitpid (%d): programming error\n", errno);
  }
  if (result == 0) {
    fprintf (stderr, "child did not exit (close_kapa), timeout");
  }

  return (TRUE);
}

void QuitKapa () {

  int i;
  
  for (i = 0; i < Ndevice; i++) {
    if (Socket[i] != -1) close (Socket[i]);
    if (Device[i] != NULL) free (Device[i]);
  }
  REALLOCATE (Socket, int, 1);
  REALLOCATE (Device, char *, 1);

  Ndevice = 0;
  Active = -1;
}

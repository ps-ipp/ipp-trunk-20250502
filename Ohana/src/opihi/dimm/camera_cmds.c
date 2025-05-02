# include "dimm.h"
# define EXIT_STATUS(S) { seteuid (UID); return (S); }

static uid_t UID, EUID;

SetEUID () {

  /* save the UID (ID of calling process) and EUID (should be root) */
  UID = getuid ();
  EUID = geteuid ();
  seteuid (UID);
}

int camera (int argc, char **argv) {
  
  /* USAGE: 
     camera init port
     camera expose exptime
     camera readout x y dx dy
     camera temp set value
     camera temp get var
  */

  if (argc < 2) goto usage;

  seteuid (EUID);
  
  if (!strcasecmp (argv[1], "init")) {
    int port, status;

    if (argc != 3) {
      gprint (GP_ERR, "USAGE: camera init (port)\n");
      EXIT_STATUS (FALSE);
    }
    sscanf (argv[2], "%x", &port);
    status = InitCamera (port);
    EXIT_STATUS (status);
  }

  if (!strcasecmp (argv[1], "expose")) {

    int status;
    double exptime;

    if (argc != 3) {
      gprint (GP_ERR, "USAGE: camera expose (exptime)\n");
      EXIT_STATUS (FALSE);
    }
    exptime = atof (argv[2]);
    status = Exposure (exptime);
    EXIT_STATUS (status);
  }

  if (!strcasecmp (argv[1], "temp")) {

    int status;
    double temp;

    if (argc != 3) {
      gprint (GP_ERR, "USAGE: camera temp (temperature)\n");
      EXIT_STATUS (FALSE);
    }
    temp = atof (argv[2]);
    status = SetTemperature (temp);
    EXIT_STATUS (status);
  }

  if (!strcasecmp (argv[1], "status")) {

    int status;
    double temp;

    if (argc != 2) {
      gprint (GP_ERR, "USAGE: camera status\n");
      EXIT_STATUS (FALSE);
    }
    DumpCameraStatus ();
    EXIT_STATUS (TRUE);
  }

  if (!strcasecmp (argv[1], "readout")) {

    int Nbuf, status;
    double temp;
    int x, y, dx, dy, NX, NY;
    Buffer *buf;

    if ((argc != 7) && (argc != 3)) {
      gprint (GP_ERR, "USAGE: camera readout (buffer) x y dx dy\n");
      EXIT_STATUS (FALSE);
    }

    if ((buf = SelectBuffer (argv[2], OLDBUFFER, TRUE)) == NULL) EXIT_STATUS (FALSE);

    CameraFullSize (&NX, &NY);
    x = y = 0;
    dx = NX;
    dy = NY;
    if (argc == 7) {
      x  = atof (argv[3]);
      y  = atof (argv[4]);
      dx = atof (argv[5]);
      dy = atof (argv[6]);
    } 

    /* generate a buffer to store the image */
    gfits_free_matrix (&buf[0].matrix);
    gfits_free_header (&buf[0].header);
    if (!CreateBuffer (buf, dx, dy, -32, 0.0, 1.0)) return FALSE;
    strcpy (buf[0].file, "(empty)");

    ReadOut (x, y, dx, dy, 1, buf[0].matrix.buffer);

    gfits_convert_format (&buf[0].header, &buf[0].matrix, -32, 1.0, 0.0, 0xffff, gfits_get_unsign_mode());

    EXIT_STATUS (TRUE);
  }

usage:
  gprint (GP_ERR, "camera init port\n");
  gprint (GP_ERR, "camera expose exptime\n");
  gprint (GP_ERR, "camera readout x y dx dy\n");
  gprint (GP_ERR, "camera temp set value\n");
  gprint (GP_ERR, "camera temp get var\n");
  seteuid (UID);
  return (FALSE);

}



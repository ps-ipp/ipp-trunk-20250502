# include "dimm.h"

double distSky (double r1, double r2, double d1, double d2);

int telescope (int argc, char **argv) {
  
  if (argc < 2) goto usage;

  if (!strcasecmp (argv[1], "init")) {
    if (argc != 3) {
      gprint (GP_ERR, "USAGE: telescope init (port)\n");
      return (FALSE);
    }
    if (!SerialInit (argv[2])) return (FALSE);
    gprint (GP_ERR, "telescope on port %s\n", argv[2]);
    return (TRUE);
  }

  if (!strcasecmp (argv[1], "cmd")) {

    int status;
    char *answer = (char *) NULL;

    if (argc != 3) {
      gprint (GP_ERR, "USAGE: telescope cmd (string)\n");
      return (FALSE);
    }
    status = SerialCommand (argv[2], &answer, 10);
    gprint (GP_ERR, "status: %d\n", status);
    if (answer != (char *) NULL) {
      gprint (GP_ERR, "answer: ..%s..\n", answer);
    }
    return (TRUE);
  }

  if (!strcasecmp (argv[1], "ack")) {

    int status;
    char line[32], *answer;

    if (argc != 3) {
      gprint (GP_ERR, "USAGE: telescope cmd (string)\n");
      return (FALSE);
    }
    line[0] = 0x06;
    line[1] = 0;
    status = SerialCommand (line, &answer, 10);
    gprint (GP_ERR, "status: %d\n", status);
    if (answer != (char *) NULL) {
      gprint (GP_ERR, "answer: ..%s..\n", answer);
    }
    return (TRUE);
  }

  if (!strcasecmp (argv[1], "slew")) {

    double ra, dec;
    int status;

    if (argc != 4) {
      gprint (GP_ERR, "USAGE: telescope slew (ra) (dec)\n");
      return (FALSE);
    }

    ra  = ohana_normalize_angle(atof (argv[2]));
    dec = atof (argv[3]);

    status = gotoRD (ra, dec);

    if (!status) return (FALSE);
    return (TRUE);
  }

  if (!strcasecmp (argv[1], "coords")) {

    int status;
    double ra, dec;
    char line[64];

    if (argc != 2) {
      gprint (GP_ERR, "USAGE: telescope coords\n");
      return (FALSE);
    }

    if (!getRD (&ra, &dec)) return (FALSE);
    gprint (GP_ERR, "%f %f\n", ra, dec);
    set_variable ("RA", ra);
    set_variable ("DEC", dec);
    dms_format (line, 64, (ra/15.0));
    set_str_variable ("Rs", line);
    dms_format (line, 64, dec);
    set_str_variable ("Ds", line);

    return (TRUE);
  }

  if (!strcasecmp (argv[1], "altaz")) {

    int status;
    double x, y;

    if (argc != 2) {
      gprint (GP_ERR, "USAGE: telescope altaz\n");
      return (FALSE);
    }

    if (!getXY (&x, &y)) return (FALSE);
    gprint (GP_ERR, "%f %f\n", x, y);
    set_variable ("ALT", x);
    set_variable ("AZ", y);
    return (TRUE);
  }

  if (!strcasecmp (argv[1], "site")) {

    int status;
    double lon, lat, LST;

    if (argc != 2) {
      gprint (GP_ERR, "USAGE: telescope site\n");
      return (FALSE);
    }

    if (!getSite (&lon, &lat, &LST)) return (FALSE);
    gprint (GP_ERR, "%f %f  %f\n", lon, lat, LST);
    set_variable ("LON", lon);
    set_variable ("LAT", lat);
    set_variable ("LST", LST);
    return (TRUE);
  }

  if (!strcasecmp (argv[1], "setsite")) {

    double lon, lat;

    if (argc != 5) {
      gprint (GP_ERR, "USAGE: telescope setsite (name) (longitude) (latitude)\n");
      return (FALSE);
    }

    lon = atof (argv[3]);
    lat = atof (argv[4]);
    if (!setSite (argv[2], lon, lat)) return (FALSE);
    set_variable ("LON", lon);
    set_variable ("LAT", lat);
    return (TRUE);
  }

  if (!strcasecmp (argv[1], "settime")) {

    if (argc != 3) {
      gprint (GP_ERR, "USAGE: telescope settime (lst)\n");
      return (FALSE);
    }

    if (!setTime (argv[2])) return (FALSE);
    return (TRUE);
  }

  if (!strcasecmp (argv[1], "verbose")) {

    int mode;

    if (argc != 3) {
      gprint (GP_ERR, "USAGE: telescope verbose (mode)\n");
      return (FALSE);
    }

    mode = atoi (argv[2]);
    SerialVerbose (mode);
    return (TRUE);
  }

  if (!strcasecmp (argv[1], "setcoords")) {

    int status;
    double ra, dec;

    if (argc != 4) {
      gprint (GP_ERR, "USAGE: telescope setcoords (ra) (dec)\n");
      return (FALSE);
    }

    ra  = atof (argv[2]);
    dec = atof (argv[3]);

    if (!setRD (ra, dec)) return (FALSE);
    return (TRUE);
  }

  if (!strcasecmp (argv[1], "offset")) {

    int status;

    if (argc != 4) {
      gprint (GP_ERR, "USAGE: telescope offset (direction) (distance)\n");
      gprint (GP_ERR, "  direction : x or y\n");
      gprint (GP_ERR, "  distance  : +arcmin or -arcmin\n");
      return (FALSE);
    }
    status = offset (argv[2], atof (argv[3]));

    if (!status) return (FALSE);
    return (TRUE);
  }

  if (!strcasecmp (argv[1], "toffset")) {

    int status;

    if (argc != 5) {
      gprint (GP_ERR, "USAGE: telescope toffset (direction) (rate) (duration)\n");
      gprint (GP_ERR, "example: telescope toffset x RC 1.5\n");
      return (FALSE);
    }
    status = toffset (argv[2], argv[3], atof(argv[4]));

    if (!status) return (FALSE);
    return (TRUE);
  }

 usage:
  gprint (GP_ERR, "telescope init port - set serial port (eg, /dev/ttyS0)\n");
  gprint (GP_ERR, "telescope site - get site information\n");
  gprint (GP_ERR, "telescope altaz - g\n");
  gprint (GP_ERR, "telescope setcoords (ra) (dec)\n");
  gprint (GP_ERR, "telescope setsite (name) (longitute) (latitude)\n");
  gprint (GP_ERR, "telescope coords\n");
  gprint (GP_ERR, "telescope slew (ra) (dec)\n");
  gprint (GP_ERR, "telescope offset (direction) (duration)\n");
  gprint (GP_ERR, "telescope toffset (direction) (rate) (duration)\n");
  return (FALSE);
}


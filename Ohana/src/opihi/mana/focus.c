# include "dimm.h"

int focus (int argc, char **argv) {
  
  if (argc < 2) goto usage;

  if (!strcasecmp (argv[1], "init")) {
    if (argc != 3) {
      gprint (GP_ERR, "USAGE: focus init (port)\n");
      return (FALSE);
    }
    if (!SerialInit (argv[2])) return (FALSE);
    gprint (GP_ERR, "focus on port %s\n", argv[2]);
    return (TRUE);
  }

  if (!strcasecmp (argv[1], "pos")) {

    int status, servo, angle;
    char *answer = (char *) NULL;
    char line[64];

    if (argc != 4) {
      gprint (GP_ERR, "USAGE: focus pos Nservo (angle)\n");
      return (FALSE);
    }

    servo = atoi (argv[2]);
    angle = atoi (argv[3]);

    sprintf (line, "%c%c%c\n", 0xff, servo, angle);
    status = SerialCommand (line, &answer, 10);
    gprint (GP_ERR, "status: %d\n", status);
    if (answer != (char *) NULL) {
      gprint (GP_ERR, "answer: ..%s..\n", answer);
    }
    return (TRUE);
  }

  if (!strcasecmp (argv[1], "raw")) {

    int status, value;
    char *answer = (char *) NULL;
    char line[64];

    if (argc != 3) {
      gprint (GP_ERR, "USAGE: focus raw value\n");
      return (FALSE);
    }

    value = atoi (argv[2]);

    sprintf (line, "%c", value);
    status = SerialCommand (line, &answer, 10);
    gprint (GP_ERR, "status: %d\n", status);
    if (answer != (char *) NULL) {
      gprint (GP_ERR, "answer: ..%s..\n", answer);
    }
    return (TRUE);
  }

 usage:
  gprint (GP_ERR, "focus init port\n");
  gprint (GP_ERR, "focus pos (string)\n");
  gprint (GP_ERR, "focus raw (value)\n");
  return (FALSE);
}


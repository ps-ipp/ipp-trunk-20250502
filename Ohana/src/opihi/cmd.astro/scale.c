# include "astro.h"

int scale (int argc, char **argv) {

  Buffer *buf;

  if (argc != 5) {
    gprint (GP_ERR, "USAGE: scale (buffer) (key) [-r/-w] (value)\n");
    return (FALSE);
  }  

  if ((buf = SelectBuffer (argv[1], OLDBUFFER, TRUE)) == NULL) return (FALSE);

  if (strcasecmp (argv[2], "bzero") && strcasecmp (argv[2], "bscale")) {
    gprint (GP_ERR, "use bzero or bscale only\n");
    return (FALSE);
  }
    
  if (strcmp (argv[3], "-r") && strcmp (argv[3], "-w")) {
    gprint (GP_ERR, "use -r or -w only\n");
    return (FALSE);
  }
    
  if (!strcmp (argv[3], "-r")) {
    if (!strcasecmp (argv[2], "bzero")) {
      set_variable (argv[4], (double) buf[0].bzero);
    } else {
      set_variable (argv[4], (double) buf[0].bscale);
    }      
  } else {
    if (!strcasecmp (argv[2], "bzero")) {
      buf[0].bzero = atof (argv[4]);
    } else {
      buf[0].bscale = atof (argv[4]);
    }      
  }

  return (TRUE);
}

/* get or set external bzero / bscale values 
   (these keywords are set to 0,1 internally, 
   so we can't just manipulate them like other keywords */

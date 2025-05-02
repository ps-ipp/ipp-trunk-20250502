# include "opihi.h"

int deimos	    PROTO((int, char **));
int findpeaks	    PROTO((int, char **));
int fitcontour	    PROTO((int, char **));
int starcontour	    PROTO((int, char **));
int rawstars	    PROTO((int, char **));
int version	    PROTO((int, char **));

static Command cmds[] = {  
  {1, "deimos",      deimos,       "deimos multislit spectrograph tools"},
  {1, "findpeaks",   findpeaks,    "find image peaks"},
  {1, "fitcontour",  fitcontour,   "fit ellipse contour"},
  {1, "starcontour", starcontour,  "object contour"},
  {1, "rawstars",    rawstars,     "find raw star stats"},
  {1, "version",     version,      "show version information"},
}; 

void InitMana () {
  
  int i;

  for (i = 0; i < sizeof (cmds) / sizeof (Command); i++) {
    AddCommand (&cmds[i]);
  }

}

void FreeMana () {
}

# include "dimm.h"

int camera          PROTO((int, char **));
int findstars       PROTO((int, char **));
int telescope       PROTO((int, char **));
int version         PROTO((int, char **));

static Command cmds[] = {  
  {1, "camera",    camera,    "camera functions"},
  {1, "findstars", findstars, "find objects on image"},
  {1, "telescope", telescope, "telescope communications"},
  {1, "version",   version,   "show version information"},
}; 

void InitDIMM () {
  
  int i;

  for (i = 0; i < sizeof (cmds) / sizeof (Command); i++) {
    AddCommand (&cmds[i]);
  }

}

void FreeDIMM () {
}

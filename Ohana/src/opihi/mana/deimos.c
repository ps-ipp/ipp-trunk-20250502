# include "data.h"

int deimos_mkslit (int argc, char **argv);
int deimos_fitslit (int argc, char **argv);
int deimos_mkobj (int argc, char **argv);
int deimos_getobj (int argc, char **argv);
// int deimos_getalt (int argc, char **argv);
int deimos_fitobj (int argc, char **argv);
// int deimos_fitalt (int argc, char **argv);
int deimos_arclines (int argc, char **argv);
int deimos_fitarc (int argc, char **argv);
int deimos_fitprofile (int argc, char **argv);

static Command deimos_commands[] = {
  {1, "fitobj",     deimos_fitobj,     "fit for object parameters using LMM"},
  {1, "fitalt",     deimos_fitobj,     "fit for object parameters using LMM"},
  {1, "getobj",     deimos_getobj,     "determine crude object parameters"},
  {1, "getalt",     deimos_getobj,     "determine crude object parameters"},
  {1, "mkobj",      deimos_mkobj,      "make a full object image"},
  {1, "mkslit",     deimos_mkslit,     "make a slit image"},
  {1, "fitslit",    deimos_fitslit,    "fit slit image to observed slit flux"},
  {1, "arclines",   deimos_arclines,   "detect arclines using LSF and STILT"},
  {1, "fitarc",     deimos_fitarc,     "fit arclamp lines"},
  {1, "fitprofile", deimos_fitprofile, "fit slit profile"},
};

int deimos (int argc, char **argv) {

  int i, N, status;

  if (argc < 2) {
    gprint (GP_ERR, "USAGE: deimos (command)\n");
    gprint (GP_ERR, "    deimos mkslit : make slit image\n");
    return (FALSE);
  }

  N = sizeof (deimos_commands) / sizeof (Command);

  /* find the deimos sub-command which matches */
  for (i = 0; i < N; i++) {
    if (!strcmp (deimos_commands[i].name, argv[1])) {
      status = (*deimos_commands[i].func) (argc - 1, argv + 1);
      return (status);
    }
  }

  gprint (GP_ERR, "unknown deimos command %s\n", argv[1]);
  return (FALSE);
}


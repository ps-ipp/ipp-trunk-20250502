# include "data.h"

int spline_list (int argc, char **argv);
int spline_help (int argc, char **argv);
int spline_create (int argc, char **argv);
int spline_apply (int argc, char **argv);
int spline_load (int argc, char **argv);
int spline_save (int argc, char **argv);
int spline_print (int argc, char **argv);
int spline_delete (int argc, char **argv);
int spline_rename (int argc, char **argv);
int spline_getspline (int argc, char **argv);

static Command spline_commands[] = {
  {1, "help",       spline_help,       "list spline help info"},
  {1, "list",       spline_list,       "list splines"},
  {1, "create",     spline_create,     "create a spline"},
  {1, "apply",      spline_apply,      "apply a spline"},
  {1, "load",       spline_load,       "write a spline to a FITS file"},
  {1, "save",       spline_save,       "read a spline from a FITS file"},
  {1, "print",      spline_print,      "print a spline to stdout"},
  {1, "delete",     spline_delete,     "delete a spline"},
  {1, "rename",     spline_rename,     "rename a spline"},
};

int spline_help (int argc, char **argv) {
  OHANA_UNUSED_PARAM(argc);
  OHANA_UNUSED_PARAM(argv);

  gprint (GP_ERR, "USAGE: spline (command)\n");
  gprint (GP_ERR, "    spline help                                  : this listing\n");
  gprint (GP_ERR, "    spline list                                  : list splines\n");
  gprint (GP_ERR, "    spline create (spline) (Xknots) (Yknots)     : create a spline\n");
  gprint (GP_ERR, "    spline apply  (spline) (Xpts) (Ypts)         : apply a spline to Xpts to get Ypts\n");
  gprint (GP_ERR, "    spline delete (spline)                       : delete named spline\n");
  gprint (GP_ERR, "    spline rename (spline) (new)                 : change spline name to new name\n");
  gprint (GP_ERR, "    spline load   (spline) (filename)            : load a spline from a FITS file\n");
  gprint (GP_ERR, "    spline save   (spline) (filename) [-append]  : save a spline in FITS format\n");
  gprint (GP_ERR, "    spline print  (spline)                       : print a spline to stdout\n");

  return FALSE;
}

int spline_command (int argc, char **argv) {

  int i, N, status;

  if (argc < 2) {
    spline_help(0,NULL);
    return (FALSE);
  }

  N = sizeof (spline_commands) / sizeof (Command);

  /* find the spline sub-command which matches */
  for (i = 0; i < N; i++) {
    if (!strcmp (spline_commands[i].name, argv[1])) {
      status = (*spline_commands[i].func) (argc - 1, argv + 1);
      return (status);
    }
  }

  gprint (GP_ERR, "unknown spline command %s\n", argv[1]);
  return (FALSE);
}

/* spline is called with the command "spline".  
   the command line word "spline" is meant to be followed the one of several 
   possible options:
   
   spline create
   spline delete
   spline list

*/

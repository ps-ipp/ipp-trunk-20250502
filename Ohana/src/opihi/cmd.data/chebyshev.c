# include "data.h"

int chebyshev_help (int argc, char **argv);
int chebyshev_list (int argc, char **argv);
int chebyshev_delete (int argc, char **argv);
int chebyshev_rename (int argc, char **argv);

int chebyshev_poly (int argc, char **argv);
int chebyshev_scale (int argc, char **argv);
int chebyshev_applyscale (int argc, char **argv);

int chebyshev_fit1d (int argc, char **argv);
int chebyshev_applyfit1d (int argc, char **argv);

int chebyshev_fit2d (int argc, char **argv);
int chebyshev_applyfit2d (int argc, char **argv);

// collection of chebyshev commands:
static Command chebyshev_commands[] = {
  {1, "help",       chebyshev_help,        "chebyshev help info"},
  {1, "list",       chebyshev_list,        "list define chebyshev polynomial sets"},
  {1, "delete",     chebyshev_delete,      "delete a chebyshev"},
  {1, "rename",     chebyshev_rename,      "rename a chebyshev"},

  {1, "poly",       chebyshev_poly,        "return evaluated chebyshev polynomial of given order for supplied coordinate vector"},

  {1, "scale",      chebyshev_scale,       "set domain scale based on supplied coordinate vector"},
  {1, "applyscale", chebyshev_applyscale,  "apply domain scale to supplied coordinate vector"},

  {1, "fit1d",      chebyshev_fit1d,       "fit chebyshev polynomial to supplied data vector"},
  {1, "applyfit1d", chebyshev_applyfit1d,  "apply chebyshev polynomial fit to supplied coordinate vector"},

  {1, "fit2d",      chebyshev_fit2d,       "fit chebyshev polynomial to supplied data vector"},
  {1, "applyfit2d", chebyshev_applyfit2d,  "apply chebyshev polynomial fit to supplied coordinate vector"},
};

int chebyshev_command (int argc, char **argv) {

  int N;

  if (argc < 2) {
    chebyshev_help(0,NULL);
    return (FALSE);
  }

  N = sizeof (chebyshev_commands) / sizeof (Command);

  /* find the chebyshev sub-command which matches */
  for (int i = 0; i < N; i++) {
    if (!strcmp (chebyshev_commands[i].name, argv[1])) {
      int status = (*chebyshev_commands[i].func) (argc - 1, argv + 1);
      return (status);
    }
  }

  gprint (GP_ERR, "unknown chebyshev command %s\n", argv[1]);
  return (FALSE);
}

/*** below are the utility commands which just manage the collection of chebyshevs ***/

int chebyshev_help (int argc, char **argv) {
  OHANA_UNUSED_PARAM(argc);
  OHANA_UNUSED_PARAM(argv);

  gprint (GP_ERR, "USAGE: chebyshev (command)\n");
  gprint (GP_ERR, "    chebyshev help                         : this listing\n");
  gprint (GP_ERR, "    chebyshev list                         : list chebyshevs\n");
  gprint (GP_ERR, "    chebyshev delete     (chebyshev)       : delete named chebyshev\n");
  gprint (GP_ERR, "    chebyshev rename     (chebyshev) (new) : change chebyshev name to new name\n");

  gprint (GP_ERR, "    chebyshev poly       (chebyshev) (new)    : change chebyshev name to new name\n");
  gprint (GP_ERR, "    chebyshev scale      (chebyshev) (new)    : change chebyshev name to new name\n");
  gprint (GP_ERR, "    chebyshev applyscale (chebyshev) (new)    : change chebyshev name to new name\n");

  gprint (GP_ERR, "    chebyshev fit1d      (chebyshev) (new)    : change chebyshev name to new name\n");
  gprint (GP_ERR, "    chebyshev applyfit1d (chebyshev) (new)    : change chebyshev name to new name\n");

  gprint (GP_ERR, "    chebyshev fit2d      (chebyshev) (new)    : change chebyshev name to new name\n");
  gprint (GP_ERR, "    chebyshev applyfit2d (chebyshev) (new)    : change chebyshev name to new name\n");

  return FALSE;
}

int chebyshev_list (int argc, char **argv) {
  OHANA_UNUSED_PARAM(argv);

  if (argc != 1) {
    gprint (GP_ERR, "USAGE: chebyshev list\n");
    return FALSE;
  }

  ListChebyshevs();
  return TRUE;
}

int chebyshev_delete (int argc, char **argv) {

  int N;

  int QUIET = FALSE;
  if ((N = get_argument (argc, argv, "-q"))) {
    QUIET = TRUE;
    remove_argument (N, &argc, argv);
  }

  if (argc != 2) {
    gprint (GP_ERR, "USAGE: chebyshev delete [-q] (chebyshev)\n");
    return FALSE;
  }

  ChebyshevType *chebyshev = FindChebyshev (argv[1]);
  if (chebyshev == NULL) {
    if (QUIET) return TRUE;
    gprint (GP_ERR, "chebyshev %s not found\n", argv[1]);
    return FALSE;
  }

  int status = DeleteChebyshev (chebyshev); 
  if (!status) abort (); // status = FALSE means the cheb selected above was not found
  return TRUE;
}

int chebyshev_rename (int argc, char **argv) {

  ChebyshevType *chebyshev;

  if (argc != 3) {
    gprint (GP_ERR, "USAGE: chebyshev rename (chebyshev) (newname)\n");
    return FALSE;
  }

  chebyshev = FindChebyshev (argv[1]);
  if (chebyshev == NULL) {
    gprint (GP_ERR, "chebyshev %s not found\n", argv[1]);
    return FALSE;
  }

  free (chebyshev->name);
  chebyshev->name = strcreate (argv[2]);
  return TRUE;
}

/* 

   typedef struct {
     char *name;
     opihi_flt scale;
     opihi_flt zero;
     int   order;
     opihi_flt *A;
   } ChebyshevType;


   examples of using the 

   cheb fit1d t1 x y 4     -- fit the vector y = f(x) using 4th order
    (saves the scaling for x, order, coeffs)
   cheb applyfit1d t1 x yfit  -- apply the fit defined in t1 to x
   cheb applyfit1d t1 X Yfit  -- apply the fit defined in t1 to a different vector X

   cheb fit t2 x y 2



 */


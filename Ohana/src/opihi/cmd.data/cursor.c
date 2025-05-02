# include "data.h"

int cursor (int argc, char **argv) {

  char string[20], key[20], *name;
  int i, N, kapa, VERBOSE;
  double X, Y, R, D, Z;

  // XXX need to be able to specify graph vs image coords
  // currently, if only one exists, that frame will be used
  // if both exist, defaults to ??
  // if ((N = get_argument (argc, argv, "-g"))) {
  // if ((N = get_argument (argc, argv, "-i"))) {

  VERBOSE = TRUE;
  if ((N = get_argument (argc, argv, "-a"))) {
    VERBOSE = FALSE;
  }

  name = NULL;
  if ((N = get_argument (argc, argv, "-n"))) {
    remove_argument (N, &argc, argv);
    name = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }
  if (!GetGraphdata (NULL, &kapa, name)) return (FALSE);
  FREE (name);

  N = 0;
  if (argc == 2) N = atof (argv[1]);

  if ((argc != 1) && (argc != 2)) {
    gprint (GP_ERR, "USAGE: cursor [Npts] [-n window] [-g | -i]\n");
    return (FALSE);
  }
  
  KiiCursorOn (kapa);
  
  struct sigaction *old_sigaction = SetInterrupt();
  for (i = 0; ((i < N) || (N == 0)) && !interrupt; i++) {

    KiiCursorRead (kapa, &X, &Y, &Z, &R, &D, key);

    snprintf_nowarn (string, 20, "X%s", key);    set_variable (string, X);
    snprintf_nowarn (string, 20, "Y%s", key);    set_variable (string, Y);
    snprintf_nowarn (string, 20, "Z%s", key);    set_variable (string, Z);
    snprintf_nowarn (string, 20, "R%s", key);    set_variable (string, R);
    snprintf_nowarn (string, 20, "D%s", key);    set_variable (string, D);

    set_str_variable ("KEY", key);
    
    if (VERBOSE) gprint (GP_LOG, "%s %f %f %f %f %f\n", key, X, Y, Z, R, D);

    if (!strcasecmp (key, "Q")) break;
  }
  ClearInterrupt (old_sigaction);

  KiiCursorOff (kapa);
  return (TRUE);
}

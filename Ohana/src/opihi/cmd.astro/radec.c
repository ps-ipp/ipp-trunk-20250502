# include "astro.h"

int radec (int argc, char **argv) {

  int N, QUIET;
  double ra, dec;
  char ra_string[32], dec_string[32];

  QUIET = FALSE;
  if ((N = get_argument (argc, argv, "-q"))) {
    QUIET = TRUE;
    remove_argument (N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "-quiet"))) {
    QUIET = TRUE;
    remove_argument (N, &argc, argv);
  }

  if (argc != 4) {
    gprint (GP_ERR, "USAGE:  radec (-hh | -hms) RA DEC [-q | -quiet]\n");
    gprint (GP_ERR, "   convert to decimal (-hh) or sexigesimal (-hms)\n");
    gprint (GP_ERR, "   output goes to $RA, $DEC\n");
    return (FALSE);
  }

  if (strcmp (argv[1], "-hh") && strcmp (argv[1], "-hms")) {
    gprint (GP_ERR, "USAGE:  radec (-hh | -hms) RA DEC [-q | -quiet]\n");
    gprint (GP_ERR, "   convert to decimal (-hh) or sexigesimal (-hms)\n");
    gprint (GP_ERR, "   output goes to $RA, $DEC\n");
    return (FALSE);
  }    

  if (!ohana_str_to_radec (&ra, &dec, argv[2], argv[3])) {
    return (FALSE);
  }

  if (!strcmp (argv[1], "-hh")) {
    set_variable ("RA", ra);
    set_variable ("DEC", dec);
    if (!QUIET) {
      gprint (GP_LOG, "%10.6f %10.6f\n", ra, dec);
    }
    return (TRUE);
  }

  // convert to hms, dms
  hms_format (ra_string, 32, ra/15.0);
  hms_format (dec_string, 32, dec);

  set_str_variable ("RA", ra_string);
  set_str_variable ("DEC", dec_string);
  if (!QUIET) {
    gprint (GP_LOG, "%s %s\n", ra_string, dec_string);
  }
  return (TRUE);
}  

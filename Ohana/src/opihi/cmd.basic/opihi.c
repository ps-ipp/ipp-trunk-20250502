# include "basic.h"

int opihi_setmode (int argc, char **argv) {

  OpihiVerboseMode value;

  if (argc < 2) {
    gprint (GP_ERR, "USAGE: opihi (verbose) [on/off/error]\n");
    return (FALSE);
  }

  if (!strcasecmp(argv[1], "verbose")) {
      if (argc == 2) {
	  value = get_verbose_shell();
	  switch (value) {
	    case OPIHI_VERBOSE_OFF:
	      gprint (GP_ERR, "opihi verbose mode: off\n");
	      break;
	    case OPIHI_VERBOSE_ON:
	      gprint (GP_ERR, "opihi verbose mode: on\n");
	      break;
	    case OPIHI_VERBOSE_ERROR:
	      gprint (GP_ERR, "opihi verbose mode: error\n");
	      break;
	    default:
	      fprintf (stderr, "impossible condition\n");
	      abort();
	  } 
	  return (TRUE);
      }
      if (!strcasecmp(argv[2], "on")) {
	set_verbose_shell (OPIHI_VERBOSE_ON);
	return (TRUE);
      }
      if (!strcasecmp(argv[2], "off")) {
	set_verbose_shell (OPIHI_VERBOSE_OFF);
	return (TRUE);
      }
      if (!strcasecmp(argv[2], "error")) {
	set_verbose_shell (OPIHI_VERBOSE_ERROR);
	return (TRUE);
      }
      gprint (GP_ERR, "unknown verbose mode %s\n", argv[2]);
      return (FALSE);
  }

  gprint (GP_ERR, "unknown mode %s\n", argv[1]);
  return (FALSE);
}

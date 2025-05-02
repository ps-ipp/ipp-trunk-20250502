# include "pcontrol.h"

int parameters (int argc, char **argv) {

  int ivalue;
  float value;

  if (argc < 2) goto usage;
  if (argc > 4) goto usage;
  if (argc == 3) goto usage;
  if ((argc == 4) && strcmp(argv[2], "=")) goto usage;

  if (!strncasecmp (argv[1], "connect_time", strlen(argv[1]))) {
    if (argc == 2) {
      value = GetMaxConnectTime();
      gprint (GP_LOG, "Max Connect Time : %f\n", value);
      return (TRUE);
    }

    value = atof(argv[3]);
    SetMaxConnectTime(value);
    return (TRUE);
  }

  if (!strncasecmp (argv[1], "wanthost_wait", strlen(argv[1]))) {
    if (argc == 2) {
      value = GetMaxWantHostWait();
      gprint (GP_LOG, "Max WantHost Wait : %f\n", value);
      return (TRUE);
    }

    value = atof(argv[3]);
    SetMaxWantHostWait(value);
    return (TRUE);
  }

  if (!strncasecmp (argv[1], "unwanted_host_jobs", strlen(argv[1]))) {
    if (argc == 2) {
      ivalue = GetMaxUnwantedHostJobs();
      gprint (GP_LOG, "Max Unwanted Host Jobs : %d\n", ivalue);
      return (TRUE);
    }

    ivalue = atof(argv[3]);
    SetMaxUnwantedHostJobs(ivalue);
    return (TRUE);
  }

  usage:

  gprint (GP_LOG, "USAGE: parameters (param) [= value])\n");
  gprint (GP_LOG, "  valid parameters: connect_time, wanthost_wait, unwanted_host_jobs\n");
  gprint (GP_LOG, "  (minimum matching word is allowed)\n");
  gprint (GP_LOG, "  example: parameters connect = 2.0\n");
  return (FALSE);
}


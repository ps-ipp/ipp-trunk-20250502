# include "opihi.h"

static char *GlobalConfig = NULL;

// this function is only called at start, so it is not thread protect
int ConfigInit (int *argc, char **argv) {

  char *config, *file;

  /*** load configuration info ***/
  file = SelectConfigFile (argc, argv, "ptolemy");
  config = LoadConfigFile (file);
  if (config == (char *) NULL) {
    if (file != (char *) NULL) free (file);
    return (FALSE);
  }

  GlobalConfig = config;

  free (file);
  return (TRUE);
}

// this function is only called at shutdown, so it is not thread protect
void ConfigFree () {
  if (GlobalConfig) free (GlobalConfig);
  return;
}

char *VarConfig (char *keyword, char *mode, void *ptr) {

  char *answer;

  answer = get_variable (keyword);
  if (answer == (char *) NULL) {
    answer = ScanConfig (GlobalConfig, keyword, mode, 0, ptr);
    return (answer);
  }

  // XXX this is a bit dangerous : answer may have an arbitrary
  // length, but we do not know the available space in 'ptr'
  // we really should be passing in the buffer size and limiting the copy


  if (!strcmp (mode, "%s"))  strcpy ((char *) ptr, answer);
  if (!strcmp (mode, "%d"))  *(int *) ptr       = atoi (answer);
  if (!strcmp (mode, "%u"))  *(unsigned *) ptr  = atoi (answer);
  if (!strcmp (mode, "%ld")) *(long *) ptr      = atoi (answer);
  if (!strcmp (mode, "%hd")) *(short *) ptr     = atoi (answer);
  if (!strcmp (mode, "%f"))  *(float *) ptr     = atof (answer);
  if (!strcmp (mode, "%lf")) *(double *) ptr    = atof (answer);

  free (answer);
  return (ptr);
}

char *VarConfigEntry (char *keyword, char *mode, int entry, void *ptr) {

  char *answer;

  answer = get_variable (keyword);
  if (answer == (char *) NULL) {
    answer = ScanConfig (GlobalConfig, keyword, mode, entry, ptr);
    return (answer);
  }

  if (!strcmp (mode, "%s"))  strcpy ((char *) ptr, answer);
  if (!strcmp (mode, "%d"))  *(int *) ptr       = atoi (answer);
  if (!strcmp (mode, "%u"))  *(unsigned *) ptr  = atoi (answer);
  if (!strcmp (mode, "%ld")) *(long *) ptr      = atoi (answer);
  if (!strcmp (mode, "%hd")) *(short *) ptr     = atoi (answer);
  if (!strcmp (mode, "%f"))  *(float *) ptr     = atof (answer);
  if (!strcmp (mode, "%lf")) *(double *) ptr    = atof (answer);

  free (answer);
  return (ptr);
}


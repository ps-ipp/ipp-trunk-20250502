# include "imregister.h"
enum {NONE, IMPORT, EXPORT};

char CatdirPhotcodeFile[256];
char MasterPhotcodeFile[256];

int args (int argc, char **argv);
int ConfigInitLocal (int *argc, char **argv);
void GetConfig (char *config, char *field, char *format, int N, void *ptr);
void usage ();

int main (int argc, char **argv) {

  int mode;

  ConfigInitLocal (&argc, argv);
  mode = args (argc, argv);
    
  if (mode == IMPORT) {
    LoadPhotcodesText (MasterPhotcodeFile);
    SavePhotcodesFITS (CatdirPhotcodeFile);
    exit (0);
  }

  if (mode == EXPORT) {
    LoadPhotcodesFITS (CatdirPhotcodeFile);
    SavePhotcodesText (MasterPhotcodeFile);
    exit (0);
  }

  usage ();
  exit (1);
}

int args (int argc, char **argv) {

  int N, mode;

  /* check for help request */
  if (get_argument (argc, argv, "-help")) usage ();
  if (get_argument (argc, argv, "-h")) usage ();

  mode = NONE;
  if ((N = get_argument (argc, argv, "-import"))) {
    mode = IMPORT;
    remove_argument (N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "-export"))) {
    mode = EXPORT;
    remove_argument (N, &argc, argv);
  }

  if (argc != 2) usage ();

  strcpy (MasterPhotcodeFile, argv[1]);
  return (mode);
}

int ConfigInitLocal (int *argc, char **argv) {

  char *config, *file;
  char CATDIR[MY_MAX_PATH];

  /*** load configuration info ***/
  file = SelectConfigFile (argc, argv, "ptolemy");
  config = LoadConfigFile (file);
  if (config == (char *) NULL) {
    fprintf (stderr, "ERROR: can't find configuration file %s\n", file);
    if (file != (char *) NULL) free (file);
    exit (1);
  }
  // if (VERBOSE) fprintf (stderr, "loaded config file: %s\n", file);

  GetConfig (config, "CATDIR",                 	"%s",  0, CATDIR);
  // GetConfig (config, "PHOTCODE_FILE",          	"%s",  0, MasterPhotcodeFile);
  
  // set the CATDIR version based on CATDIR
  snprintf_nowarn (CatdirPhotcodeFile, MY_MAX_PATH, "%s/Photcodes.dat", CATDIR);

  free (config);
  free (file);
  return TRUE;
}

void GetConfig (char *config, char *field, char *format, int N, void *ptr) {

  char *status;

  status = ScanConfig (config, field, format, N, ptr);
  if (status == NULL) {
    fprintf (stderr, "error in config, cannot find %s\n", field);
    exit (1);
  }
  return;
}

void usage () {

  fprintf (stderr, "USAGE: photcode-table -export (textfile) [-D CATDIR catdir]\n");
  fprintf (stderr, "USAGE: photcode-table -import (textfile) [-D CATDIR catdir]\n");
  exit (2);
}

# include "imregister.h"
# include "detrend.h"

static char *dBFile  = NULL;
static char *dBPath  = NULL;
static char *dBTrash = NULL;

/*** these are functions to handle the special detrend.db file/trash names ****/

char *set_dBFile () {

  struct stat statbuf;
  int status;

  dBPath = DetrendDB;

  /* define dBFile based on config data */
  ALLOCATE (dBFile, char, strlen (dBPath) + 15);
  sprintf (dBFile, "%s/detrend.db", dBPath);;
  ALLOCATE (dBTrash, char, strlen (dBPath) + 15);
  sprintf (dBTrash, "%s/trash", dBPath);

  /* check on directories */
  status = stat (dBPath, &statbuf);
  if (output.Modify) {
    if (status == -1) {
      if (mkdirhier (dBPath, S_IRWXU | S_IRWXG | S_IRWXO) == -1) {
	fprintf (stderr, "ERROR: can't find or create path %s\n", dBPath);
	exit (1);
      }
    }
  }
  if (output.Delete) {
    status = stat (dBTrash, &statbuf);
    if (status == -1) {
      if (mkdirhier (dBTrash, S_IRWXU | S_IRWXG | S_IRWXO) == -1) {
	fprintf (stderr, "ERROR: detrend dB trash not found %s\n", dBTrash);
	exit (1);
      }
    }
  }
  return (dBFile);
}

char *get_dBPath () {

  return (dBPath);

}

int delete_image (DetReg *item) {
  
  int status;
  char line[512];

  if (output.verbose) fprintf (stderr, "deleting %s\n", item[0].filename);
  snprintf (line, 512, "mv -f %s/%s %s", dBPath, item[0].filename, dBTrash);
  status = system (line);
  if (status) fprintf (stderr, "trouble moving %s to trash (%s)\n", item[0].filename, dBTrash);

  if (status) 
    return (FALSE);
  else 
    return (TRUE);
}


# include "dvoshell.h"
int gcat_listnames (SkyList *skylist, HostTable *table, int ShowFile, int ShowHost, int ShowBackup, int ShowFlags, int ShowCoords, int ShowID);

int catname (int argc, char **argv) {
  
  int N;

  int ShowCoords = FALSE;
  if ((N = get_argument (argc, argv, "-coords"))) {
    remove_argument (N, &argc, argv);
    ShowCoords = TRUE;
  }
  int ShowID = FALSE;
  if ((N = get_argument (argc, argv, "-catid"))) {
    remove_argument (N, &argc, argv);
    ShowID = TRUE;
  }
  int ShowFile = FALSE;
  if ((N = get_argument (argc, argv, "-file"))) {
    remove_argument (N, &argc, argv);
    ShowFile = TRUE;
  }
  int ShowHost = FALSE;
  if ((N = get_argument (argc, argv, "-host"))) {
    remove_argument (N, &argc, argv);
    ShowHost = TRUE;
  }
  int ShowBackup = FALSE;
  if ((N = get_argument (argc, argv, "-backup"))) {
    remove_argument (N, &argc, argv);
    ShowBackup = TRUE;
  }
  int ShowFlags = FALSE;
  if ((N = get_argument (argc, argv, "-flags"))) {
    remove_argument (N, &argc, argv);
    ShowFlags = TRUE;
  }

  if (argc != 2) {
    gprint (GP_ERR, "USAGE: catname name [-host] [-backup] [-flags] [-file] [-coords] [-catid]\n");
    return (FALSE);
  }

  /* load sky from correct table */
  char *CATDIR = GetCATDIR();
  if (!CATDIR) {
    gprint (GP_ERR, "CATDIR is not set\n");
    return FALSE;
  }
  SkyTable *sky = GetSkyTable ();
  if (!sky) {
    gprint (GP_ERR, "failed to load sky table for database\n");
    return FALSE;
  }
  HostTable *table = NULL;  
  if (HostTableExists (CATDIR, sky->hosts)) {
    table = HostTableLoad (CATDIR, sky->hosts);
    if (!table) {
      gprint (GP_ERR, "ERROR: failure reading Host Table %s for parallel database %s\n", sky->hosts, CATDIR);
      return FALSE;
    }    
  }

  SkyList *skylist = SkyListByName (sky, argv[1]);

  gcat_listnames (skylist, table, ShowFile, ShowHost, ShowBackup, ShowFlags, ShowCoords, ShowID);
  SkyListFree (skylist);

  return (TRUE);
}

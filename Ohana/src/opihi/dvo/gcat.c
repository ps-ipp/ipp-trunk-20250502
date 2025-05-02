# include "dvoshell.h"
int gcat_listnames (SkyList *skylist, HostTable *table, int ShowFile, int ShowHost, int ShowBackup, int ShowFlags, int ShowCoords, int ShowID);

int gcat (int argc, char **argv) {
  
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

  if ((argc != 3) && (argc != 4)) {
    gprint (GP_ERR, "USAGE: gcat RA DEC [Radius] [-host] [-backup] [-flags] [-file] [-coords] [-catid]\n");
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

  double Ra = atof (argv[1]);
  double Dec = atof (argv[2]);
  double Radius = (argc == 4) ? atof(argv[3]) : 0.0001;

  SkyList *skylist = SkyListByRadius (sky, -1, Ra, Dec, Radius);

  gcat_listnames (skylist, table, ShowFile, ShowHost, ShowBackup, ShowFlags, ShowCoords, ShowID);
  SkyListFree (skylist);

  return (TRUE);
}

int gcat_listnames (SkyList *skylist, HostTable *table, int ShowFile, int ShowHost, int ShowBackup, int ShowFlags, int ShowCoords, int ShowID) {

  int i;
  struct stat filestat;

  // prepare to handle interrupt signals
  struct sigaction *old_sigaction = SetInterrupt();

  for (i = 0; (i < skylist[0].Nregions) && !interrupt; i++) {
    SkyRegion *region = skylist[0].regions[i];

    int isLast = (i == skylist[0].Nregions - 1);

    char hostfile[1024];
    if (table) {
      int hostID = (region->hostFlags & DATA_USE_BCK) ? region->backupID : region->hostID;
      int index = table->index[hostID];
      snprintf (hostfile, 1024, "%s/%s.cpt", table->hosts[index].pathname, region->name);
    } else {
      strcpy (hostfile, skylist[0].filename[i]);
    }

    if (ShowFile) {
      gprint (GP_ERR, "%3d %s", i, hostfile);
    } else {
      gprint (GP_ERR, "%3d %s", i, region->name);
    }

    if (ShowCoords) {
      gprint (GP_ERR, " %7.3f %7.3f : %7.3f %7.3f", region->Rmin, region->Rmax, region->Dmin, region->Dmax);
    } else {
      gprint (GP_ERR, " %33s", " ");
    }

    if (stat (hostfile, &filestat) != -1) {
      gprint (GP_ERR, " +");
    } else {
      gprint (GP_ERR, " -");
    } 
    if (ShowID) {
      gprint (GP_ERR, " %5d", region->index);
    } else {
      gprint (GP_ERR, "%6s", " ");
    }
    if (ShowHost) {
      gprint (GP_ERR, "  %3d", region->hostID);
    } else {
      gprint (GP_ERR, "     ");
    }
    if (ShowBackup) {
      gprint (GP_ERR, "  %3d", region->backupID);
    } else {
      gprint (GP_ERR, "     ");
    }
    if (ShowFlags) {
      gprint (GP_ERR, "  0x%04x", region->hostFlags);
    } else {
      gprint (GP_ERR, "        ");
    }
    gprint (GP_ERR, "\n");

    if (isLast) {
      set_variable ("CPT_RMIN", region->Rmin);
      set_variable ("CPT_RMAX", region->Rmax);
      set_variable ("CPT_DMIN", region->Dmin);
      set_variable ("CPT_DMAX", region->Dmax);
      set_variable ("CPT_CATID", region->index);
      set_str_variable ("CATNAME", hostfile);
    }
  }

  ClearInterrupt (old_sigaction);
  return (TRUE);
}

# include "dvoshell.h"

// return some stats on the specified catalog files
int catstats (int argc, char **argv) {
  
  int i, N;
  struct stat filestat;

  int VERBOSE = FALSE;
  if ((N = get_argument (argc, argv, "-v"))) {
    remove_argument (N, &argc, argv);
    VERBOSE = TRUE;
  }
  int ShowAll = FALSE;
  if ((N = get_argument (argc, argv, "-all"))) {
    remove_argument (N, &argc, argv);
    ShowAll = TRUE;
  }
  int Depth = -1;
  if ((N = get_argument (argc, argv, "-depth"))) {
    remove_argument (N, &argc, argv);
    Depth = atoi (argv[N]);
    remove_argument (N, &argc, argv);    
  }
  int ThisHost = FALSE;
  if ((N = get_argument (argc, argv, "-this-host"))) {
    remove_argument (N, &argc, argv);
    ThisHost = TRUE;
  }

  // use remote tables, but not dvo_client..
  int PARALLEL_LOCAL = FALSE;
  HostTable *table = NULL;
  if ((N = get_argument (argc, argv, "-parallel-local"))) {
    remove_argument (N, &argc, argv);
    PARALLEL_LOCAL = TRUE;
  }

  if (argc != 5) {
    gprint (GP_ERR, "USAGE: catstats (name)\n");
    return (FALSE);
  }

  SkyTable *sky = GetSkyTable ();
  SkyList *skylist = SkyListByBounds (sky, Depth, Rmin, Rmax, Dmin, Dmax);
  SkyRegion **regions = skylist[0].regions;


  if (PARALLEL_LOCAL) {
    char *CATDIR = GetCATDIR();
    if (!CATDIR) {
      gprint (GP_ERR, "CATDIR is not set\n");
      return FALSE;
    }
    table = HostTableLoad (CATDIR, sky->hosts);
    if (!table) {
      gprint (GP_ERR, "ERROR: failure reading Host Table %s for database %s\n", sky->hosts, CATDIR);
      return FALSE;
    }    
  }

  int Nregion = 0;
  for (i = 0; i < skylist[0].Nregions; i++) {

    // skip tables that are not on this host (if -this-host supplied)
    if (ThisHost && !HostTableTestHost (regions[i], HOST_ID)) continue;

    if (PARALLEL_LOCAL) {
      int hostID = (skylist[0].regions[i]->hostFlags & DATA_USE_BCK) ? skylist[0].regions[i]->backupID : skylist[0].regions[i]->hostID;
      int seq = table->index[hostID];
      HOSTDIR = table->hosts[seq].pathname;
    }
    char hostfile[1024];
    snprintf (hostfile, 1024, "%s/%s.cpt", HOSTDIR, skylist[0].regions[i]->name);
    char *filename = (HOST_ID || PARALLEL_LOCAL) ? hostfile : skylist[0].filename[i];

    if (ShowAll || (stat (filename, &filestat) != -1)) {
      if (VERBOSE) gprint (GP_ERR, "%3d %s %6.2f - %6.2f, %6.2f - %6.2f\n", i, regions[i][0].name, 
			   regions[i][0].Rmin, regions[i][0].Rmax, regions[i][0].Dmin, regions[i][0].Dmax);
      
      char name[64];
      snprintf (name, 64, "region:%d", Nregion);
      set_str_variable (name, regions[i][0].name);
      Nregion ++;
    }
  }
  set_int_variable ("region:n", Nregion);

  if (table) free (table);
  return (TRUE);
}

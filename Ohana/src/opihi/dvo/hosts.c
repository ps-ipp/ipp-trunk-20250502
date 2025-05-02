# include "dvoshell.h"
# include <glob.h>
# define DVO_MAX_PATH 1024

enum {TEMP_NONE, TEMP_DVO_RESULTS, TEMP_DVO_LOG, 
      TEMP_RELASTRO_CATALOG, TEMP_RELASTRO_CAT_SUBSET, 
      TEMP_RELPHOT_CATALOG, TEMP_RELPHOT_CAT_SUBSET, TEMP_RELPHOT_LOG, 
      TEMP_DVOPSPS_DET, TEMP_FIXSTKIDS_RESULTS, TEMP_CHECKASTRO_CATALOG,
};

// functions to manage the remote hosts
int hosts (int argc, char **argv) {
  
  if (argc < 2) {
    gprint (GP_ERR, "USAGE: hosts (command) [options]\n");
    gprint (GP_ERR, "  commands:\n");
    gprint (GP_ERR, "    purge-temp : delete all tempfiles for this shell\n");
    gprint (GP_ERR, "               : [-old-pid] [-all-pid] [-v] [-verbose] [-commit] [-type type] [-age hours]\n");
    gprint (GP_ERR, "    get-results : determine name of RESULTS file\n");
    return FALSE;
  }

  if (!strncmp(argv[1], "purge-temp", MAX(strlen(argv[1]), 3))) {
    glob_t pglob;
    int ALL_PID = FALSE;
    int PID = getpid();
    int N;
    if ((N = get_argument (argc, argv, "-old-pid"))) {
      remove_argument (N, &argc, argv);
      PID = atoi(argv[N]);
      remove_argument (N, &argc, argv);
    }

    if ((N = get_argument (argc, argv, "-all-pid"))) {
      remove_argument (N, &argc, argv);
      ALL_PID = TRUE;
    }
    
    struct timeval now;
    gettimeofday (&now, NULL);

    float AGE = 0;
    int NOW = now.tv_sec;
    if ((N = get_argument (argc, argv, "-age"))) {
      remove_argument (N, &argc, argv);
      AGE = atof(argv[N]);
      remove_argument (N, &argc, argv);
    }

    int VERBOSE = FALSE;
    if ((N = get_argument (argc, argv, "-v"))) {
      remove_argument (N, &argc, argv);
      VERBOSE = TRUE;
    }
    if ((N = get_argument (argc, argv, "-verbose"))) {
      remove_argument (N, &argc, argv);
      VERBOSE = TRUE;
    }
    
    int DRYRUN = TRUE;
    if ((N = get_argument (argc, argv, "-commit"))) {
      remove_argument (N, &argc, argv);
      DRYRUN = FALSE;
    }
    
    // we have the following types of temp files:
    // dvo.results.*.fits
    // dvo.results.*.fits.log
    // log.rlpc.* (relphot logs)
    // relphot.catalog.subset.dat [no glob needed]
    int TEMP_TYPE = TEMP_NONE;
    if ((N = get_argument (argc, argv, "-type"))) {
      remove_argument (N, &argc, argv);
      if (!strcasecmp(argv[N], "dvo.results"))        	     TEMP_TYPE = TEMP_DVO_RESULTS;
      if (!strcasecmp(argv[N], "dvo.log"))            	     TEMP_TYPE = TEMP_DVO_LOG;
      if (!strcasecmp(argv[N], "relphot.catalog"))    	     TEMP_TYPE = TEMP_RELPHOT_CATALOG;
      if (!strcasecmp(argv[N], "relphot.catalog.subset"))    TEMP_TYPE = TEMP_RELPHOT_CAT_SUBSET;
      if (!strcasecmp(argv[N], "relastro.catalog"))   	     TEMP_TYPE = TEMP_RELASTRO_CATALOG;
      if (!strcasecmp(argv[N], "relastro.catalog.subset"))   TEMP_TYPE = TEMP_RELASTRO_CAT_SUBSET;
      if (!strcasecmp(argv[N], "checkastro.catalog"))        TEMP_TYPE = TEMP_CHECKASTRO_CATALOG;
      if (!strcasecmp(argv[N], "relphot.log"))        	     TEMP_TYPE = TEMP_RELPHOT_LOG;
      if (!strcasecmp(argv[N], "dvopsps.det"))        	     TEMP_TYPE = TEMP_DVOPSPS_DET;
      if (!strcasecmp(argv[N], "fixstkids.results"))  	     TEMP_TYPE = TEMP_FIXSTKIDS_RESULTS;
      remove_argument (N, &argc, argv);
    }
    if (TEMP_TYPE == TEMP_NONE) {
      gprint (GP_ERR, "USAGE: hosts purge-temp [-type (type)]\n");
      gprint (GP_ERR, "  allowed types: dvo.results, dvo.log\n");
      gprint (GP_ERR, "    relphot.catalog, relphot.catalog.subset, relphot.log\n");
      gprint (GP_ERR, "    relastro.catalog, relastro.catalog.subset\n");
      gprint (GP_ERR, "    checkastro.catalog, dvopsps.det, fixstkids.results\n");
      return FALSE;
    }

    // XXX wrap this up in a function:
    char *CATDIR = GetCATDIR();
    if (!CATDIR) {
      gprint (GP_ERR, "CATDIR is not set\n");
      return FALSE;
    }
    SkyTable *sky = GetSkyTable();
    if (!sky) {
      gprint (GP_ERR, "failed to load sky table for database\n");
      return FALSE;
    }
    HostTable *table = HostTableLoad (CATDIR, sky->hosts);
    if (!table) {
      gprint (GP_ERR, "ERROR: failure reading Host Table %s for database %s\n", sky->hosts, CATDIR);
      return FALSE;
    }    
    
    if (argc != 2) {
      gprint (GP_ERR, "USAGE: hosts purge-temp -type type [options] : delete all tempfiles for this shell\n");
      gprint (GP_ERR, "   [-old-pid] [-all-pid] [-v] [-verbose]\n");
      gprint (GP_ERR, "    [-commit] [-age hours]\n");
      gprint (GP_ERR, "current arguments: ");
      for (int i = 0; i < argc; i++) { gprint (GP_ERR, "%s ", argv[i]); }
      gprint (GP_ERR, "\n");
      return FALSE;
    }

    int i;
    for (i = 0; i < table->Nhosts; i++) {
      if (HOST_ID && (HOST_ID != table->hosts[i].hostID)) continue;

      pglob.gl_offs = 0;
      char name[DVO_MAX_PATH];
      if (ALL_PID) {
	if (TEMP_TYPE == TEMP_DVO_RESULTS)     	   snprintf (name, DVO_MAX_PATH, "%s/dvo.results.*.fits", table->hosts[i].pathname);
	if (TEMP_TYPE == TEMP_DVO_LOG)         	   snprintf (name, DVO_MAX_PATH, "%s/dvo.results.*.fits.log", table->hosts[i].pathname);
	if (TEMP_TYPE == TEMP_RELPHOT_CATALOG)	   snprintf (name, DVO_MAX_PATH, "%s/relphot.catalog.?????.?????.dat", table->hosts[i].pathname);
	if (TEMP_TYPE == TEMP_RELPHOT_CAT_SUBSET)  snprintf (name, DVO_MAX_PATH, "%s/relphot.catalog.subset.dat", table->hosts[i].pathname);
	if (TEMP_TYPE == TEMP_RELPHOT_LOG)     	   snprintf (name, DVO_MAX_PATH, "%s/log.rlpc.*", table->hosts[i].pathname);
	if (TEMP_TYPE == TEMP_RELASTRO_CATALOG)    snprintf (name, DVO_MAX_PATH, "%s/relastro.catalog.?????.?????.dat", table->hosts[i].pathname);
	if (TEMP_TYPE == TEMP_RELASTRO_CAT_SUBSET) snprintf (name, DVO_MAX_PATH, "%s/relastro.catalog.subset.dat", table->hosts[i].pathname);
	if (TEMP_TYPE == TEMP_CHECKASTRO_CATALOG)  snprintf (name, DVO_MAX_PATH, "%s/checkastro.catalog.?????.?????.dat", table->hosts[i].pathname);
	if (TEMP_TYPE == TEMP_DVOPSPS_DET)     	   snprintf (name, DVO_MAX_PATH, "%s/dvopsps.*.det.dat", table->hosts[i].pathname);
	if (TEMP_TYPE == TEMP_FIXSTKIDS_RESULTS)   snprintf (name, DVO_MAX_PATH, "%s/fixstkids.results.*.dat", table->hosts[i].pathname);
      } else {
	if (TEMP_TYPE == TEMP_DVO_RESULTS)     	   snprintf (name, DVO_MAX_PATH, "%s/dvo.results.%05d.*.fits", table->hosts[i].pathname, PID);
	if (TEMP_TYPE == TEMP_DVO_LOG)         	   snprintf (name, DVO_MAX_PATH, "%s/dvo.results.%05d.*.fits.log", table->hosts[i].pathname, PID);
	if (TEMP_TYPE == TEMP_RELPHOT_CATALOG)	   snprintf (name, DVO_MAX_PATH, "%s/relphot.catalog.%05d.?????.dat", table->hosts[i].pathname, PID);
	if (TEMP_TYPE == TEMP_RELPHOT_CAT_SUBSET)  snprintf (name, DVO_MAX_PATH, "%s/relphot.catalog.subset.dat", table->hosts[i].pathname);
	if (TEMP_TYPE == TEMP_RELPHOT_LOG)     	   snprintf (name, DVO_MAX_PATH, "%s/log.rlpc.*", table->hosts[i].pathname);
	if (TEMP_TYPE == TEMP_RELASTRO_CATALOG)    snprintf (name, DVO_MAX_PATH, "%s/relastro.catalog.%05d.?????.dat", table->hosts[i].pathname, PID);
	if (TEMP_TYPE == TEMP_RELASTRO_CAT_SUBSET) snprintf (name, DVO_MAX_PATH, "%s/relastro.catalog.subset.dat", table->hosts[i].pathname);
	if (TEMP_TYPE == TEMP_CHECKASTRO_CATALOG)  snprintf (name, DVO_MAX_PATH, "%s/checkastro.catalog.%05d.?????.dat", table->hosts[i].pathname, PID);
	if (TEMP_TYPE == TEMP_DVOPSPS_DET)     	   snprintf (name, DVO_MAX_PATH, "%s/dvopsps.%05d.*.det.dat", table->hosts[i].pathname, PID);
	if (TEMP_TYPE == TEMP_FIXSTKIDS_RESULTS)   snprintf (name, DVO_MAX_PATH, "%s/fixstkids.results.%05d.*.dat", table->hosts[i].pathname, PID);
      }
      if (VERBOSE) gprint (GP_ERR, "checking %s\n", name);
      glob (name, 0, NULL, &pglob);
      int j;
      struct stat filestats;
      for (j = 0; j < pglob.gl_pathc; j++) {
	if (AGE > 0) {
	  if (stat(pglob.gl_pathv[j], &filestats)) {
	    gprint (GP_ERR, "failed to get stats for %s\n", pglob.gl_pathv[j]);
	    continue;
	  }
	  float myAge = (NOW - filestats.st_mtime) / 3600.0;
	  if (myAge < AGE) continue;
	}
	if (VERBOSE) gprint (GP_ERR, "unlink %s\n", pglob.gl_pathv[j]);
	if (!DRYRUN) unlink (pglob.gl_pathv[j]);
      }
      globfree (&pglob);
    }
    return TRUE;
  }

  if (!strncmp(argv[1], "get.results", MAX(strlen(argv[1]), 3))) {
    int N;

    char *varname = NULL;
    if ((N = get_argument (argc, argv, "-var"))) {
      remove_argument (N, &argc, argv);
      varname = strcreate(argv[N]);
      remove_argument (N, &argc, argv);
    }

    if (argc < 2) {
      gprint (GP_ERR, "USAGE: hosts get.results [-var var]\n");
      return FALSE;
    }

    if (varname) {
      set_str_variable (varname, RESULT_FILE);
      free (varname);
    } else {
      gprint (GP_LOG, "results: %s\n", RESULT_FILE);
    }
    return TRUE; 
  }

  gprint (GP_ERR, "error: unknown hosts command %s\n", argv[1]);
  return FALSE;
}

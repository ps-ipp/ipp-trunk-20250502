# include "sedstar.h"

int main (int argc, char **argv) {

  char *root, *ext, tmp;
  int i, Nbytes;
  SkyList *skylist;
  SkyTable *sky;
  AddstarClientOptions options;
  Catalog incatalog, outcatalog;
  SEDtable *sedtable;

  // need to construct these options with args_load2mass...
  SetSignals ();
  options = ConfigInit (&argc, argv);
  options = args_sedstar (argc, argv, options);

  sky = SkyTableLoadOptimal (CATDIR, SKY_TABLE, GSCFILE, TRUE, SKY_DEPTH, VERBOSE);
  SkyTableSetFilenames (sky, CATDIR, "cpt");
  
  // select regions of interest
  skylist = SkyListByPatch (sky, -1, &UserPatch);

  // load the SED data table
  sedtable = SEDtableLoad (argv[1]);

  for (i = 0; i < skylist[0].Nregions; i++) {
    dvo_catalog_init (&incatalog, TRUE);
    incatalog.filename = skylist[0].filename[i];
    incatalog.catflags = DVO_LOAD_AVERAGE | DVO_LOAD_MEASURE | DVO_LOAD_SECFILT;

    // an error exit status here is a significant error
    if (!dvo_catalog_open (&incatalog, skylist[0].regions[i], VERBOSE, "r")) {
      fprintf (stderr, "ERROR: failure to open/create catalog file %s\n", incatalog.filename);
      exit (2);
    }

    // Naverage_disk == 0 implies an empty catalog file
    if ((incatalog.Naverage_disk == 0) && options.only_match) {
      if (VERBOSE) fprintf (stderr, "skipping empty region\n");
      dvo_catalog_unlock (&incatalog);
      dvo_catalog_free (&incatalog);
      continue;
    }

    // create output catalog filename
    root = strstr (incatalog.filename, CATDIR);
    if (root == NULL) Shutdown ("error with input catalog name");
    ext = incatalog.filename + strlen(CATDIR);
    while (*ext == '/') ext++;
    Nbytes = snprintf (&tmp, 0, "%s/%s", argv[2], ext);
    ALLOCATE (outcatalog.filename, char, Nbytes + 1);
    snprintf (outcatalog.filename, Nbytes + 1, "%s/%s", argv[2], ext);

    outcatalog.catformat = dvo_catalog_catformat (CATFORMAT);  // set the default catformat from config data
    outcatalog.catmode   = dvo_catalog_catmode (CATMODE);      // set the default catmode from config data
    outcatalog.Nsecfilt  = GetPhotcodeNsecfilt ();
    outcatalog.catflags = DVO_LOAD_AVERAGE | DVO_LOAD_MEASURE | DVO_LOAD_MISSING | DVO_LOAD_SECFILT;

    // an error exit status here is a significant error
    if (!dvo_catalog_open (&outcatalog, skylist[0].regions[i], VERBOSE, "w")) {
      Shutdown ("ERROR: failure to open/create catalog file %s\n", outcatalog.filename);
    }

    SEDfitCatalog (&outcatalog, &incatalog, sedtable);
    
    SetProtect (TRUE);
    if (!dvo_catalog_save (&outcatalog, VERBOSE)) { fprintf (stderr, "ERROR: failed to save %s\n", outcatalog.filename); exit (1); }
    if (!dvo_catalog_unlock (&outcatalog)) { fprintf (stderr, "ERROR: failed to unlock %s\n", outcatalog.filename); exit (1); }
    SetProtect (FALSE);
    dvo_catalog_free (&outcatalog);

    dvo_catalog_unlock (&incatalog);
    dvo_catalog_free (&incatalog);
    // XXX free filename or not?
  }
  exit (0);
}  

/**  sedstar: 

* load in the SED data table
* load in the catalog file (by region)
* fit stars in the catalog file
* load output catalog file?
* construct output catalog file (optional)
* save output catalog file

**/

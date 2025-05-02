# include "dvosplit.h"

Catalog *open_output_catalogs (SkyList *outlist, int catformat, int catmode) {

  int i;
  Catalog *outcatalogs;

  ALLOCATE (outcatalogs, Catalog, outlist[0].Nregions);

  // an error exit status here is a significant error
  for (i = 0; i < outlist[0].Nregions; i++) {
    
    // set the parameters which guide catalog open/load/create
    dvo_catalog_init (&outcatalogs[i], TRUE);
    outcatalogs[i].filename  = outlist[0].filename[i];
    outcatalogs[i].Nsecfilt  = GetPhotcodeNsecfilt ();
    outcatalogs[i].catflags  = DVO_LOAD_AVERAGE | DVO_LOAD_MISSING | DVO_LOAD_SECFILT | DVO_LOAD_MEASURE | DVO_LOAD_LENSING | DVO_LOAD_LENSOBJ | DVO_LOAD_STARPAR | DVO_LOAD_GALPHOT; // for a subset, use: DVO_LOAD_NONE;
    outcatalogs[i].catformat = CATFORMAT ? dvo_catalog_catformat (CATFORMAT) : catformat;  // set the default catformat from config data
    outcatalogs[i].catmode   = CATMODE   ? dvo_catalog_catmode (CATMODE)     : catmode;    // set the default catmode from config data

    // the user options -set-format and -set-mode assign values to the globals CATFORMAT
    // and CATMODE (in args.c); otherwise the values from the input catalogs are inherited

    if (!dvo_catalog_open (&outcatalogs[i], outlist[0].regions[i], VERBOSE, "w")) {
      fprintf (stderr, "ERROR: failure to open catalog file %s\n", outcatalogs[i].filename);
      exit (2);
    }
  }
  
  return (outcatalogs);
}

# include "addstar.h"
# include "loadICRF.h"

/* This is the DVO program to upload ICRF QSO data (eg, from Leonid Petrov) into a DVO
   database.  It is modeled on the loadstarpar program and is expected to be run only
   rarely (once?).  The ICRF QSO parameter data are delivered as text file.  It does not
   allow a subset of the sky to be uploaded; entire QSO parameter files are loaded if
   supplied on the command line.

   USAGE: loadICRF -D CATDIR (catdir) (file.fits) [...more files]
*/

int main (int argc, char **argv) {

  int i;
  AddstarClientOptions options;

  SetSignals ();
  options = ConfigInit (&argc, argv);
  options = args_loadICRF (&argc, argv, options); // XXX fix args after rest is done

  // load the full sky description table (dvodb must exist)
  SkyTable *sky = SkyTableLoadOptimal (CATDIR, SKY_TABLE, GSCFILE, FALSE, SKY_DEPTH, VERBOSE);
  SkyTableSetFilenames (sky, CATDIR, "cpt");
  
  // load the list of hosts
  HostTable *hosts = NULL;
  if (PARALLEL) {
    hosts = HostTableLoad (CATDIR, sky->hosts);
    if (!hosts) {
      fprintf (stderr, "ERROR: failure reading Host Table %s for database %s\n", sky->hosts, CATDIR);
      exit (1);
    }    

    // ensure that the paths are absolute path names
    for (i = 0; i < hosts->Nhosts; i++) {
      char *tmppath = abspath (hosts->hosts[i].pathname, DVO_MAX_PATH);
      free (hosts->hosts[i].pathname);
      hosts->hosts[i].pathname = tmppath;
    }

    // set up the array of active hosts
    init_remote_hosts ();
  }

  // generate the subset matching the user-selected region
  SkyList *skylist = SkyListByPatch (sky, -1, &UserPatch);

  // argv[i] are a list of input tables; load one at a time
  for (i = 1; i < argc; i++) {
    fprintf (stderr, "loading %s\n", argv[i]);
    loadICRF_table (skylist, hosts, argv[i], &options);
  }
  exit (0);
}  

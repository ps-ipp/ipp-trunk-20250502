# include "addstar.h"
# include "loadstarpar.h"

/* This is the DVO program to upload Greg Green's stellar parametersq into a DVO database.
   It is modeled on the loadwise program and is expected to be run only rarely (once?).
   The stellar parameter data are delivered as *.fits files.  It does not allow a subset
   of the sky to be uploaded; entire stellar parameter files are loaded if supplied on the
   command line.

   USAGE: loadstarpar -D CATDIR (catdir) (file.fits) [...more files]
*/

int main (int argc, char **argv) {

  int i;
  AddstarClientOptions options;

  // need to construct these options with args_loadstarpar...
  SetSignals ();
  options = ConfigInit (&argc, argv);
  options = args_loadstarpar (&argc, argv, options);

  // load the full sky description table (dvodb must exist)
  SkyTable *sky = SkyTableLoadOptimal (CATDIR, SKY_TABLE, GSCFILE, FALSE, SKY_DEPTH, VERBOSE);
  SkyTableSetFilenames (sky, CATDIR, "cpt");
  
  // make the tmpdir if needed
  if (!loadstarpar_tmpdir()) exit (1);

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

  for (i = 1; i < argc; i++) {
    fprintf (stderr, "loading %s\n", argv[i]);
    loadstarpar_table (skylist, hosts, argv[i], &options);
  }

  FreeConfig ();
  FreePhotcodeTable ();
  SkyListFree (skylist);
  SkyTableFree (sky);
  FreeHostTable (hosts);

  free_remote_hosts();

  ohana_memcheck(TRUE);
  ohana_memdump(TRUE);
  exit (0);
}  

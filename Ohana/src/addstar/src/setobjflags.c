# include "addstar.h"
# include "setobjflags.h"

/* This is the DVO program to upload Greg Green's stellar parametersq into a DVO database.
   It is modeled on the loadwise program and is expected to be run only rarely (once?).
   The stellar parameter data are delivered as *.fits files.  It does not allow a subset
   of the sky to be uploaded; entire stellar parameter files are loaded if supplied on the
   command line.

   USAGE: setobjflags -D CATDIR (catdir) (file.fits) [...more files]
*/

int main (int argc, char **argv) {

  // need to construct these options with args_setobjflags...
  SetSignals ();
  ConfigInit_setobjflags (&argc, argv);
  args_setobjflags (&argc, argv);

  // load the full sky description table (dvodb must exist)
  SkyTable *sky = SkyTableLoadOptimal (CATDIR, NULL, NULL, FALSE, -1, VERBOSE);
  SkyTableSetFilenames (sky, CATDIR, "cpt");
  
  // make the tmpdir if needed
  if (!setobjflags_tmpdir()) exit (1);

  // load the list of hosts
  HostTable *hosts = NULL;
  if (PARALLEL) {
    hosts = HostTableLoad (CATDIR, sky->hosts);
    if (!hosts) {
      fprintf (stderr, "ERROR: failure reading Host Table %s for database %s\n", sky->hosts, CATDIR);
      exit (1);
    }    

    // ensure that the paths are absolute path names
    for (int i = 0; i < hosts->Nhosts; i++) {
      char *tmppath = abspath (hosts->hosts[i].pathname, DVO_MAX_PATH);
      free (hosts->hosts[i].pathname);
      hosts->hosts[i].pathname = tmppath;
    }

    // set up the array of active hosts
    init_remote_hosts ();
  }

  // generate the subset matching the user-selected region
  SkyList *skylist = SkyListByPatch (sky, -1, &UserPatch);

  setobjflags_table (skylist, hosts);

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

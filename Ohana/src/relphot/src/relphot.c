# include "relphot.h"

int main (int argc, char **argv) {

  // get configuration info, args
  SetSignals ();
  RelphotMode mode = initialize (argc, argv);
  if (!mode) exit (2);

  SkyList *skylist = NULL;

  SkyTable *sky = SkyTableLoadOptimal (CATDIR, NULL, NULL, TRUE, -1, VERBOSE);
  if (!sky) {
    fprintf (stderr, "ERROR running loading sky table from %s\n", CATDIR);
    exit (2);
  }
  SkyTableSetFilenames (sky, CATDIR, "cpt");
  
  if ((mode != PARALLEL_REGIONS) && (mode != PARALLEL_IMAGES)) {
    skylist = NULL;
    if (UserCatalog) {
      int Nchar = strlen(UserCatalog);
      if (!strcmp (&UserCatalog[Nchar-4], ".cpt")) UserCatalog[Nchar-4] = 0;
      skylist = SkyListByName (sky, UserCatalog);
    } else {
      skylist = SkyListByPatch (sky, -1, &UserPatch);
    }
    if (!skylist) {
      fprintf (stderr, "ERROR setting up skylist for %s\n", CATDIR);
      exit (2);
    }
  }
  
  switch (mode) {
    case UPDATE_IMAGES:
      // calculate zero points for images (may group by exposure [mosaic], night [tgroups]; may measure flat-correction)
      // IF CALLED WITH NLOOP == 0, DOES NOT LOAD bcatalog, just loads image table and generates ImageSubset.dat
      // If called with -update, calls reload_catalogs() just like apply_offsets, UNLESS -only-stacks-and-warps is selected
      relphot_images (skylist);
      relphot_free (sky, skylist);
      exit (0);

    case UPDATE_AVERAGES:
      // take the current set of detections and set the mean magnitudes
      relphot_objects (skylist, 0, NULL);
      relphot_free (sky, skylist);
      exit (0);

    case SYNTH_PHOT:
      // set the mean magnitudes ONLY for SYNPHOT objects
      relphot_synthphot (skylist, 0, NULL);
      relphot_free (sky, skylist);
      exit (0);

    case PARALLEL_REGIONS:
      // calculate zero points for images (may group my exposure [mosaic], night [tgroups]; may measure flat-correction)
      // equivalent to relphot_images, but run in parallel across multiple remote machines
      relphot_parallel_regions (sky);
      relphot_free (sky, skylist);
      exit (0);

    case PARALLEL_IMAGES:
      // operation on the remote machines in the PARALLEL_REGION mode
      relphot_parallel_images (sky);
      relphot_free (sky, skylist);
      exit (0);

    case APPLY_OFFSETS:
      // re-run this step from a previous attempt (assumes an existing Images.subset.dat file)
      if (!PARALLEL) {
	fprintf (stderr, "-apply-offsets only makes sense in an parallel context\n");
	exit (2);
      }
      // we do not need to load grid corrections here.  the value of
      // GRID_MEANFILE, if specified, is passed to the clients
      reload_catalogs (skylist, 0, NULL);
      relphot_free (sky, skylist);
      exit (0);

    default:
      fprintf (stderr, "ERROR: no valid relphot mode chosen\n");
      exit (2);
  }
  fprintf (stderr, "IMPOSSIBLE: skipped out of switch?\n");
  exit (1);
}

/***

    I would like to merge the functionality of relphot_images (Nloop == 0), relphot_objects, and apply_offsets

    relphot_objects vs reload_objects (apply offsets):

    relphot_objects 
    
    
***/

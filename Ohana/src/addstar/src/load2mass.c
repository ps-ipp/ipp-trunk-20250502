# include "addstar.h"
# include "2mass.h"

int main (int argc, char **argv) {

  char *path;
  int i;
  SkyTable *sky, *sky2mass;
  SkyList *skylist = NULL;
  SkyList *overlap = NULL;
  AddstarClientOptions options;

  // need to construct these options with args_load2mass...
  SetSignals ();
  options = ConfigInit (&argc, argv);
  options = args_load2mass (argc, argv, options);

  // load the full sky description table:
  sky = SkyTableLoadOptimal (CATDIR, SKY_TABLE, GSCFILE, TRUE, SKY_DEPTH, VERBOSE);
  SkyTableSetFilenames (sky, CATDIR, "cpt");
  
  // generate the subset matching the user-selected region
  skylist = SkyListByPatch (sky, -1, &UserPatch);

  // if we only match to existing (already populated) regions, limit the select to those regions:
  if (options.existing_regions) {
    SkyList *tmp;
    tmp = SkyListExistingSubset (skylist, CATDIR);
    SkyListFree (skylist);
    skylist = tmp;
  }

  path = TWO_MASS_DIR_AS;

  // the accel.dat file has the raw filenames
  // test if the file exists, or else try the .gz version
  sky2mass = load2mass_acc (path, "accel.dat");
  
  for (i = 0; i < sky2mass[0].Nregions; i++) {
    // check if any of the skylist entries overlap this 2mass catalog:
    overlap = SkyListByBounds_List (skylist, -1, sky2mass[0].regions[i].Rmin, sky2mass[0].regions[i].Rmax, sky2mass[0].regions[i].Dmin, sky2mass[0].regions[i].Dmax);
    if (overlap[0].Nregions == 0) {
      SkyListFree (overlap);
      continue;
    }
    
    fprintf (stderr, "loading %s\n", sky2mass[0].filename[i]);
    load2mass_as_rawdata (overlap, sky2mass[0].filename[i], options);
    SkyListFree (overlap);
  }
  exit (0);
}  

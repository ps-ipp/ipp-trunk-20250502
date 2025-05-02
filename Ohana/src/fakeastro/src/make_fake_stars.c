# include "fakeastro.h"

Stars *make_fake_stars (Catalog *catalog, int Ncatalog, SkyList *skylist, Image *image, Stars *stars, int *nstars) {

  // patch is generous region around image, but limited ot this image
  SkyRegion *imagePatch = get_image_patch (image);

  // fprintf (stderr, "image patch: %f %f , %f %f\n", imagePatch->Rmin, imagePatch->Rmax, imagePatch->Dmin, imagePatch->Dmax);

  int Nstars = *nstars;

  // load stars from database in these regions
  int i;
  for (i = 0; i < Ncatalog; i++) {
    // skylist matches catalog

    // fprintf (stderr, "try catalog: %f %f , %f %f : ", skylist[0].regions[i][0].Rmin, skylist[0].regions[i][0].Rmax, skylist[0].regions[i][0].Dmin, skylist[0].regions[i][0].Dmax);

    // XXX check that catalog overlaps patch
    if (!SkyRegionsOverlap(skylist[0].regions[i], imagePatch)) { continue; }
    // fprintf (stderr, "skip\n"); 
    // fprintf (stderr, "keep\n");

    // generate fake measurements for this image
    stars = make_fake_stars_catalog (stars, &Nstars, imagePatch, &catalog[i], image);
  }

  free (imagePatch);

  *nstars = Nstars;
  return stars;
}

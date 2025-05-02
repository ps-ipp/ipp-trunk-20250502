# include "relastro.h"

// loop over all images.  for any images which have a bad coordinate solution, replace the
// original coordinates and recalculate the positions

int FixProblemImages (SkyList *skylist) {

    fprintf (stderr, "this function is currently not used : check on change from measure -> measureT in load_catalogs\n");
    abort();

  int Nbad;
  off_t i, Nimage;
  Image *image;
  SkyList sublist;

  // allocate so we can reallocate below
  ALLOCATE (sublist.regions, SkyRegion *, 1);
  ALLOCATE (sublist.filename, char *, 1);

  image = getimages (&Nimage, NULL);

  Nbad = 0;
  // first check on the dPos reported for each image
  for (i = 0; i < Nimage; i++) {
    double dPosSum, dPos;
    off_t nPos;

    // check if this image should be fixed
    if (badCoords(i)) {
      Nbad ++;
      continue;
    }

    getOffsets (&dPosSum, &nPos, i);
    dPos = sqrt(dPosSum / nPos);
    if (dPos > 4.0) {
      setBadCoords (i);
      Nbad ++;
    }
  }

  fprintf (stderr, "fixing %d images\n", Nbad);

  for (i = 0; i < Nimage; i++) {
    int j, cat, Ncat, *catlist, Ncatlist;
    Catalog *catalog;

    // check if this image should be fixed
    if (!badCoords(i)) continue;

    fprintf (stderr, "fixing %s\n", image[i].name);

    // I need a list of the catalogs for this image
    catlist = getCatlist(&Ncatlist, i);

    // allocate Ncatlist skylist regions
    REALLOCATE (sublist.regions, SkyRegion *, Ncatlist);
    REALLOCATE (sublist.filename, char *, Ncatlist);
    sublist.Nregions = Ncatlist;
    sublist.ownElements = FALSE; // this list is only holding a view to the elements

    // copy the desired catalogs from skylist to skylistSubset
    for (j = 0; j < Ncatlist; j++) {
      cat = catlist[j];
      sublist.filename[j] = skylist[0].filename[cat];
      sublist.regions[j] = skylist[0].regions[cat];
    }

    // XXX use a different function here
    // catalog = load_catalogs (&sublist, &Ncat, FALSE);
    assert (Ncat == Ncatlist);

    // match measurements with images
    initImageBins (catalog, Ncat, FALSE);
    findImages (catalog, Ncat, FALSE);

    // update the detection coordinates using the new image parameters
    resetImageRaw (catalog, Ncat, i);

    freeImageBins (Ncat);

    // write the updated detections to disk
    save_catalogs (catalog, Ncat);
  }
  
  return (TRUE);
}

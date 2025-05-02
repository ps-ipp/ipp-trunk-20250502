# include "fakeastro.h"

int fakeastro_images_region (ImageInfo *imageInfo, Image *refImage, int NrefImage, SkyTable *skyTableInput, SkyTable *skyTableOutput, SkyRegion *innerRegion) {

  // extend the outer region by 2.0 degrees
  SkyRegion *outerRegion = SkyRegionExpand (innerRegion, 2.0);

  SkyList *skyListInput  = SkyListByPatch (skyTableInput, -1, outerRegion);

  // load all catalogs for this extended region (catalog and skyListInput are 1-to-1 in regions)
  int Ncatalog;
  Catalog *catalog = load_fake_stars (skyListInput, &Ncatalog);

  int i, j;
  for (i = 0; i < NrefImage; i++) {

    // we only want to make fake images for the exposures.  NOTE : we should define
    // template to only include DIS, but this is for safety.
    if (strcmp(&refImage[i].coords.ctype[4], "-DIS")) continue;

    // choose only images in this (inner) region
    double Rc, Dc;
    XY_to_RD (&Rc, &Dc, 0.0, 0.0, &refImage[i].coords);
    Rc = ohana_normalize_angle (Rc);

    // exposure center must be in innerRegion:
    if (!SkyRegionHasPoint(innerRegion, Rc, Dc)) continue;

    int NfakeImage;
    Image *fakeImage = make_fake_images (&refImage[i], &NfakeImage);
    
    // save the new fake images with their true image parameters (not yet fitted to the data)
    if (imageInfo->NtrueImage + NfakeImage >= imageInfo->NTRUEIMAGE) {
      imageInfo->NTRUEIMAGE += 1000 + NfakeImage;
      REALLOCATE (imageInfo->trueImage, Image, imageInfo->NTRUEIMAGE);
    }
    for (j = 0; j < NfakeImage; j++) {
      memcpy (&imageInfo->trueImage[imageInfo->NtrueImage], &fakeImage[j], sizeof(Image));
      imageInfo->NtrueImage ++;
    }

    int NfakeStars = 0;
    Stars *fakeStars = NULL;

    for (j = 0; j < NfakeImage; j++) {

      // we only want to make fake stars for the fake chips (not the PHU entries)
      if (strcmp(&fakeImage[j].coords.ctype[4], "-WRP")) continue;

      int Nstart = NfakeStars;
      fakeStars = make_fake_stars (catalog, Ncatalog, skyListInput, &fakeImage[j], fakeStars, &NfakeStars);
      
      // only fit the new stars to this image
      fit_fake_stars (&fakeStars[Nstart], NfakeStars - Nstart, &fakeImage[j]);

      fprintf (stderr, "%s : %d\n", fakeImage[j].name, NfakeStars - Nstart);
    }
    // send in just the PHU image for reference
    save_fake_stars (skyTableOutput, &fakeImage[0], fakeStars, NfakeStars);
    free (fakeStars);

    // append the new fake images to the end of the full set:
    if (imageInfo->NfakeImage + NfakeImage >= imageInfo->NFAKEIMAGE) {
      imageInfo->NFAKEIMAGE += 1000 + NfakeImage;
      REALLOCATE (imageInfo->fakeImage, Image, imageInfo->NFAKEIMAGE);
    }
    for (j = 0; j < NfakeImage; j++) {
      memcpy (&imageInfo->fakeImage[imageInfo->NfakeImage], &fakeImage[j], sizeof(Image));
      imageInfo->NfakeImage ++;
    }
    
    free (fakeImage);
  }

  SkyListFree (skyListInput);

  for (i = 0; i < Ncatalog; i++) {
    dvo_catalog_free (&catalog[i]);
  }

  return TRUE;
}

/* 
   
   given a modest sized region (Rmin,Rmax,Dmin,Dmax):
   define an exterior region 

   * define an outer region with borders 2 linear deg wider 

   * load all input catalogs for the outer region

   * select all images with exposure centers in the inner region

   * generate fake stars for the selected images

   */

# include "relastro.h"

// This function generates a subset of the images based on selections.  Input db has already
// been loaded with the raw fits table data
int assign_images (FITS_DB *db, RegionHostTable *regionHosts) {

  off_t Nimage;

  INITTIME;

  // convert database table to internal structure (binary to Image)
  // 'image' points to the same memory as db->ftable->buffer
  Image *image = gfits_table_get_Image (&db[0].ftable, &Nimage, &db[0].scaledValue, &db[0].nativeOrder);
  if (!image) {
      fprintf (stderr, "ERROR: failed to read images\n");
      exit (2);
  }
  MARKTIME("convert image table to internal structure: %f sec\n", dtime);

  // *** NOTE : for the moment, regions must be in the range 0 - 360, -90 - +90

  // generate the chip match here so we can define the mosaic centers (if needed)
  BuildChipMatch (image, Nimage);
  MARKTIME("build chip match for %d images: %f sec\n", (int) Nimage, dtime);

  char mapfile[DVO_MAX_PATH];
  snprintf_nowarn (mapfile, DVO_MAX_PATH, "%s/AstroMap.fits", CATDIR);
  AstromOffsetTable *table = AstromOffsetMapLoad (mapfile, 100000, VERBOSE);

  // assign images.coords.offsetMap -> table->map[i]
  if (table) {
    AstromOffsetTableMatchChips (image, Nimage, table);
  } else {
    table = AstromOffsetTableInit ();
  }
  put_astrom_table (table);

  initMosaics (image, Nimage);
  MARKTIME("set mosaic coordinates and Mcal values: %f sec\n", dtime);

  // register the image array with ImageOps.c for later getimageByID calls
  initImages (image, NULL, Nimage, FALSE);

  if (VERBOSE) fprintf (stderr, "finding images\n");

  // for each regionHost, select images which are contained by the region
  // even faster would be to use a tree to get to the real regions...
  select_images_hostregion (regionHosts, image, Nimage);

  // supply the mosaics to the image table for the regionHosts : we already have the image
  // <-> mosaic relationship, we just need to select these mosaics (once per regionHost)
  select_mosaics_hostregion (regionHosts, image, Nimage);

  return TRUE;
}

# define D_NIMAGE 1000

// assign images to the region hosts; at the end, each host will have its list of images
int select_images_hostregion (RegionHostTable *regionHosts, Image *image, off_t Nimage) {

  off_t i, j;

  // INITTIME;
  
  for (i = 0; i < regionHosts->Nhosts; i++) {
    regionHosts->hosts[i].Nimage = 0;
    regionHosts->hosts[i].NIMAGE = D_NIMAGE;
    ALLOCATE (regionHosts->hosts[i].image, Image, regionHosts->hosts[i].NIMAGE);
    ALLOCATE (regionHosts->hosts[i].imseq, off_t, regionHosts->hosts[i].NIMAGE);
  }

  for (j = 0; j < Nimage; j++) {
    
    // allow certain cameras to stay static
    if (SKIP_PS1_CHIP  && isGPC1chip (image[j].photcode)) continue;
    if (SKIP_PS1_STACK && isGPC1stack(image[j].photcode)) continue;
    if (SKIP_HSC       && isHSCchip  (image[j].photcode)) continue;
    if (SKIP_CFH       && isCFHchip  (image[j].photcode)) continue;
    
    /* select images by photcode, or equiv photcode, if specified */
    if (NphotcodesKeep > 0) {
      int found = FALSE;
      // XXX this bit of code excludes DIS mosaics and should be fixed
      for (i = 0; (i < NphotcodesKeep) && !found; i++) {
	if (photcodesKeep[i][0].code == image[j].photcode) found = TRUE;
	if (photcodesKeep[i][0].code == GetPhotcodeEquivCodebyCode(image[j].photcode)) found = TRUE;
      }
      if (!found) continue;
    }
    if (NphotcodesSkip > 0) {
      int found = FALSE;
      for (i = 0; (i < NphotcodesSkip) && !found; i++) {
	if (photcodesSkip[i][0].code == image[j].photcode) found = TRUE;
	if (photcodesSkip[i][0].code == GetPhotcodeEquivCodebyCode(image[j].photcode)) found = TRUE;
      }
      if (found) continue;
    }

    /* exclude images by time */
    if (TimeSelect) {
      if (image[j].tzero < TSTART) continue;
      if (image[j].tzero > TSTOP) continue;
    }
    
    // do not include DIS (PHU-level mosaics) in the output list
    if (!strcmp(&image[j].coords.ctype[4], "-DIS")) continue;

    // Exclude images with crazy astrometry
    // XXX NOTE : this is gpc1-specific
    { 
      double dP1 = hypot(image[j].coords.pc1_1, image[j].coords.pc1_2);
      double dP2 = hypot(image[j].coords.pc2_1, image[j].coords.pc2_2);
      if (fabs(dP1 - 1.0) > 0.02) continue;
      if (fabs(dP2 - 1.0) > 0.02) continue;

      double X00, Y00, X10, Y10, X01, Y01;
      XY_to_LM (&X00, &Y00, 0.0, 0.0, &image[j].coords);
      XY_to_LM (&X10, &Y10, image[j].NX, 0.0, &image[j].coords);
      XY_to_LM (&X01, &Y01, 0.0, image[j].NY, &image[j].coords);
      double dS0 = hypot ((X00 - X10), (Y00 - Y10));
      double dS1 = hypot ((X00 - X01), (Y00 - Y01));
      if (dS0 > 6000) continue;
      if (dS1 > 6500) continue;
    }	

    // use a reference coordinate for each image to assign to hosts
    // define image center - note the DIS images (mosaic phu) are special
    double Xc, Yc;
    double Rc, Dc;
    Xc = 0.5*image[j].NX; 
    Yc = 0.5*image[j].NY;
    
    XY_to_RD (&Rc, &Dc, Xc, Yc, &image[j].coords);
    Rc = ohana_normalize_angle_to_midpoint (Rc, 180.0);
    image[j].RAo  = Rc;
    image[j].DECo = Dc;

    i = find_host_for_coords (regionHosts, Rc, Dc);

    if (i == -1) continue;

    RegionHostInfo *host = &regionHosts->hosts[i];

    // image bounds are defined for a range centered on the image center thus, an image
    // with center 0.5 will have chips bounds ranging from ~ -1.5 to +2.5 or so, while an
    // image with center 359.5 will have chips bounds ranging from ~ 357.5 to 361.5 or so
    double Rmin, Rmax, Dmin, Dmax;
    calculate_image_bounds (&image[j], &Rmin, &Rmax, &Dmin, &Dmax, Rc);

    host->RminCat = MIN(Rmin, host->RminCat);
    host->RmaxCat = MAX(Rmax, host->RmaxCat);
    host->DminCat = MIN(Dmin, host->DminCat);
    host->DmaxCat = MAX(Dmax, host->DmaxCat);

    // regionHosts needs to have the full outer boundary
    // (so reload_catalogs covers the correct region)
    regionHosts->Rmin = MIN(Rmin, regionHosts->Rmin);
    regionHosts->Rmax = MAX(Rmax, regionHosts->Rmax);
    regionHosts->Dmin = MIN(Dmin, regionHosts->Dmin);
    regionHosts->Dmax = MAX(Dmax, regionHosts->Dmax);

    // this is a bit memory expensive : I am making a complete copy of the image table here
    // XXX is adding an image, can we just add a pointer?
    off_t Nsubset = host->Nimage;
    host->image[Nsubset] = image[j];
    host->imseq[Nsubset] = j;

    host->Nimage ++;
    if (host->Nimage == host->NIMAGE) {
      host->NIMAGE += D_NIMAGE;
      REALLOCATE (host->image, Image, host->NIMAGE);
      REALLOCATE (host->imseq, off_t, host->NIMAGE);
    }

    // save the astrometry maps, where they exist
    // here we are adding an AstromOffsetTable, but the data are only pointers
    // we should NOT free this table with AstromOffsetTableFree()
    if (image[j].coords.offsetMap) {
      if (!host->astromTable) {
	host->astromTable = AstromOffsetTableInit();
      }
      AstromOffsetTableAddMapFromImage(host->astromTable, &image[j]);
    }
  }

  return TRUE;
}

double Xf[] = {0.0, 1.0, 0.0, 1.0};
double Yf[] = {0.0, 0.0, 1.0, 1.0};

int calculate_image_bounds (Image *image, double *rmin, double *rmax, double *dmin, double *dmax, double Rmid) {

  int n;

  double Rmin = 360.0;
  double Rmax =   0.0;
  double Dmin = +90.0;
  double Dmax = -90.0;

  // define image corners
  for (n = 0; n < 4; n++) {
    double Xc, Yc, Rc, Dc;
    Xc = Xf[n]*image->NX; 
    Yc = Yf[n]*image->NY;
    XY_to_RD (&Rc, &Dc, Xc, Yc, &image->coords);
    Rc = ohana_normalize_angle_to_midpoint (Rc, Rmid);
      
    Rmin = MIN (Rmin, Rc);
    Rmax = MAX (Rmax, Rc);
    Dmin = MIN (Dmin, Dc);
    Dmax = MAX (Dmax, Dc);
  }

  *rmin = Rmin;
  *rmax = Rmax;
  *dmin = Dmin;
  *dmax = Dmax;

  return TRUE;
}

// XXX add a search tree to speed this up?
int find_host_for_coords (RegionHostTable *regionHosts, double Rc, double Dc) {

  int i;

  if (isnan(Rc)) return -1;
  if (isnan(Dc)) return -1;

  for (i = 0; i < regionHosts->Nhosts; i++) {

    RegionHostInfo *host = &regionHosts->hosts[i];
    if (Rc <  host->Rmin) continue;
    if (Rc >= host->Rmax) continue;
    if (Dc <  host->Dmin) continue;
    if (Dc >= host->Dmax) continue;

    return i;
  }
  return -1;
}

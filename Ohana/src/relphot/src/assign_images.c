# include "relphot.h"

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
  snprintf (mapfile, DVO_MAX_PATH, "%s/AstroMap.fits", CATDIR);
  AstromOffsetTable *table = AstromOffsetMapLoad (mapfile, 100000, VERBOSE);

  // assign images.coords.offsetMap -> table->map[i]
  if (table) {
    AstromOffsetTableMatchChips (image, Nimage, table);
  }
  put_astrom_table (table);

  if (MOSAIC_ZEROPT) {
    makeMosaics (image, Nimage, FALSE);
    MARKTIME("set mosaic coordinates and Mcal values: %f sec\n", dtime);

    // center coords and zero Mcal, dMcal, Mchisq for the mosaics
    setMosaicCenters (image, Nimage);
    MARKTIME("set mosaic coordinates and Mcal values: %f sec\n", dtime);
  }

  // register the image array with ImageOps.c for later getimageByID calls
  initImages (image, NULL, Nimage);

  if (VERBOSE) fprintf (stderr, "finding images\n");

  // for each regionHost, select images which are contained by the region
  // even faster would be to use a tree to get to the real regions...
  select_images_hostregion (regionHosts, image, Nimage);

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
  }

  for (j = 0; j < Nimage; j++) {
    
    /* exclude images by photcode */
    int Ns = GetActivePhotcodeIndex (image[j].photcode);
    if (Ns < 0) continue;

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
    // define image center - note the DIS images (mosaic phu) are special, but we have 
    // already excluded them above.  we also save the image centers for reference
    double Xc, Yc;
    double Rc, Dc;

    Xc = 0.5*image[j].NX; 
    Yc = 0.5*image[j].NY;

    XY_to_RD (&Rc, &Dc, Xc, Yc, &image[j].coords);
    Rc = ohana_normalize_angle_to_midpoint (Rc, 180.0);
    image[j].RAo  = Rc;
    image[j].DECo = Dc;

    // NOTE: if we are NOT using mosaic centers, we don't need to keep chips with centers
    // on opposite sides of 0,360 together.  in which case, it is OK for the range of Rc
    // to be 0.0 to 360.0.  also NOTE: RAo,DECo are only used for debugging reference.

    if (MOSAIC_ZEROPT) {
      // use the coords of the associated mosaic to select (only for chips; stacks use their own center)
      Mosaic *mosaic = getMosaicForImage (j); 
      if (mosaic) {
    	Rc = mosaic->coords.crval1;
    	Dc = mosaic->coords.crval2;
    	// NOTE : have defined mosaic Rc,Dc to choose the side of 0,360 on which most of the
    	// chips are located.  but, for host assignment, we rationalize to 0.0 - 360.0
    	Rc = ohana_normalize_angle_to_midpoint (Rc, 180.0);
      }
    }

    i = find_host_for_coords (regionHosts, Rc, Dc);

    if (i == -1) continue;

    RegionHostInfo *host = &regionHosts->hosts[i];

    // image bounds are defined for a range centered on the mosaic center thus, a mosaic
    // with center 0.5 will have chips bounds ranging from ~ -1.5 to +2.5 or so, while a mosaic
    // with center 359.5 will have chips bounds ranging from ~ 357.5 to 361.5 or so
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
    off_t Nsubset = regionHosts->hosts[i].Nimage;
    regionHosts->hosts[i].image[Nsubset] = image[j];
    // regionHosts->hosts[i].line_number[Nsubset] = j;

    regionHosts->hosts[i].Nimage ++;
    if (regionHosts->hosts[i].Nimage == regionHosts->hosts[i].NIMAGE) {
      regionHosts->hosts[i].NIMAGE += D_NIMAGE;
      REALLOCATE (regionHosts->hosts[i].image, Image, regionHosts->hosts[i].NIMAGE);
      // REALLOCATE (regionHosts->hosts[i].line_number, off_t, regionHosts->hosts[i].NIMAGE);
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

// Rc is in range 0.0 - 360.0, hosts Rmin,Rmax in range 0.0 - 360.0
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


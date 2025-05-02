# include "addstar.h"

// for DIS/WRP sets, we need to save the DIS set and supply it to the WRP entries
static Coords *mosaic = NULL;

void initMosaicCoords () {
  mosaic = NULL;
}

void saveMosaicCoords (Coords *input) {
  ALLOCATE (mosaic, Coords, 1);
  memcpy (mosaic, input, sizeof(Coords));
}

/* read an image header corresponding to a CMF / CMP data block */
// XXX need to pass AddstarClientOptions?
int ReadImageHeader (Header *header, Image *image, int photcode) {

  int Nastro, hour, min, Nx, Ny, haveNx, haveNy, sourceID;
  double tmp, sec, Cerror, FWHM_X, FWHM_Y;
  char *c, photname[64], line[80], ccdnum[64];
  PhotCode *photcodeData = NULL;

  // zero out the entire image structure
  memset (image, 0, sizeof(Image));

  /* get astrometry information */
  if (!GetCoords (&image[0].coords, header)) {
    fprintf (stderr, "no astrometric solution in header\n");
    return (FALSE);
  }
  // XXX currently, image uses an unsigned short for NX,XY. this is rather restrictive
  // and needs to be at least checked.
  haveNx = gfits_scan (header, "NAXIS1",   "%d", 1, &Nx);
  haveNy = gfits_scan (header, "NAXIS2",   "%d", 1, &Ny);

  if (!haveNx && !haveNy) {
      haveNx = gfits_scan (header, "IMNAXIS1",   "%d", 1, &Nx);
      haveNy = gfits_scan (header, "IMNAXIS2",   "%d", 1, &Ny);
  }

  if (!haveNx && !haveNy) {
      haveNx = gfits_scan (header, "ZNAXIS1",   "%d", 1, &Nx);
      haveNy = gfits_scan (header, "ZNAXIS2",   "%d", 1, &Ny);
  }

  if (!haveNx || !haveNy) {
      fprintf (stderr, "missing image dimensions in header\n");
      return (FALSE);
  }

  if ((Nx < 0) || (Nx > 0xffff)) {
    fprintf (stderr, "WARNING: NX, NY out of range : image boundary will be wrong\n");
  }

  image[0].NX = Nx;
  image[0].NY = Ny;

  image[0].refColorBlue = NAN;
  image[0].refColorRed = NAN;

  if (!gfits_scan (header, "TZERO",   "%d",  1, &image[0].tzero) && !ACCEPT_TIME) {
    image[0].tzero = parse_time (header);
  }

  /* only load astrometry, NAXIS1,2, and time if this is a MOSAIC_PHU (ctype is ....-DIS) */
  if (!strcmp (&image[0].coords.ctype[4], "-DIS")) {
    saveMosaicCoords (&image[0].coords);
    return (TRUE);
  }

  // 2021.11.03 : consider replacing or supplementing CERROR with CERSTD
  // if (!gfits_scan (header, "CERROR",   "%lf", 1, &tmp)) tmp = 1.0;

  /* require Nastro > 0 unless or ACCEPT_ASTROM */
  FWHM_X = FWHM_Y = Nastro = Cerror = 0;

  gfits_scan (header, "NASTRO", "%d",  1, &Nastro);
  gfits_scan (header, "CERROR", "%lf", 1, &Cerror);
  gfits_scan (header, "FWHM_X", "%lf", 1, &FWHM_X);
  gfits_scan (header, "FWHM_Y", "%lf", 1, &FWHM_Y);

  if (((Nastro == 0) || (Cerror > MAX_CERROR)) && !ACCEPT_ASTROM) {
    fprintf (stderr, "bad astrometric solution in header\n");
    return (FALSE);
  }
  if ((MIN_FWHM_X > 0.01) && (FWHM_X < MIN_FWHM_X)) {
    fprintf (stderr, "bad psf, skipping\n");
    return (FALSE);
  }
  if ((MIN_FWHM_Y > 0.01) && (FWHM_Y < MIN_FWHM_Y)) {
    fprintf (stderr, "bad psf, skipping\n");
    return (FALSE);
  }
  if (!strcmp (&image[0].coords.ctype[4], "-WRP")) {
    if (!mosaic) {
      fprintf (stderr, "no mosaic for WRP image (use -mosaic)\n");
      return (FALSE);
    }
    image[0].coords.mosaic = mosaic;
  } else {
    /* force image to lie in 0-360 range */
    image[0].coords.crval1 = ohana_normalize_angle (image[0].coords.crval1);
  }

  {
    double R, D;
    /* sanity check on the image coordinates */
    XY_to_RD (&R, &D, 0.5*Nx, 0.5*Ny, &image[0].coords);
    if (!finite(R) || !finite(D)) {
      fprintf (stderr, "corrupted header coordinates, skipping\n");
      return (FALSE);
    }
  }

  /* CERROR / CERSTD in data file in arcsec; db field is units of 20 mas */
  image[0].cerror = 50.0 * Cerror;

  /* get photcode from header */
  if (photcode == 0) {
    if (!gfits_scan (header, "PHOTCODE", "%s", 1, photname)) {
      fprintf (stderr, "failure: photcode not supplied in header\n");
      return (FALSE);
    }
    photcodeData = GetPhotcodebyName (photname);
    if (photcodeData == NULL) {
      fprintf (stderr, "photcode %s not found in photcode table\n", photname);
      return (FALSE);
    }
    photcode = photcodeData[0].code;
  }
  if (photcode == 0) {
    fprintf (stderr, "no valid photcode is supplied\n");
    return (FALSE);
  }
  image[0].photcode = photcode;

  image[0].NX -= XOVERSCAN;
  image[0].NY -= YOVERSCAN;
  gfits_scan (header, ExptimeKeyword,  "%lf", 1, &tmp);
  image[0].exptime = tmp;

  /*** why are we no longer using APMIFIT?? ***/
  tmp = 0;
  /* gfits_scan (header, "APMIFIT",  "%lf", 1, &tmp); */
  image[0].apmifit = tmp;

  tmp = 0;
  /* gfits_scan (header, "dAPMIFIT", "%lf", 1, &tmp); */
  image[0].dapmifit = tmp;

  tmp = 0;
  // gfits_scan (header, "FLIMIT",   "%lf", 1, &tmp);
  gfits_scan (header, "DETEFF.MAGREF", "%lf", 1, &tmp);
  image[0].detection_limit = tmp * 10.0;

  tmp = 0;
  gfits_scan (header, "FSATUR",   "%lf", 1, &tmp);
  image[0].saturation_limit = tmp * 10.0;

  tmp = 0;
  gfits_scan (header, "FWHM_X",   "%lf", 1, &tmp);
  image[0].fwhm_x = tmp * 25.0 * image[0].coords.cdelt1 * 3600.0;

  tmp = 0;
  gfits_scan (header, "FWHM_Y",   "%lf", 1, &tmp);
  image[0].fwhm_y = tmp * 25.0 * image[0].coords.cdelt1 * 3600.0;

  if (STKeyword[0] && strcasecmp(STKeyword, "NONE")) {
    /* get ST (used for airmass calculation) */
    gfits_scan (header, STKeyword, "%s", 1, line);
    /* remove ':' characters */
    for (c = strchr (line, ':'); c != (char *) NULL; c = strchr (line, ':')) { *c = ' '; }
    sscanf (line, "%d %d %lf", &hour, &min, &sec);
    image[0].sidtime = hour + min/60.0 + sec/3600.0;
  } else {
    double jd;
    jd = ohana_sec_to_jd (image[0].tzero);
    image[0].sidtime  = ohana_lst (jd, Longitude);
  }
  image[0].latitude = Latitude;

  tmp = 0;
  if (gfits_scan (header, "TRATE",   "%lf", 1, &tmp)) {
    image[0].trate = 10000 * tmp;
  } else {
    image[0].trate = 0.0;
  }

  image[0].secz = NAN;
  if (gfits_scan (header, AirmassKeyword, "%lf", 1, &tmp)) {
    image[0].secz = tmp;
  }

  if (!gfits_scan (header, CCDNumKeyword, "%s", 1, ccdnum)) {
    fprintf (stderr, "CCDNumKeyword %s not found\n", CCDNumKeyword);
    return FALSE;
  } else {
    // CCDNumKeyword (EXTNAME) is a string yet we need an integer
    // RULE: ccdnum is the value of the first string of digits in CCDNumKeyword
    // For example: ccdnum(XY42.hdr) = 42, ccdnum(XY01.hdr) = 1
    // if no digits occur, then we assume there is only one ccd
    char *p = ccdnum;
    // get rid of any leading non-digit characters
    while (*p && !isdigit(*p)) p++;
    if (*p) {
      image[0].ccdnum = atoi(p);
    } else {
      image[0].ccdnum = 0;
    }
  }

  if (!gfits_scan (header, ImageIDKeyword, "%u", 1, &image[0].externID)) {
    image[0].externID = 0;
  }
  if (!gfits_scan (header, SourceIDKeyword, "%d", 1, &sourceID)) {
    image[0].sourceID = 0;
  } else {
    if (sourceID > 0xffff) {
      fprintf (stderr, "image source ID is surprisingly large: %d, skipping\n", sourceID);
      return (FALSE);
    }
    image[0].sourceID = sourceID;
  }

#ifdef notdef
  // XXX this is archaic: we used to set a fixed zero point of 25 to shift data into the range
  // 0 - 32 so it would fit in an unsigned int.  This is also needed because some programs like
  // sextractor will put in an arbitrary zero point that we need to understand to get back to
  // instrumental mags.
  double ZeroPt;
  gfits_scan (header, "ZERO_PT", "%lf", 1, &ZeroPt);
  if (ZeroPt != GetZeroPoint()) {
      fprintf (stderr, "WARNING: inconsistent zero point values: image: %f, config: %f\n", ZeroPt, GetZeroPoint());
  }
#endif

  // in this case, lookup and apply the zero point measured for this chip
  if (!strcasecmp(ZERO_POINT_OPTION, "CHIP_HEADER")) {
      float ZPT_OBS;
      if (!photcodeData || !gfits_scan (header, ZERO_POINT_KEYWORD, "%f", 1, &ZPT_OBS)) {
          fprintf (stderr, "zero point not supplied in header\n");
          ZERO_POINT_OFFSET = 0.0;
          ZERO_POINT_ERROR = NAN;
      } else {
          ZERO_POINT_OFFSET = 0.001*photcodeData[0].C - ZPT_OBS;
          float ZPT_ERR;
          if (!gfits_scan (header, "ZPT_ERR", "%f", 1, &ZPT_ERR)) {
              // XXX: do we want to print this message?
              fprintf (stderr, "zero point error not supplied in header\n");
              ZPT_ERR = NAN;
          }
          ZERO_POINT_ERROR = ZPT_ERR;
      }
  }

  // in this case, lookup and apply the zero point measured for the exposure
  if (!strcasecmp(ZERO_POINT_OPTION, "PHU_HEADER")) {
      if (!photcodeData) {
          fprintf (stderr, "photcode data not supplied for this chip\n");
          ZERO_POINT_OFFSET = 0.0;
          ZERO_POINT_ERROR = NAN;
      } else {
          ZERO_POINT_OFFSET = 0.001*photcodeData[0].C - ZPT_OBS_PHU;
          ZERO_POINT_ERROR =  ZPT_ERR_PHU;
      }
  }

  image[0].McalPSF   = ZERO_POINT_OFFSET;
  image[0].McalAPER  = ZERO_POINT_OFFSET;
  image[0].dMcal     = ZERO_POINT_ERROR;
  image[0].McalChiSq = NAN;
  image[0].flags     = 0;

  /* find expected number of stars */
  if (!gfits_scan (header, "NSTARS", "%u", 1, &image[0].nstar) && !NO_STARS) {
    fprintf (stderr, "WARNING: can't get NSTARS from header (TEXT mode will be invalid)\n");
  }

  return (TRUE);
}

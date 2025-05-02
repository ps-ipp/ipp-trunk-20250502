# include "addstar.h"
# define EXTERNAL_ID TRUE

/* exclude some detections based on various filter options:
 * *** S/N limit
 * *** Acceptable window on detector
 * *** bad photFlags
 */

// As a temporary hack, I am going to overload the following fields with radial aperture values
// from the convolved images:

# define  F_ApR5_C1 X11_sm_obj
# define dF_ApR5_C1  E1_sm_obj
# define  F_ApR6_C1 X22_sm_obj
# define dF_ApR6_C1  E2_sm_obj
# define  F_ApR7_C1 X11_sh_obj
# define dF_ApR7_C1  E2_sh_obj

# define  F_ApR5_C2 X11_sm_psf
# define dF_ApR5_C2  E1_sm_psf
# define  F_ApR6_C2 X22_sm_psf
# define dF_ApR6_C2  E2_sm_psf
# define  F_ApR7_C2 X11_sh_psf
# define dF_ApR7_C2  E2_sh_psf

// the imageID supplied here is the sequence **within this set**
// this value is updated based on the image table later (in UpdateImageIDs)
Catalog *FilterStars (Catalog *newcat, Image *image, unsigned int imageID, SkyRegion *region, const AddstarClientOptions *options) {

  int j, N;
  float MTIME, dMs, dMx;
  float RMIN, RMAX, DMIN, DMAX;

  /* correct instrumental mags for exposure time */
  MTIME = (image[0].exptime > 0) ? 2.500*log10(image[0].exptime) : 0.0;

  RMIN = region->Rmin;
  RMAX = region->Rmax;
  DMIN = region->Dmin;
  DMAX = region->Dmax;

  /* modify resulting star list */
  Catalog *outcat = addstar_catalog_init (newcat->Nmeasure);
  ALLOCATE (outcat->average, Average, newcat->Nmeasure);
  if (newcat->lensing) {
    ALLOCATE (outcat->lensing, Lensing, newcat->Nlensing);
  }
  
  for (N = j = 0; j < newcat->Nmeasure; j++) {

    if (newcat->measure[j].photFlags & options->detectionFilter) continue;

    /* allow for some dynamic filtering of star list */
    if (SNLIMIT && newcat->measure[j].dM > SNLIMIT) continue;
    if (XMAX && (newcat->measure[j].Xccd > XMAX)) continue;
    if (XMIN && (newcat->measure[j].Xccd < XMIN)) continue;
    if (YMAX && (newcat->measure[j].Yccd > YMAX)) continue;
    if (YMIN && (newcat->measure[j].Yccd < YMIN)) continue;
    if (PHOTFLAG_EXCLUDE && (newcat->measure[j].photFlags & PHOTFLAG_EXCLUDE)) continue;

    dvo_average_init (&outcat->average[N]);
    outcat->measure[N] = newcat->measure[j];

    if (newcat->lensing) {
      outcat->lensing[N] = newcat->lensing[j];
      outcat->average[N].Nlensing = 1;
      outcat->average[N].lensingOffset = N;
    }

    outcat->average[N].Nmeasure = 1;
    outcat->average[N].measureOffset = N;

    XY_to_RD (&outcat->average[N].R, &outcat->average[N].D, outcat->measure[N].Xccd, outcat->measure[N].Yccd, &image[0].coords);
    outcat->average[N].R = ohana_normalize_angle (outcat->average[N].R);
    outcat->measure[N].R = outcat->average[N].R;
    outcat->measure[N].D = outcat->average[N].D;

    outcat->measure[N].photcode = image[0].photcode;

    // determine the full coverage of this set of measurements
    RMIN = MIN (RMIN, outcat->average[N].R);
    RMAX = MAX (RMAX, outcat->average[N].R);
    DMIN = MIN (DMIN, outcat->average[N].D);
    DMAX = MAX (DMAX, outcat->average[N].D);

    /** additional quantities to supply to Stars based on the image data **/

    /* calculate accurate per-star airmass and azimuth */
    outcat->measure[N].airmass = airmass (image[0].secz, outcat->average[N].R, outcat->average[N].D, image[0].sidtime, image[0].latitude);
    outcat->measure[N].az      = azimuth (15.0*image[0].sidtime - outcat->average[N].R, outcat->average[N].D, image[0].latitude);
    outcat->measure[N].McalPSF = image[0].McalPSF;
    outcat->measure[N].McalAPER= image[0].McalAPER;
    outcat->measure[N].t       = image[0].tzero + 1e-4*outcat->measure[N].Yccd*image[0].trate;  /* trate is in 0.1 msec / row */
    outcat->measure[N].dt      = MTIME;

    // watch out for any strange values:
    if ((outcat->measure[N].M > 25.0) && (outcat->measure[N].M < 32.0)) {
      fprintf (stderr, "*");
    }
    // stars->M is either NAN or a valid inst magnitude
    // stars->dM is either NAN or a valid error

    dMs  = 0.0;
    dMx = 0.0;
    if (SUBPIX) {
      dMs =  get_subpix (outcat->measure[N].Xccd, outcat->measure[N].Yccd);
      dMx = scat_subpix (outcat->measure[N].Xccd, outcat->measure[N].Yccd);
      if (!isnan(outcat->measure[N].dM)) {
	outcat->measure[N].dM = hypot (outcat->measure[N].dM, dMx);
      }
    }

    if (!isnan(outcat->measure[N].M)) {
      outcat->measure[N].M   += MTIME - dMs;
    }
    if (!isnan(outcat->measure[N].Map)) {
      outcat->measure[N].Map += MTIME - dMs;
    }
    if (!isnan(outcat->measure[N].Mkron)) {
      outcat->measure[N].Mkron += MTIME - dMs;
    }
    if (!isnan(outcat->measure[N].FluxPSF)) {
      outcat->measure[N].FluxPSF /= image[0].exptime;
    }
    if (!isnan(outcat->measure[N].dFluxPSF)) {
      outcat->measure[N].dFluxPSF /= image[0].exptime;
    }
    if (!isnan(outcat->measure[N].FluxKron)) {
      outcat->measure[N].FluxKron /= image[0].exptime;
    }
    if (!isnan(outcat->measure[N].dFluxKron)) {
      outcat->measure[N].dFluxKron /= image[0].exptime;
    }
    if (!isnan(outcat->measure[N].FluxAp)) {
      outcat->measure[N].FluxAp /= image[0].exptime;
    }
    if (!isnan(outcat->measure[N].dFluxAp)) {
      outcat->measure[N].dFluxAp /= image[0].exptime;
    }
    if (outcat->lensing) {
      // correct things which scale like exptime with the exptime
      if (!isnan(outcat->lensing[N]. F_ApR5)) outcat->lensing[N]. F_ApR5 /= image[0].exptime;
      if (!isnan(outcat->lensing[N].dF_ApR5)) outcat->lensing[N].dF_ApR5 /= image[0].exptime;
      if (!isnan(outcat->lensing[N].sF_ApR5)) outcat->lensing[N].sF_ApR5 /= image[0].exptime;
      if (!isnan(outcat->lensing[N]. F_ApR6)) outcat->lensing[N]. F_ApR6 /= image[0].exptime;
      if (!isnan(outcat->lensing[N].dF_ApR6)) outcat->lensing[N].dF_ApR6 /= image[0].exptime;
      if (!isnan(outcat->lensing[N].sF_ApR6)) outcat->lensing[N].sF_ApR6 /= image[0].exptime;
      if (!isnan(outcat->lensing[N]. F_ApR7)) outcat->lensing[N]. F_ApR7 /= image[0].exptime;
      if (!isnan(outcat->lensing[N].dF_ApR7)) outcat->lensing[N].dF_ApR7 /= image[0].exptime;
      if (!isnan(outcat->lensing[N].sF_ApR7)) outcat->lensing[N].sF_ApR7 /= image[0].exptime;

      // XXX optionall use these overloaded names?
      if (!isnan(outcat->lensing[N]. F_ApR5_C1)) outcat->lensing[N]. F_ApR5_C1 /= image[0].exptime;
      if (!isnan(outcat->lensing[N].dF_ApR5_C1)) outcat->lensing[N].dF_ApR5_C1 /= image[0].exptime;
      if (!isnan(outcat->lensing[N]. F_ApR6_C1)) outcat->lensing[N]. F_ApR6_C1 /= image[0].exptime;
      if (!isnan(outcat->lensing[N].dF_ApR6_C1)) outcat->lensing[N].dF_ApR6_C1 /= image[0].exptime;
      if (!isnan(outcat->lensing[N]. F_ApR7_C1)) outcat->lensing[N]. F_ApR7_C1 /= image[0].exptime;
      if (!isnan(outcat->lensing[N].dF_ApR7_C1)) outcat->lensing[N].dF_ApR7_C1 /= image[0].exptime;

      if (!isnan(outcat->lensing[N]. F_ApR5_C2)) outcat->lensing[N]. F_ApR5_C2 /= image[0].exptime;
      if (!isnan(outcat->lensing[N].dF_ApR5_C2)) outcat->lensing[N].dF_ApR5_C2 /= image[0].exptime;
      if (!isnan(outcat->lensing[N]. F_ApR6_C2)) outcat->lensing[N]. F_ApR6_C2 /= image[0].exptime;
      if (!isnan(outcat->lensing[N].dF_ApR6_C2)) outcat->lensing[N].dF_ApR6_C2 /= image[0].exptime;
      if (!isnan(outcat->lensing[N]. F_ApR7_C2)) outcat->lensing[N]. F_ApR7_C2 /= image[0].exptime;
      if (!isnan(outcat->lensing[N].dF_ApR7_C2)) outcat->lensing[N].dF_ApR7_C2 /= image[0].exptime;
    }
    
    // the external ID is supplied, but do we trust it?
    if (!EXTERNAL_ID) {
      outcat->measure[N].detID = N; // sequence number within image
    }

    if (PSPS_ID) {
      double mjd;
      mjd = ohana_sec_to_mjd (image[0].tzero);
      int isStack = ((image[0].photcode >= 11000) && (image[0].photcode <= 11400));

      // this is wrong for forcedwarps: should be treated like a stackID?
      if (isStack) {
	outcat->measure[N].extID = CreatePSPSStackDetectionID(image[0].sourceID, image[0].externID, outcat->measure[N].detID);
      } else {
	outcat->measure[N].extID = CreatePSPSDetectionID(mjd, image[0].ccdnum, outcat->measure[N].detID);
      }
    } else {
      outcat->measure[N].extID = 0;
    }

    outcat->measure[N].imageID = imageID; // this value is updated in UpdateImageIDs

    // add imageID to lensing entry, if it exists
    if (outcat->lensing) {
      outcat->lensing[N].imageID = imageID;
    }

    N ++;
  }

  // DEBUG printf("N stars orig = %d after filter = %d\n", image[0].nstar, N);
  
  image[0].nstar = N;

  REALLOCATE (outcat->measure, Measure, N);
  REALLOCATE (outcat->average, Average, N);
  if (outcat->lensing) {
    REALLOCATE (outcat->average, Average, N);
  }
  dvo_catalog_free (newcat);
  free (newcat);

  outcat->Naverage = N;
  outcat->Nmeasure = N;
  outcat->Nlensing = outcat->lensing ? N : 0;

  if (VERBOSE) fprintf (stderr, "read %d stars from target file\n", N);
  if (VERBOSE) fprintf (stderr, "stars cover region %f,%f - %f,%f\n", RMIN, DMIN, RMAX, DMAX);

  // define containing region a bit generously
  region->Rmin = RMIN;
  region->Rmax = RMAX;
  region->Dmin = DMIN;
  region->Dmax = DMAX;

  return (outcat);
}

# if (0)
Stars *MergeStars (Stars *stars, unsigned int *Nstars, Stars *instars, unsigned int Ninstars) {

  int i, j;

  if (stars == NULL) {
    ALLOCATE (stars, Stars, Ninstars);
  } else {
    REALLOCATE (stars, Stars, *Nstars + Ninstars);
  }

  for (j = 0, i = *Nstars; i < *Nstars + Ninstars; i++, j++) {
    stars[i] = instars[j];
  }
  
  *Nstars += Ninstars;
  return (stars);
}
# endif

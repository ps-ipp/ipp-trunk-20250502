# include "relastro.h"

// NOTE: we only measure the systematic floor of the astrometric scatter per stack, no change to the calibration
int UpdateStacks (Catalog *catalog, int Ncatalog) {

  if (SKIP_PS1_STACK) return TRUE;

  off_t Nimage;
  Image *image = getimages (&Nimage, NULL);

  off_t Nskip = 0;
  off_t Nmeas = 0;
  off_t NnewFit = 0;
  off_t NoldFit = 0;

  // measure the scatter for each stack
  for (off_t i = 0; i < Nimage; i++) {

    /* skip all except stack images */
    if (!isGPC1stack(image[i].photcode)) continue;

    /* convert measure coordinates to raw entries */
    off_t Nraw;
    StarData *raw = getImageRaw (catalog, Ncatalog, i, &Nraw, MODE_SIMPLE);
    if (!raw) {
      Nskip ++;
      continue;
    }
    if (Nraw <= IMFIT_TOO_FEW) {
      Nskip ++;
      free (raw);
      continue;
    }

    /* convert average coordinates to ref entries */
    off_t Nref;
    StarData *ref = getImageRef (catalog, Ncatalog, i, &Nref, MODE_SIMPLE);
    if (!ref) {
      Nskip ++;
      free (raw);
      continue;
    }

    // note that Nraw & Nref must be equal: if not, we made a programming error in one of these two functions.
    assert (Nraw == Nref);

    // the natural default for stacks is to NOT re-fit
    if (FIT_STACKS == FALSE) {
      int Nstat;
      float dLsig, dMsig, dRsig;
      GetScatterRawRef(&dLsig, &dMsig, &dRsig, &Nstat, raw, ref, Nraw, IMFIT_SYS_SIGMA_LIM);

      // XXX: I need to convert dLsig, dMsig from degrees to pixels
      dLsig *= 3600.0;
      dMsig *= 3600.0;

      image[i].dXpixSys = dLsig;
      image[i].dYpixSys = dMsig;
      image[i].nFitAstrom = Nstat;
      continue;
    }

    // save these in case of failure
    Coords oldCoords;
    SaveCoords (&oldCoords, &image[i].coords);

    float dXpixSys = image[i].dXpixSys;
    float dYpixSys = image[i].dYpixSys;
    int   nFitAstr = image[i].nFitAstrom;

    // FitChip does iterative, clipped fitting
    if (!FitChip (raw, ref, Nraw, &image[i])) {
      if (VERBOSE) fprintf (stderr, "reject fit for image %s ("OFF_T_FMT") : Nstars: "OFF_T_FMT", Nused %d of %d\n", image[i].name, i, Nraw, image[i].nFitAstrom, image[i].nstar);

      // restore status quo ante (replace truMap with tmpMap)
      RestoreCoords (&image[i].coords, &oldCoords, &image[i]);
      image[i].dXpixSys = dXpixSys;
      image[i].dYpixSys = dYpixSys; 
      image[i].nFitAstrom = nFitAstr;

      NoldFit ++;
      free (raw);
      free (ref);
      continue;
    }

    AstromOffsetMapFree (oldCoords.offsetMap);

    // Apply the modified coords back to the measure.R,D.  Note that raw.R,D, ref.L,M, etc
    // are all automatically updated in this block because they are re-generated from
    // image.coords on each pass.
    setImageRaw (catalog, Ncatalog, i, raw, Nraw, MODE_MOSAIC);

    NnewFit ++;
    free (raw);
    free (ref);
  }

  fprintf (stderr, "UpdateStacks: %d measured, %d skipped\n", (int) Nmeas, (int) Nskip);
  return (TRUE);
}

// XXX 2 hardwired hacks in this file: 1) photcode hardwired for GPC1 stacks, 2) platescale

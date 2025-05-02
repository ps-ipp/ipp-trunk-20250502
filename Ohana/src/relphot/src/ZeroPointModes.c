# include "relphot.h"

static int CurrentLoop = -1; // track the number of iterations

void SetZptIteration (int current) {
  // XXX add 'manual' option here: ask for desired zpt iteration
  if (MANUAL_ITERATION) {
    int newValue;
    fprintf (stdout, "Set Iteration (current = %d): ", CurrentLoop);
    if (!fscanf (stdin, "%d", &newValue)) return;
    fprintf (stdout, "Using iteration %d\n", newValue);
    CurrentLoop = newValue;
    return;
  }

  // automatic iteration:
  CurrentLoop = current;
}

int GetZptIteration (void) {
  return CurrentLoop;
}

void SetZeroPointModes (Catalog *catalog, int Ncatalog) {

  if (TGROUP_ZEROPT && MOSAIC_ZEROPT) {
    if (CurrentLoop < 6) {
      TGROUP_ZPT_MODE = TGROUP_ZPT_MODE_ALL;
      IMAGE_ZPT_MODE  = IMAGE_ZPT_MODE_NONE;
      MOSAIC_ZPT_MODE = MOSAIC_ZPT_MODE_NONE;
    }
    if ((CurrentLoop >= 6) && (CurrentLoop < 12)) {
      // after iterating a few times on the TGroup zero points:
      // * identify the photometric nights
      // * fit the mosaics from the non-photometric nights
      TGROUP_ZPT_MODE = TGROUP_ZPT_MODE_GOOD_NIGHT; // stop fitting the bad nights
      MOSAIC_ZPT_MODE = MOSAIC_ZPT_MODE_BAD_NIGHT;
      IMAGE_ZPT_MODE  = IMAGE_ZPT_MODE_NONE;
      clean_tgroups(); // do this on every pass to update the status of nights 
    }
    if ((CurrentLoop >= 12) && (CurrentLoop < 18)) {
      // after iterating a few times on the TGroup & Mosaic zero points:
      // * identify the good / bad Mosaics
      // * fit the chips from the bad Mosaics
      TGROUP_ZPT_MODE = TGROUP_ZPT_MODE_GOOD_NIGHT; // stop fitting the bad nights
      MOSAIC_ZPT_MODE = MOSAIC_ZPT_MODE_BAD_NIGHT_GOOD_MOSAIC; // only fit good mosaics on bad nights
      IMAGE_ZPT_MODE  = IMAGE_ZPT_MODE_BAD_NIGHT_BAD_MOSAIC; // only fit bad mosaics on bad nights
      clean_tgroups(); // do this on every pass 
      clean_mosaics(); // do this on every pass 
    }
    if ((CurrentLoop >= 18) && (CurrentLoop < 21)) {
      // after iterating a few times on the TGroup & Mosaic zero points:
      // * identify the good / bad Mosaics
      // * fit the chips from the bad Mosaics
      TGROUP_ZPT_MODE = TGROUP_ZPT_MODE_GOOD_NIGHT; // stop fitting the bad nights
      MOSAIC_ZPT_MODE = MOSAIC_ZPT_MODE_BAD_NIGHT_GOOD_MOSAIC; // only fit good mosaics on bad nights
      IMAGE_ZPT_MODE  = IMAGE_ZPT_MODE_BAD_NIGHT_BAD_MOSAIC; // only fit bad mosaics on bad nights
      clean_tgroups(); // do this on every pass 
      clean_mosaics(); // do this on every pass 
      clean_images(); // do this on every pass 
      clean_stars(catalog, Ncatalog); // do this on every pass 
    }
    if ((CurrentLoop >= 21) && (CurrentLoop <= 999)) {
      // after iterating a few times on the TGroup & Mosaic zero points:
      // * identify the good / bad Mosaics
      // * fit the chips from the bad Mosaics
      TGROUP_ZPT_MODE = TGROUP_ZPT_MODE_GOOD_NIGHT; // stop fitting the bad nights
      MOSAIC_ZPT_MODE = MOSAIC_ZPT_MODE_BAD_NIGHT_GOOD_MOSAIC; // only fit good mosaics on bad nights
      IMAGE_ZPT_MODE  = IMAGE_ZPT_MODE_BAD_NIGHT_BAD_MOSAIC; // only fit bad mosaics on bad nights
      GRID_ZPT_MODE  = GRID_ZPT_MODE_ALL; // only fit bad mosaics on bad nights
      clean_tgroups(); // do this on every pass 
      clean_mosaics(); // do this on every pass 
      clean_images(); // do this on every pass 
      clean_stars(catalog, Ncatalog); // do this on every pass 
    }
  }

  if (TGROUP_ZEROPT && !MOSAIC_ZEROPT) {
    if (CurrentLoop <= 4) {
      TGROUP_ZPT_MODE = TGROUP_ZPT_MODE_ALL;
      IMAGE_ZPT_MODE  = IMAGE_ZPT_MODE_NONE;
      MOSAIC_ZPT_MODE = MOSAIC_ZPT_MODE_NONE;
    }
    if ((CurrentLoop > 4) && (CurrentLoop <= 8)) {
      // after iterating a few times on the TGroup zero points:
      // * identify the photometric nights
      // * fit the images from the non-photometric nights
      TGROUP_ZPT_MODE = TGROUP_ZPT_MODE_GOOD_NIGHT; // stop fitting the bad nights
      MOSAIC_ZPT_MODE = MOSAIC_ZPT_MODE_NONE;
      IMAGE_ZPT_MODE  = IMAGE_ZPT_MODE_BAD_NIGHT;
      clean_tgroups(); // do this on every pass or just sometimes?
    }
    if ((CurrentLoop > 8) && (CurrentLoop <= 999)) {
      // after iterating a few times on the TGroup zero points:
      // * identify the photometric nights
      // * fit the images from the non-photometric nights
      TGROUP_ZPT_MODE = TGROUP_ZPT_MODE_GOOD_NIGHT; // stop fitting the bad nights
      MOSAIC_ZPT_MODE = MOSAIC_ZPT_MODE_NONE;
      IMAGE_ZPT_MODE  = IMAGE_ZPT_MODE_BAD_NIGHT;
      GRID_ZPT_MODE  = GRID_ZPT_MODE_ALL; // only fit bad mosaics on bad nights
      clean_tgroups(); // do this on every pass or just sometimes?
    }
  }

  if (!TGROUP_ZEROPT && MOSAIC_ZEROPT) {
    if (CurrentLoop < 6) {
      TGROUP_ZPT_MODE = TGROUP_ZPT_MODE_NONE;
      MOSAIC_ZPT_MODE = MOSAIC_ZPT_MODE_ALL;
      IMAGE_ZPT_MODE  = IMAGE_ZPT_MODE_NONE;
    }
    if ((CurrentLoop >= 6) && (CurrentLoop < 12)) {
      // after iterating a few times on the Mosaic zero points:
      // * identify the good / bad mosaics
      // * fit the images from the bad mosaics
      TGROUP_ZPT_MODE = TGROUP_ZPT_MODE_NONE; // stop fitting the bad nights
      MOSAIC_ZPT_MODE = MOSAIC_ZPT_MODE_GOOD_MOSAIC;
      IMAGE_ZPT_MODE  = IMAGE_ZPT_MODE_BAD_MOSAIC;
      clean_mosaics(); // do this on every pass or just sometimes?
    }
    if ((CurrentLoop >= 12) && (CurrentLoop < 18)) {
      // after iterating a few times on the Mosaic zero points:
      // * identify the good / bad mosaics
      // * fit the images from the bad mosaics
      TGROUP_ZPT_MODE = TGROUP_ZPT_MODE_NONE; // stop fitting the bad nights
      MOSAIC_ZPT_MODE = MOSAIC_ZPT_MODE_GOOD_MOSAIC;
      IMAGE_ZPT_MODE  = IMAGE_ZPT_MODE_BAD_MOSAIC;
      clean_mosaics(); // do this on every pass or just sometimes?
      clean_images(); // do this on every pass 
      clean_stars(catalog, Ncatalog); // do this on every pass 
    }
    if ((CurrentLoop >= 18) && (CurrentLoop < 999)) {
      // after iterating a few times on the Mosaic zero points:
      // * identify the good / bad mosaics
      // * fit the images from the bad mosaics
      TGROUP_ZPT_MODE = TGROUP_ZPT_MODE_NONE; // stop fitting the bad nights
      MOSAIC_ZPT_MODE = MOSAIC_ZPT_MODE_GOOD_MOSAIC;
      IMAGE_ZPT_MODE  = IMAGE_ZPT_MODE_BAD_MOSAIC;
      GRID_ZPT_MODE  = GRID_ZPT_MODE_ALL; // only fit bad mosaics on bad nights
      clean_mosaics(); // do this on every pass or just sometimes?
      clean_images(); // do this on every pass 
      clean_stars(catalog, Ncatalog); // do this on every pass 
    }
  }

  if (!TGROUP_ZEROPT && !MOSAIC_ZEROPT) {
    // if we are not fitting TGROUP or MOSAIC, just fit all images
    TGROUP_ZPT_MODE = TGROUP_ZPT_MODE_NONE;
    MOSAIC_ZPT_MODE = MOSAIC_ZPT_MODE_NONE;
    IMAGE_ZPT_MODE  = IMAGE_ZPT_MODE_ALL;
  }
}

// mode = TGROUP, MOSAIC, IMAGE
// should we use IRLS or OLS?  depends on CurrentLoop and which stage we are in.
int UseStandardOLS (ZptFitModeType mode) {

  if (TGROUP_ZEROPT && MOSAIC_ZEROPT) {
    switch (mode) {
      case ZPT_STARS: 
	if (CurrentLoop < 3) return TRUE;
	return FALSE;
      case ZPT_TGROUP: 
	if (CurrentLoop < 3) return TRUE;
	return FALSE;
      case ZPT_MOSAIC:
	if (CurrentLoop < 9) return TRUE;
	return FALSE;
      case ZPT_IMAGES:
      default:
	if (CurrentLoop < 15) return TRUE;
	return FALSE;
    }
  }

  if (TGROUP_ZEROPT && !MOSAIC_ZEROPT) {
    switch (mode) {
      case ZPT_STARS: 
	if (CurrentLoop < 3) return TRUE;
	return FALSE;
      case ZPT_TGROUP: 
      case ZPT_MOSAIC:
	if (CurrentLoop < 3) return TRUE;
	return FALSE;
      case ZPT_IMAGES:
      default:
	if (CurrentLoop < 7) return TRUE;
	return FALSE;
    }
  }

  if (!TGROUP_ZEROPT && MOSAIC_ZEROPT) {
    switch (mode) {
      case ZPT_STARS: 
	if (CurrentLoop < 3) return TRUE;
	return FALSE;
      case ZPT_TGROUP: 
      case ZPT_MOSAIC:
	if (CurrentLoop < 3) return TRUE;
	return FALSE;
      case ZPT_IMAGES:
      default:
	if (CurrentLoop < 7) return TRUE;
	return FALSE;
    }
  }

  if (!TGROUP_ZEROPT && !MOSAIC_ZEROPT) {
    switch (mode) {
      case ZPT_STARS: 
	if (CurrentLoop < 3) return TRUE;
	return FALSE;
      case ZPT_TGROUP: 
      case ZPT_MOSAIC:
      case ZPT_IMAGES:
      default:
	if (CurrentLoop < 3) return TRUE;
	return FALSE;
    }
  }
  return FALSE;
}

/*  TGROUP & MOSAIC:
    0 - TGROUP ONLY & OLS
    1 - TGROUP ONLY & OLS
    2 - TGROUP ONLY & OLS

    3 - TGROUP ONLY & IRLS
    4 - TGROUP ONLY & IRLS
    5 - TGROUP ONLY & IRLS

    6 - TGROUP + MOSAIC & OLS MOSAIC
    7 - TGROUP + MOSAIC & OLS MOSAIC
    8 - TGROUP + MOSAIC & OLS MOSAIC

    9 - TGROUP + MOSAIC & IRLS MOSAIC
   10 - TGROUP + MOSAIC & IRLS MOSAIC
   11 - TGROUP + MOSAIC & IRLS MOSAIC

   12 - TGROUP + MOSAIC + IMAGE & OLS IMAGE
   13 - TGROUP + MOSAIC + IMAGE & OLS IMAGE
   14 - TGROUP + MOSAIC + IMAGE & OLS IMAGE

   15 - TGROUP + MOSAIC + IMAGE & IRLS IMAGE
   16 - TGROUP + MOSAIC + IMAGE & IRLS IMAGE
   17 - TGROUP + MOSAIC + IMAGE & IRLS IMAGE

*/


# include "addstar.h"

// examine the PHU of this file and determine the file mode
int GetFileMode (Header *header) {

  char ctype[81], ctmp[81];

  int Naxis;
  int simple, extend;
  int havePHOT_VER, haveTARG_VER;

  // NOTE target of %t must be int length
  gfits_scan_alt (header, "SIMPLE", "%t", 1, &simple);
  int haveNaxis = gfits_scan (header, "NAXIS",  "%d", 1, &Naxis);
  int haveCTYPE = gfits_scan (header, "CTYPE2", "%s", 1, ctype);

  gfits_scan_alt (header, "EXTEND", "%t", 1, &extend);
    
  // SDSS tsObj files have a version number for the PHOTO and 
  // TS (target selection) pipelines present as header keywords
  havePHOT_VER = gfits_scan (header, "PHOT_VER", "%s", 1, ctmp);
  haveTARG_VER = gfits_scan (header, "TARG_VER", "%s", 1, ctmp);
  if (havePHOT_VER && haveTARG_VER) {
    if (VERBOSE) fprintf (stderr, "found SDSS objects\n");
    return SDSS_OBJ;
  }

    
  // UKIRT files have TELESCOP = UKIRT and INSTRUME = WFCAM:
  char telescope[81], instrument[81];
  int have_TELESCOPE  = gfits_scan (header, "TELESCOP", "%s", 1, telescope);
  int have_INSTRUMENT = gfits_scan (header, "INSTRUME", "%s", 1, instrument);
  if (have_TELESCOPE && have_INSTRUMENT && !strcmp(telescope, "UKIRT") && !strcmp(instrument, "WFCAM")) {
    if (VERBOSE) fprintf (stderr, "found UKIRT data\n");
    return UKIRT_OBJ;
  }

  if (haveNaxis && (Naxis == 2)) {
    int Nx, Ny;
    gfits_scan (header, "NAXIS1",  "%d", 1, &Nx);
    gfits_scan (header, "NAXIS2",  "%d", 1, &Ny);
    if ((Nx > 0) && (Ny > 0)) {
      if (haveCTYPE && !strcmp (&ctype[4], "-WRP")) {
	if (VERBOSE) fprintf (stderr, "found MOSAIC CMP\n");
	return MOSAIC_CMP;
      }
      if (VERBOSE) fprintf (stderr, "found SIMPLE CMP\n");
      return SIMPLE_CMP;
    }
  }

  if (haveNaxis && (TEXTMODE || !simple)) {
    if (haveCTYPE && !strcmp (&ctype[4], "-WRP")) {
      if (VERBOSE) fprintf (stderr, "found MOSAIC CMP\n");
      return MOSAIC_CMP;
    }
    if (VERBOSE) fprintf (stderr, "found SIMPLE CMP\n");
    return SIMPLE_CMP;
  }

  if (!extend && strcmp (&ctype[4], "-DIS")) {
    if (!strcmp (&ctype[4], "-WRP")) {
      if (VERBOSE) fprintf (stderr, "found MOSAIC CMF\n");
      return MOSAIC_CMF;
    }
    if (VERBOSE) fprintf (stderr, "found SIMPLE CMF\n");
    return SIMPLE_CMF;
  }

  if (!extend && !strcmp (&ctype[4], "-DIS")) {
    if (VERBOSE) fprintf (stderr, "found MOSAIC PHU\n");
    return MOSAIC_PHU;
  }

  if (extend && strcmp (&ctype[4], "-DIS")) {
    if (VERBOSE) fprintf (stderr, "found SIMPLE MEF\n");
    return SIMPLE_MEF;
  }

  if (extend && !strcmp (&ctype[4], "-DIS")) {
    if (VERBOSE) fprintf (stderr, "found MOSAIC MEF\n");
    return MOSAIC_MEF;
  }

  if (VERBOSE) fprintf (stderr, "extension type is unknown\n");
  return (NONE);
}

# include "dvoImageOverlaps.h"

// examine the PHU of this file and determine the file mode
int GetFileMode (Header *header) {

  char ctype[80];
  int Naxis;
  int simple, extend, haveNaxis, haveCTYPE;

  // NOTE target of %t must be int length
  gfits_scan_alt (header, "SIMPLE", "%t", 1, &simple);
  haveNaxis = gfits_scan (header, "NAXIS",  "%d", 1, &Naxis);
  haveCTYPE = gfits_scan (header, "CTYPE2", "%s", 1, ctype);

  gfits_scan_alt (header, "EXTEND", "%t", 1, &extend);
    
  if (haveNaxis && ((Naxis == 2) || !simple)) {
    if (haveCTYPE && !strcmp (&ctype[4], "-WRP")) {
      return MOSAIC_CMP;
    }
    return SIMPLE_CMP;
  }

  if (!extend && strcmp (&ctype[4], "-DIS")) {
    if (!strcmp (&ctype[4], "-WRP")) {
      return MOSAIC_CMF;
    }
    return SIMPLE_CMF;
  }

  if (!extend && !strcmp (&ctype[4], "-DIS")) {
    return MOSAIC_PHU;
  }

  if (extend && strcmp (&ctype[4], "-DIS")) {
    return SIMPLE_MEF;
  }

  if (extend && !strcmp (&ctype[4], "-DIS")) {
    return MOSAIC_MEF;
  }

  return (NONE);
}

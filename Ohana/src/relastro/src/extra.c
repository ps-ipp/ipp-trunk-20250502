# include "relastro.h"

// for now (20140710) I need to identify gpc1 chips explicitly.  generalize in the future
int isGPC1chip (int photcode) {

  if ((photcode > 10000) && (photcode < 10077)) return TRUE; // g-band
  if ((photcode > 10100) && (photcode < 10177)) return TRUE; // r-band
  if ((photcode > 10200) && (photcode < 10277)) return TRUE; // i-band
  if ((photcode > 10300) && (photcode < 10377)) return TRUE; // z-band
  if ((photcode > 10400) && (photcode < 10477)) return TRUE; // y-band
  if ((photcode > 10500) && (photcode < 10577)) return TRUE; // w-band

  return FALSE;
}

// for now (20140710) I need to identify gpc1 stacks explicitly.  generalize in the future
int isGPC1stack (int photcode) {

  if (photcode == 11000) return TRUE; // g-band
  if (photcode == 11100) return TRUE; // r-band
  if (photcode == 11200) return TRUE; // i-band
  if (photcode == 11300) return TRUE; // z-band
  if (photcode == 11400) return TRUE; // y-band
  if (photcode == 11500) return TRUE; // w-band

  return FALSE;
}

// for now (20140710) I need to identify gpc1 stacks explicitly.  generalize in the future
int isGPC1warp (int photcode) {

  if (photcode == 12000) return TRUE; // g-band
  if (photcode == 12100) return TRUE; // r-band
  if (photcode == 12200) return TRUE; // i-band
  if (photcode == 12300) return TRUE; // z-band
  if (photcode == 12400) return TRUE; // y-band
  if (photcode == 12500) return TRUE; // w-band

  return FALSE;
}

// for now (20160925) I need to identify HSC chips explicitly.  generalize in the future
int isHSCchip (int photcode) {

  if ((photcode >= 20000) && (photcode <= 20111)) return TRUE; // g-band
  if ((photcode >= 21000) && (photcode <= 21111)) return TRUE; // r-band
  if ((photcode >= 22000) && (photcode <= 22111)) return TRUE; // i-band
  if ((photcode >= 23000) && (photcode <= 23111)) return TRUE; // z-band
  if ((photcode >= 24000) && (photcode <= 24111)) return TRUE; // y-band

  return FALSE;
}

// for now (20160925) I need to identify CFH chips explicitly.  generalize in the future
int isCFHchip (int photcode) {

  if ((photcode >= 100) && (photcode <= 152)) return TRUE; // g-band
  if ((photcode >= 200) && (photcode <= 252)) return TRUE; // r-band
  if ((photcode >= 300) && (photcode <= 352)) return TRUE; // i-band
  if ((photcode >= 400) && (photcode <= 452)) return TRUE; // z-band
  if ((photcode >= 500) && (photcode <= 552)) return TRUE; // y-band

  return FALSE;
}

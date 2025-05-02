# include "dvoshell.h"
# define D_NMATCH 500;

void compare (Catalog *catlog1, Catalog *catlog2, 
	      Vector *rvec,  Vector *dvec,  Vector *mvec, Vector *drvec, Vector *ddvec, Vector *dmvec, double radius) {

  off_t i, j, first_j, Nmatch, NMATCH;
  double dX, dY, dR;

  Nmatch = 0;
  NMATCH = D_NMATCH;
  ResetVector ( rvec, OPIHI_FLT, NMATCH);
  ResetVector ( dvec, OPIHI_FLT, NMATCH);
  ResetVector ( mvec, OPIHI_FLT, NMATCH);
  ResetVector (drvec, OPIHI_FLT, NMATCH);
  ResetVector (ddvec, OPIHI_FLT, NMATCH);
  ResetVector (dmvec, OPIHI_FLT, NMATCH);

  for (i = j = 0; (i < catlog1[0].Naverage) && (j < catlog2[0].Naverage);) {
    
    dX = catlog1[0].average[i].R - catlog2[0].average[j].R;

    if (!(i % 100))
      gprint (GP_ERR, ".");
    
    if (dX <= -radius)
      i++;
    if (dX >= radius)
      j++;

    if (fabs (dX) < radius) {
      first_j = j;
      for (j = first_j; (fabs (dX) < radius) && (j < catlog2[0].Naverage); j++) {
	dX = catlog1[0].average[i].R - catlog2[0].average[j].R;
	dY = catlog1[0].average[i].D - catlog2[0].average[j].D;
	dR = hypot (dX, dY);
	if (dR < radius) {
	  rvec[0].elements.Flt[Nmatch] = catlog1[0].average[i].R;
	  dvec[0].elements.Flt[Nmatch] = catlog1[0].average[i].D;
	  // mvec[0].elements.Flt[Nmatch] = catlog1[0].average[i].M;
	  drvec[0].elements.Flt[Nmatch] = dX;
	  ddvec[0].elements.Flt[Nmatch] = dY;
	  // dmvec[0].elements.Flt[Nmatch] = catlog1[0].average[i].M - catlog2[0].average[j].M;
	  Nmatch ++;
	  if (Nmatch == NMATCH - 1) {
	    NMATCH += D_NMATCH;
	    REALLOCATE ( rvec[0].elements.Flt, opihi_flt, NMATCH);
	    REALLOCATE ( dvec[0].elements.Flt, opihi_flt, NMATCH);
	    REALLOCATE ( mvec[0].elements.Flt, opihi_flt, NMATCH);
	    REALLOCATE (drvec[0].elements.Flt, opihi_flt, NMATCH);
	    REALLOCATE (ddvec[0].elements.Flt, opihi_flt, NMATCH);
	    REALLOCATE (dmvec[0].elements.Flt, opihi_flt, NMATCH);
	  }
	}
      }
      j = first_j;
      i++;
    }
  }

  ResetVector ( rvec, OPIHI_FLT, Nmatch);
  ResetVector ( dvec, OPIHI_FLT, Nmatch);
  ResetVector ( mvec, OPIHI_FLT, Nmatch);
  ResetVector (drvec, OPIHI_FLT, Nmatch);
  ResetVector (ddvec, OPIHI_FLT, Nmatch);
  ResetVector (dmvec, OPIHI_FLT, Nmatch);
}


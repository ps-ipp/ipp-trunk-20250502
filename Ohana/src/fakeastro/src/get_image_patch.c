# include "fakeastro.h"

SkyRegion *get_image_patch (Image *image) {

  SkyRegion *region;

  // find the R,D coords of the 4 corners and 4 edge midpoints

  static double Xpt[] = {0.0, 0.5, 1.0, 0.0, 1.0, 0.0, 0.5, 1.0};
  static double Ypt[] = {0.0, 0.0, 0.0, 0.5, 0.5, 1.0, 1.0, 1.0};

  double Rmin = +480.0;
  double Rmax = -360.0;
  double Dmin =  +90.0;
  double Dmax =  -90.0;

  int i;
  for (i = 0; i < 8; i++) {
    double R, D;
    XY_to_RD (&R, &D, Xpt[i]*image->NX, Ypt[i]*image->NY, &image->coords);

    // fprintf (stderr, "%f %f -> %f %f\n", Xpt[i]*image->NX, Ypt[i]*image->NY, R, D); 
    Rmin = MIN(R,Rmin);
    Rmax = MAX(R,Rmax);
    Dmin = MIN(D,Dmin);
    Dmax = MAX(D,Dmax);
  }

  ALLOCATE (region, SkyRegion, 1);

  region->Rmin = Rmin;
  region->Rmax = Rmax;
  region->Dmin = Dmin;
  region->Dmax = Dmax;

  // fprintf (stderr, "%f %f , %f %f\n", Rmin, Rmax, Dmin, Dmax);

  return region;
}

SkyRegion *get_mosaic_patch (Image *image) {

  SkyRegion *region;

  // find the R,D coords of the 4 corners and 4 edge midpoints

  static double Xpt[] = {-0.5,  0.0,  0.5, -0.5, 0.5, -0.5, 0.0, 0.5};
  static double Ypt[] = {-0.5, -0.5, -0.5,  0.0, 0.0,  0.5, 0.5, 0.5};

  double Rmin = +480.0;
  double Rmax = -360.0;
  double Dmin =  +90.0;
  double Dmax =  -90.0;

  int i;
  for (i = 0; i < 8; i++) {
    double R, D;
    XY_to_RD (&R, &D, Xpt[i]*image->NX, Ypt[i]*image->NY, &image->coords);

    // fprintf (stderr, "%f %f -> %f %f\n", Xpt[i]*image->NX, Ypt[i]*image->NY, R, D); 
    Rmin = MIN(R,Rmin);
    Rmax = MAX(R,Rmax);
    Dmin = MIN(D,Dmin);
    Dmax = MAX(D,Dmax);
  }

  ALLOCATE (region, SkyRegion, 1);

  region->Rmin = Rmin;
  region->Rmax = Rmax;
  region->Dmin = Dmin;
  region->Dmax = Dmax;

  // fprintf (stderr, "%f %f , %f %f\n", Rmin, Rmax, Dmin, Dmax);

  return region;
}

// region has region of Rmin <= R < Rmax, Dmin <= D < Dmax
int SkyRegionHasPoint (SkyRegion *region, double R, double D) {

  if (D <  region->Dmin) return FALSE;
  if (D >= region->Dmax) return FALSE;
  if (R <  region->Rmin) return FALSE;
  if (R >= region->Rmax) return FALSE;
  return TRUE;
}

int SkyRegionsOverlap (SkyRegion *region, SkyRegion *patch) {

  // what are the assumptions?  
  // * patch->Rmin,Rmax are on the same side of the boundary 
  //   * [-2,-1], [-2,+1], [358,361] : all OK
  //   * [358, 2] : NOT OK

  myAssert (patch->Rmin <= patch->Rmax, "impossible!");

  if (region->Dmin >= patch->Dmax) return FALSE;
  if (region->Dmax <= patch->Dmin) return FALSE;

  int keepR0 = FALSE;
  int keepR1 = FALSE;
  int keepR2 = FALSE;
  if (patch->Rmin < 0.0) {
    float Rmin = patch->Rmin + 360.0;
    float Rmax = patch->Rmax + 360.0;
    keepR0 = (Rmin <= region->Rmax) && (Rmax >= region->Rmin);
  }
  if (patch->Rmax > 360.0) {
    float Rmin = patch->Rmin - 360.0;
    float Rmax = patch->Rmax - 360.0;
    keepR1 = (Rmin <= region->Rmax) && (Rmax >= region->Rmin);
  }
  keepR2 = (patch->Rmin <= region->Rmax) && (patch->Rmax >= region->Rmin);

  int keep = keepR0 || keepR1 || keepR2;

  if (!keep) return FALSE;
  return TRUE;
}

SkyRegion *SkyRegionExpand (SkyRegion *region, float boundary) {

  SkyRegion *output = NULL;
  ALLOCATE (output, SkyRegion, 1);

  output->Dmin = MAX (-90.0, region->Dmin - boundary);
  output->Dmax = MIN (+90.0, region->Dmax + boundary);

  if ((region->Rmax == 360.0) && (region->Rmin == 0.0)) {
    output->Rmin = region->Rmin;
    output->Rmax = region->Rmax;
    return output;
  }

  float dRmin = boundary / cos (RAD_DEG*region->Dmin);
  float dRmax = boundary / cos (RAD_DEG*region->Dmin);

  float dR = MAX (dRmin, dRmax);

  output->Rmin = region->Rmin - dR;
  output->Rmax = region->Rmax + dR;

  return output;
}

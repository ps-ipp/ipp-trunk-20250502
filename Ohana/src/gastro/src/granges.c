# include "gastro.h"

void granges (SStars *stars1, SStars *stars2, int N1, int N2, int NPIX, double *gx, double *gy, double *gx0, double *gy0) {

  int i;
  double maxX1, minX1, maxY1, minY1;
  double maxX2, minX2, maxY2, minY2;
  double Xzero, Xrange, Yzero, Yrange;

  maxX1 = minX1 = stars1[0].X;
  maxY1 = minY1 = stars1[0].Y;
  for (i = 0; i < N1; i++) {
    maxX1 = MAX (maxX1, stars1[i].X);    
    minX1 = MIN (minX1, stars1[i].X);    
    maxY1 = MAX (maxY1, stars1[i].Y);    
    minY1 = MIN (minY1, stars1[i].Y);    
  }

  maxX2 = minX2 = stars2[0].X;
  maxY2 = minY2 = stars2[0].Y;
  for (i = 0; i < N2; i++) {
    maxX2 = MAX (maxX2, (stars2[i].X));    
    minX2 = MIN (minX2, (stars2[i].X));    
    maxY2 = MAX (maxY2, (stars2[i].Y));    
    minY2 = MIN (minY2, (stars2[i].Y));    
  }

  Xzero = minX1 - maxX2;
  Yzero = minY1 - maxY2;
  Xrange = ((maxX1 - minX1) + (maxX2 - minX2));
  Yrange = ((maxY1 - minY1) + (maxY2 - minY2));
  
  *gx = (NPIX - 1.0) / Xrange;
  *gy = (NPIX - 1.0) / Yrange;
  *gx0  = (1.0 - NPIX)*Xzero/Xrange;
  *gy0  = (1.0 - NPIX)*Yzero/Yrange;

  fprintf (stderr, "gx, gy: %f %f\n", *gx, *gy);

}

# include "markstar.h"

gcatstats (catalog, catstats)
Catalog catalog[];
CatStats catstats[];
{

  off_t i;
  double RaCenter, DecCenter;
  double MinRA, MaxRA, MinDEC, MaxDEC;
  double *X1, *Y1;
  Coords tcoords;
  
  gfits_scan (&catalog[0].header, "RA0", "%lf", 1, &MinRA);
  gfits_scan (&catalog[0].header, "RA1", "%lf", 1, &MaxRA);
  gfits_scan (&catalog[0].header, "DEC0", "%lf", 1, &MinDEC);
  gfits_scan (&catalog[0].header, "DEC1", "%lf", 1, &MaxDEC);

  /* double check on region RA and DEC ranges */
  DecCenter = 0.5*(MinDEC + MaxDEC);
  RaCenter = 0.5*(MinRA + MaxRA);
  if (MaxDEC > 86.25) {  /* we are on the pole */
    DecCenter = 90.0;
    RaCenter = 0.0;
  }

  catstats[0].RA[0] = MinRA;
  catstats[0].RA[1] = MaxRA;
  catstats[0].DEC[0] = MinDEC;
  catstats[0].DEC[1] = MaxDEC;
  catstats[0].Area = (MaxDEC - MinDEC)*(MaxRA - MinRA) / cos (RAD_DEG*DecCenter);
  if (MaxDEC > 86.25) {  /* we are on the pole */
    catstats[0].Area = 44.2;
  }
  catstats[0].density = catalog[0].Naverage / (3600*3600*catstats[0].Area);
  catstats[0].spacing = 1.0 / (NSIGMA * catstats[0].density * 2*TRAIL_WIDTH);
  fprintf (stderr, "Area, density, spacing: %f %f %f\n", catstats[0].Area, 
	   catstats[0].density, catstats[0].spacing);
  /* number of stars per square arcsec */

  /** allocate local arrays **/
  ALLOCATE (catstats[0].X, double, catalog[0].Naverage);
  ALLOCATE (catstats[0].Y, double, catalog[0].Naverage);
  ALLOCATE (catstats[0].N, int,   catalog[0].Naverage);

  /* project onto rectilinear grid with 1 arcsec pixels, sort by X */
  /* reference for coords is center of field  */
  InitCoords (&catstats[0].coords, "DEC--TAN");
  catstats[0].coords.crval1 = RaCenter;
  catstats[0].coords.crval2 = DecCenter;
  catstats[0].coords.cdelt1 = catstats[0].coords.cdelt2 = 1.0 / 3600.0;

  X1 = catstats[0].X;
  Y1 = catstats[0].Y;
  for (i = 0; i < catalog[0].Naverage; i++, X1++, Y1++) {
    RD_to_XY (X1, Y1, catalog[0].average[i].R, catalog[0].average[i].D, &catstats[0].coords);
    catstats[0].N[i] = i;
  }
  if (catalog[0].Naverage > 1) sort_coords_index (catstats[0].X, catstats[0].Y, catstats[0].N, catalog[0].Naverage);
  
}


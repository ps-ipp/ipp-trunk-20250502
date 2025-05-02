# include "dvo.h"
# include "dvodb.h"
# include "get_graphdata.h"

void image_subset (Image *image, off_t Nimage, off_t **Subset, off_t *Nsubset,
		   SkyRegionSelection *selection, 
		   e_time tzero, double trange, int TimeSelect) 
{

  int j, flipped, status;
  off_t i, n, *subset;
  int npts;
  double r, d, X, Y, x[4], y[4], Rmin, Rmax, Rmid;
  Graphdata graph;
  SkyRegion patch;

  Rmin = Rmax = Rmid = 0;

  if (selection->useDisplay) {
    if (!GetGraphdata (&graph, NULL, NULL)) {
      gprint (GP_ERR, "region display not available\n");
      return;
    }
    Rmin = graph.coords.crval1 - 182.0;
    Rmax = graph.coords.crval1 + 182.0;
    Rmid = 0.5*(Rmin + Rmax);
    BuildChipMatch (image, Nimage);
  }

  patch.Rmin = 0;
  patch.Rmax = 0;
  patch.Dmin = 0;
  patch.Dmax = 0;
  if (selection->useSkyregion) {
    double Rs, Re, Ds, De;
    get_skyregion (&Rs, &Re, &Ds, &De);
    patch.Rmin = Rs;
    patch.Rmax = Re;
    patch.Dmin = Ds;
    patch.Dmax = De;
    Rmin = patch.Rmin - 182.0;
    Rmax = patch.Rmax + 182.0;
    Rmid = 0.5*(Rmin + Rmax);
  }

  if (trange < 0) {
    tzero = tzero + trange;
    trange = fabs (trange);
  }

  npts = 200;
  ALLOCATE (subset, off_t, npts);
  n = 0;
  for (i = 0; i < Nimage; i++) {
    if (TimeSelect && ((image[i].tzero < tzero) || (image[i].tzero+image[i].trate*image[i].NY > tzero + trange))) {
      // fprintf (stderr, "skipping %s\n", image[i].name);
      continue;
    }
    if (selection->useDisplay) {
      // first check if region center is in image
      status = RD_to_XY (&X, &Y, Rmid, graph.coords.crval2, &image[i].coords);
      if (status && (X >= 0) && (X < image[i].NX) && (Y >= 0) && (Y < image[i].NY)) goto in_region;

      /* project this image to screen display coords */
      x[0] = 0;           y[0] = 0;
      x[1] = image[i].NX; y[1] = 0;
      x[2] = image[i].NX; y[2] = image[i].NY;
      x[3] = 0;           y[3] = image[i].NY;
      flipped = FALSE;
      for (j = 0; j < 4; j++) {
	XY_to_RD (&r, &d, x[j], y[j], &image[i].coords);
	/* use same side of 0,360 boundary for all corners */
	if ((j == 0) && (r < Rmin)) flipped = TRUE; 
	if ((j == 0) && (r > Rmax)) flipped = TRUE; 
	r = ohana_normalize_angle (r);
	while (flipped && (r < Rmid)) r+= 360.0;
	while (flipped && (r > Rmid)) r-= 360.0;
	status = RD_to_XY (&X, &Y, r, d, &graph.coords);
	if (!status) continue;
	if (X < graph.xmin) continue;
	if (X > graph.xmax) continue;
	if (Y < graph.ymin) continue;
	if (Y > graph.ymax) continue;
	goto in_region;
	/** we miss any images which surround the region.  we are also
	    missing the DIS images for which the corners don't touch
	    the region, but which are needed for WRP images with
	    corners touching the region **/
      }
      // fprintf (stderr, "skipping %s\n", image[i].name);
      continue;
    }
    if (selection->useSkyregion) {
      /* project this image to screen display coords */
      x[0] = 0;           y[0] = 0;
      x[1] = image[i].NX; y[1] = 0;
      x[2] = image[i].NX; y[2] = image[i].NY;
      x[3] = 0;           y[3] = image[i].NY;
      flipped = FALSE;
      for (j = 0; j < 4; j++) {
	XY_to_RD (&r, &d, x[j], y[j], &image[i].coords);
	/* use same side of 0,360 boundary for all corners */
	if ((j == 0) && (r < Rmin)) flipped = TRUE; 
	if ((j == 0) && (r > Rmax)) flipped = TRUE; 
	while (flipped && (r < Rmid)) r+= 360.0;
	while (flipped && (r > Rmid)) r-= 360.0;
	if (r < patch.Rmin) continue;
	if (r > patch.Rmax) continue;
	if (d < patch.Dmin) continue;
	if (d > patch.Dmax) continue;
	goto in_region;
	/** we miss any images which surround the region.  we are also
	    missing the DIS images for which the corners don't touch
	    the region, but which are needed for WRP images with
	    corners touching the region **/
      }
      // fprintf (stderr, "skipping %s\n", image[i].name);
      continue;
    }
  in_region:
    subset[n] = i;
    n++;
    if (n > npts - 1) {
      npts += 200;
      REALLOCATE (subset, off_t, npts);
    }
  }

  REALLOCATE (subset, off_t, MAX (n, 1));
  *Subset = subset;
  *Nsubset = n;
  return;
}

/* this routine fills the subset index with the list of selected images.
   images may be selected on the basis of the region or on a time range 
*/

# include "dvoshell.h"
extern double drand48();

int imdense (int argc, char **argv) {
  
  off_t i, Nimage;
  int kapa, N, status, NPTS;
  double r, d, x, y, Rmin, Rmax;
  Vector Xvec, Yvec;
  Image *image;
  Graphdata graphmode;

  if (!style_args (&graphmode, &argc, argv, &kapa)) return FALSE;

  /* need options to list only images in region and only images in a time range */
  /* also, option to list and not plot or plot and not list images */
  if (argc != 1) {
    gprint (GP_ERR, "USAGE: image\n");
    return (FALSE);
  }

  if ((image = LoadImagesDVO (&Nimage)) == NULL) return (FALSE);
  // BuildChipMatch (image, Nimage);

  Rmin = graphmode.coords.crval1 - 182.0;
  Rmax = graphmode.coords.crval1 + 182.0;
  
  // srand48() is called by startup.c

  N = 0;
  NPTS = 200;
  SetVector (&Xvec, OPIHI_FLT, NPTS);
  SetVector (&Yvec, OPIHI_FLT, NPTS);

  for (i = 0; i < Nimage; i++) {
    /* choose a position for point within image box */
    x = (0.1 + 0.9*drand48()) * image[i].NX;
    y = (0.1 + 0.9*drand48()) * image[i].NY;
    /* project this image to screen display coords */
    status = FALSE;
    XY_to_RD (&r, &d, x, y, &image[i].coords);
    r = ohana_normalize_angle (r);
    while (r < Rmin) r += 360.0; 
    while (r > Rmax) r -= 360.0; 
    status |= RD_to_XY (&Xvec.elements.Flt[N], &Yvec.elements.Flt[N], r, d, &graphmode.coords);
    if ((Xvec.elements.Flt[N] >= graphmode.xmin) && 
	(Xvec.elements.Flt[N] <= graphmode.xmax) && 
	(Yvec.elements.Flt[N] >= graphmode.ymin) && 
	(Yvec.elements.Flt[N] <= graphmode.ymax) && status) {
      N++;
      if (N > NPTS - 1) { 
	NPTS += 200;
	REALLOCATE (Xvec.elements.Flt, opihi_flt, NPTS);
	REALLOCATE (Yvec.elements.Flt, opihi_flt, NPTS);
      }
    }
  }

  Xvec.Nelements = Yvec.Nelements = N;
  if (N > 0) {
    graphmode.style = KAPA_PLOT_POINTS; /* points */
    graphmode.etype = 0;
    PlotVectorPair (kapa, &Xvec, &Yvec, NULL, &graphmode);
  }

  free (Xvec.elements.Flt);
  free (Yvec.elements.Flt);
  FreeImagesDVO (image);
  return (TRUE);

}



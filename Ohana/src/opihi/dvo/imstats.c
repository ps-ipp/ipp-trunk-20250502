# include "dvoshell.h"

int imstats (int argc, char **argv) {
  
  off_t i, Nimage;
  int kapa, N;
  int Mcal, AutoLimits;
  double r, d;
  Image *image;
  Vector Xvec, Yvec;  
  Graphdata graphmode;

  if (!style_args (&graphmode, &argc, argv, &kapa)) return (FALSE);

  Mcal = TRUE;
  if ((N = get_argument (argc, argv, "-dM"))) {
    remove_argument (N, &argc, argv);
    Mcal = FALSE;
  }

  AutoLimits = FALSE;
  if ((N = get_argument (argc, argv, "-l"))) {
    remove_argument (N, &argc, argv);
    AutoLimits = TRUE;
  }

  if (argc != 1) {
    gprint (GP_ERR, "USAGE: imstats [-dM] [-l]\n");
    return (FALSE);
  }

  if ((image = LoadImagesDVO (&Nimage)) == NULL) return (FALSE);
  // BuildChipMatch (image, Nimage);

  /* assign vector values */
  SetVector (&Xvec, OPIHI_FLT, Nimage);
  SetVector (&Yvec, OPIHI_FLT, Nimage);

  gprint (GP_LOG, "seq  ra (J2000) dec    time (s)   Nstars\n");
  for (i = 0; i < Nimage; i++) {
    Xvec.elements.Flt[i] = image[i].secz;
    if (Mcal) 
      Yvec.elements.Flt[i] = image[i].McalPSF;
    else 
      Yvec.elements.Flt[i] = image[i].dMcal;
    XY_to_RD (&r, &d, 0.5*image[i].NX, 0.5*image[i].NY, &image[i].coords);
    gprint (GP_ERR, "%d %8.4f %8.4f %10d %6d  %5.3f %6.3f %6.3f\n", 
	     i, r, d, image[i].tzero, image[i].nstar, Xvec.elements.Flt[i], 
	     image[i].McalPSF, image[i].dMcal);
  } 
  if (AutoLimits) SetLimits (&Xvec, &Yvec, &graphmode);

  graphmode.style = KAPA_PLOT_POINTS; /* points */
  graphmode.etype = 0;
  PlotVectorPair (kapa, &Xvec, &Yvec, NULL, &graphmode);
  
  free (Xvec.elements.Flt);
  free (Yvec.elements.Flt);
  FreeImagesDVO (image);
  return (TRUE);
}


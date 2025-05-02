# include "relastro.h"
   
void plot_scatter (Catalog *catalog, int Ncatalog) {
  OHANA_UNUSED_PARAM(catalog);
  OHANA_UNUSED_PARAM(Ncatalog);

# if (0)   
  int i, j, k, m, N, Ntot, Nsecfilt;
  float Mrel, Mcal, Mmos, Mgrid;
  double *xlist, *ylist, *ilist;
  Graphdata graphdata;

  // XXX in the future, use catalog[0].Nsecfilt only?  allow catalogs to have variable Nsecfilt?
  Nsecfilt = GetPhotcodeNsecfilt ();
  assert (catalog[0].Nsecfilt == Nsecfilt);

  Ntot = 0;
  for (i = 0; i < Ncatalog; i++) {
    for (j = 0; j < catalog[i].Naverage; j++) {
      Ntot += catalog[i].average[j].Nmeasure;
    }
  }
  ALLOCATE (xlist, double, Ntot);
  ALLOCATE (ylist, double, Ntot);
  ALLOCATE (ilist, double, Ntot);

  N = 0;
  for (i = 0; i < Ncatalog; i++) {
    for (j = 0; j < catalog[i].Naverage; j++) {

      /* calculate the average value for a single star */
      if (catalog[i].average[j].code & STAR_BAD) continue;  
      m = catalog[i].average[j].measureOffset;

      for (k = 0; k < catalog[i].average[j].Nmeasure; k++, m++) {
	if (catalog[i].measure[m].flags & MEAS_BAD) continue;

	Mrel = catalog[i].secfilt[Nsecfilt*j+PhotSec].M;
	xlist[N] = Mrel;
	ylist[N] = PhotSys  (&catalog[i].measure[m], &catalog[i].average[j], &catalog[i].secfilt[j*Nsecfilt]) - Mcal - Mmos - Mgrid - Mrel;
	ilist[N] = PhotInst (&catalog[i].measure[m]);
	N++;
      }
    }
  }

  plot_defaults (&graphdata);
  graphdata.xmin = PlotMmin;
  graphdata.xmax = PlotMmax;
  graphdata.ymin = PlotdMmin;
  graphdata.ymax = PlotdMmax;
  plot_list (&graphdata, xlist, ylist, N, "mag vs dmag", "Mag.png");

  plot_defaults (&graphdata);
  graphdata.ymin = PlotdMmin;
  graphdata.ymax = PlotdMmax;
  plot_list (&graphdata, ilist, ylist, N, "imag vs dmag", "iMag.png");
  free (xlist);
  free (ylist);
  free (ilist);

# endif
}

/* XXX this should become astrometrically relevant */

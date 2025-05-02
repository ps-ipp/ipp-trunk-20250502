# include "relphot.h"
   
void plot_scatter (Catalog *catalog, int Ncatalog) {

  off_t i, j, k, m, N, Ntot;
  float Mrel, Mcal, Mmos, Mgrid;
  double *xlist, *ylist, *ilist;
  Graphdata graphdata;

  Ntot = 0;
  for (i = 0; i < Ncatalog; i++) {
    for (j = 0; j < catalog[i].Naverage; j++) {
      Ntot += catalog[i].averageT[j].Nmeasure;
    }
  }
  ALLOCATE (xlist, double, Ntot);
  ALLOCATE (ylist, double, Ntot);
  ALLOCATE (ilist, double, Ntot);

  int Nsecfilt = GetPhotcodeNsecfilt ();

  int Ns;
  for (Ns = 0; Ns < Nphotcodes; Ns++) {

    int thisCode = photcodes[Ns][0].code;
    int Nsec = GetPhotcodeNsec(thisCode);

    N = 0;
    for (i = 0; i < Ncatalog; i++) {
      for (j = 0; j < catalog[i].Naverage; j++) {

	/* calculate the average value for a single star */
	// if (catalog[i].secfilt[Nsecfilt*j+Nsec].flags & STAR_BAD) continue;  
	m = catalog[i].averageT[j].measureOffset;

	for (k = 0; k < catalog[i].averageT[j].Nmeasure; k++, m++) {
	  // skip measurements that do not match the current photcode
	  int ecode = GetPhotcodeEquivCodebyCode (catalog[i].measureT[m].photcode);
	  if (ecode != thisCode) { continue; }

	  if (catalog[i].measureT[m].dbFlags & MEAS_BAD) continue;
	  Mcal = getMcal  (m, i, MAG_CLASS_PSF);
	  if (isnan(Mcal)) continue;
	  Mmos = getMmos  (m, i);
	  if (isnan(Mmos)) continue;
	  Mgrid = getMgridTiny (&catalog[i].measureT[m]);
	  if (isnan(Mgrid)) continue;

	  Mrel = catalog[i].secfilt[Nsecfilt*j+Nsec].MpsfChp;
	  if (isnan(Mrel)) continue;

	  xlist[N] = Mrel;
	  ylist[N] = PhotSysTiny (&catalog[i].measureT[m], &catalog[i].averageT[j], &catalog[i].secfilt[j*Nsecfilt], MAG_CLASS_PSF) - Mcal - Mmos - Mgrid - Mrel;
	  ilist[N] = PhotInstTiny (&catalog[i].measureT[m], MAG_CLASS_PSF);
	  N++;
	}
      }
    }

    if (N == 0) {
      fprintf (stderr, "no valid average values yet\n");
      continue;
    }

    plot_defaults (&graphdata);
    graphdata.xmin = PlotMmin;
    graphdata.xmax = PlotMmax;
    graphdata.ymin = PlotdMmin;
    graphdata.ymax = PlotdMmax;
    plot_list (&graphdata, xlist, ylist, N, "mag vs dmag", "%s.Mag.png", OUTROOT);

    plot_defaults (&graphdata);
    graphdata.ymin = PlotdMmin;
    graphdata.ymax = PlotdMmax;
    plot_list (&graphdata, ilist, ylist, N, "imag vs dmag", "%s.iMag.png", OUTROOT);
  }
  free (xlist);
  free (ylist);
  free (ilist);
}

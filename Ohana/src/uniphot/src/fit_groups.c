# include "uniphot.h"

void fit_tgroup (Group *tgroup, int Ntgroup) {

  int i, j, Nlist;
  float Mcal, Mgrp;
  double *mlist, *dlist;
  StatType stats;
  Group *sgroup;

  Nlist = tgroup[0].Nimage;
  for (i = 0; i < Ntgroup; i++) { Nlist = MAX (Nlist, tgroup[i].Nimage); }
  ALLOCATE (mlist, double, Nlist);
  ALLOCATE (dlist, double, Nlist);

  for (i = 0; i < Ntgroup; i++) {
    for (j = Nlist = 0; j < tgroup[i].Nimage; j++) {
      if (tgroup[i].image[j][0].flags & IMAGE_BAD) continue;
      sgroup = (Group *) tgroup[i].imlink[j][0].sgroup;
      Mcal = tgroup[i].image[j][0].McalPSF;
      Mgrp = sgroup[0].M;
      mlist[Nlist] = (Mcal - Mgrp);
      dlist[Nlist] = tgroup[i].image[j][0].dMcal;
      Nlist ++;
    }
    liststats (mlist, dlist, Nlist, &stats);
    tgroup[i].M  = stats.mean;
    tgroup[i].dMsub = stats.sigma;
    tgroup[i].Ngood = stats.Nmeas;
    
    // fprintf (stderr, "tgroup %d : %f +/- %f : %d stars\n", i, stats.mean, stats.sigma, stats.Nmeas);

    initstats ("MEAN");
    liststats (mlist, dlist, Nlist, &stats);
    tgroup[i].dM = stats.sigma;
    initstats (STATMODE);
  }
  free (mlist);
  free (dlist);
}

void fit_sgroup (Group *sgroup, int Nsgroup) {

  int i, j, Nlist;
  float Mcal, Mgrp;
  double *mlist, *dlist;
  StatType stats;
  Group *tgroup;
  
  Nlist = sgroup[0].Nimage;
  for (i = 0; i < Nsgroup; i++) { Nlist = MAX (Nlist, sgroup[i].Nimage); }
  ALLOCATE (mlist, double, Nlist);
  ALLOCATE (dlist, double, Nlist);
  Nlist = 0;

  for (i = 0; i < Nsgroup; i++) {
    for (j = Nlist = 0; j < sgroup[i].Nimage; j++) {
      if (sgroup[i].image[j][0].flags & IMAGE_BAD) continue;
      tgroup = (Group *) sgroup[i].imlink[j][0].tgroup;
      Mcal = sgroup[i].image[j][0].McalPSF;
      Mgrp = tgroup[0].M;
      mlist[Nlist] = (Mcal - Mgrp);
      dlist[Nlist] = sgroup[i].image[j][0].dMcal;
      Nlist ++;
    }
    liststats (mlist, dlist, Nlist, &stats);
    sgroup[i].M  = stats.mean;
    sgroup[i].dMsub = stats.sigma;
    sgroup[i].Ngood = stats.Nmeas;

    // fprintf (stderr, "sgroup %d : %f +/- %f : %d stars\n", i, stats.mean, stats.sigma, stats.Nmeas);

    initstats ("MEAN");
    liststats (mlist, dlist, Nlist, &stats);
    sgroup[i].dM = stats.sigma;
    initstats (STATMODE);
  }
  free (mlist);
  free (dlist);
}

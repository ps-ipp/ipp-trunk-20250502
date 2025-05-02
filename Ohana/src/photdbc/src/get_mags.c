# include "photdbc.h"

void get_mags (Catalog *catalog) {

  int i, j, m, n, N;
  int Nlist, Nsec, Nsecfilt, Ncode;
  short int *Mval;
  int Photcode, BAD_MEASURE;
  double *list, *dlist;
  StatType stats;

  if (VERBOSE) fprintf (stderr, "re-calculating photometry\n");

  BAD_MEASURE = FLAG_AREA | FLAG_MINST | FLAG_SIGCLIP | FLAG_CHISQ | FLAG_TOOFEW;
  Nsecfilt = catalog[0].Nsecfilt;

  /* allocate a list for temp storage of mag values */
  Nlist = 0;
  for (i = 0; i < catalog[0].Naverage; i++) {
    Nlist = MAX (Nlist, catalog[0].average[i].Nm);
    for (j = 0; j < Nsecfilt; j++) {
      catalog[0].secfilt[i*Nsecfilt+j].MpsfChp  = NAN;
      catalog[0].secfilt[i*Nsecfilt+j].dMpsfChp = NAN;
      catalog[0].secfilt[i*Nsecfilt+j].Mchisq   = NAN;
    }
  }
  ALLOCATE (list, double, Nlist);
  ALLOCATE (dlist, double, Nlist);
  initstats ("WT_MEAN");

  /* check on photcode */
  for (n = 0; n < photcodes.Ncode; n++) {
    if (photcodes.code[n].type == PHOT_DEP) continue;
    if (photcodes.code[n].type == PHOT_REF) continue;
    Photcode = photcodes.code[n].code;
    Nsec = photcodes.hashNsec[Photcode];

    /* find average magnitudes */
    for (i = 0; i < catalog[0].Naverage; i++) {
      N = 0;
      m = catalog[0].average[i].offset;
      for (j = 0; j < catalog[0].average[i].Nm; j++, m++) {
	if ((Ncode = photcodes.hashcode[catalog[0].measure[m].photcode]) < 0) continue;
	if (photcodes.code[Ncode].equiv != Photcode) continue;
	if (photcodes.code[Ncode].type != PHOT_DEP) continue;
	if (catalog[0].measure[m].flags & BAD_MEASURE) continue;
	list[N] = PhotRel (&catalog[0].measure[m], &catalog[0].average[i],  &catalog[0].secfilt[i*Nsecfilt]);
	dlist[N] = sqrt (SQ(DMGAIN*catalog[0].measure[m].dM) + DMSYS);
	N++;
      }
      liststats (list, dlist, N, &stats);
      if (N < 1) continue;
    
      Mval = (Nsec == -1) ? &catalog[0].average[i].M : &catalog[0].secfilt[i*Nsecfilt+Nsec].MpsfChp;
      *Mval = stats.mean;
      Mval = (Nsec == -1) ? &catalog[0].average[i].dM : &catalog[0].secfilt[i*Nsecfilt+Nsec].dMpsfChp;
      *Mval = stats.sigma;
      Mval = (Nsec == -1) ? &catalog[0].average[i].Mchisq : &catalog[0].secfilt[i*Nsecfilt+Nsec].Mchisq;
      *Mval = stats.chisq;
    }
  }
}

# include "mosastro.h"

int ClipOnFP (double Nsigma) {

  int i, j, Nscatter, Nmask, Nkeep, Nkpcp;
  double DL, DM, dL, dM;
  StarData *raw, *ref;

  Nmask = Nkeep = 0;

  // double sigma is returned;
  GetScatter (&Nscatter, &DL, &DM, FALSE);

  for (i = 0; i < Nchip; i++) {
    raw = chip[i].raw;
    ref = chip[i].ref;
    Nkpcp = 0;
    for (j = 0; j < chip[i].Nmatch; j++) {
      dL = raw[j].L - ref[j].L;
      dM = raw[j].M - ref[j].M;
      if ((fabs(dL) > Nsigma*DL) || (fabs(dM) > Nsigma*DM)) {
	raw[j].mask = TRUE;
	Nmask ++;
      } else {
	raw[j].mask = FALSE;	
	Nkeep ++;
	Nkpcp ++;
      }
    }
    fprintf (stderr, "Nchip: %d\n", Nkpcp);
  }

  fprintf (stderr, "Nmask: %d, Nkeep: %d\n", Nmask, Nkeep);
  return (TRUE);
}

/*
  sigma = sigma / (3600.0 * field.project.cdelt1);
*/

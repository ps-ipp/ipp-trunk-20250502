# include "mosastro.h"

/* measure scatter on the focal plane */
double GetScatter (int *Nscatter, double *DL, double *DM, int bright) {

  int i, j, Ntotal;
  double dL, dM, dL2, dM2, dl, dm, dR;
  StarData *raw, *ref;

  Ntotal = 0.0;
  dL = dL2 = dM = dM2 = 0;
  for (i = 0; i < Nchip; i++) {
    raw = chip[i].raw;
    ref = chip[i].ref;
    for (j = 0; j < chip[i].Nmatch; j++) {
      if (raw[j].mask) continue;
      if (bright && (raw[j].Mag > INST_BRIGHT)) continue;
      dl = raw[j].L - ref[j].L;
      dm = raw[j].M - ref[j].M;
      dL  += dl;
      dL2 += SQ(dl);
      dM  += dm;
      dM2 += SQ(dm);
      Ntotal ++;
    }
  }
  dL = sqrt (fabs(dL2 / Ntotal - (dL*dL) / (Ntotal*Ntotal)));
  dM = sqrt (fabs(dM2 / Ntotal - (dM*dM) / (Ntotal*Ntotal)));
  dR = hypot(dL, dM) * 3600.0 * field.project.cdelt1;
  *Nscatter = Ntotal;
  *DL = dL;
  *DM = dM;
  return (dR);
}

/* sigma is returned in arcsec, dL, dM are in pixels */
/* return dL and dM independently as well */

# include "astro.h"

int Npar;
int Nfpar;
opihi_flt *par;
opihi_flt *fpar;
opihi_flt *sky;

opihi_flt (*fitfunc)(opihi_flt, opihi_flt, opihi_flt *, int, opihi_flt *);
void (*imfit_cleanup)(void);

void fgauss_setup (char *name);
void pgauss_setup (char *name);
void sgauss_setup (char *name);
void qgauss_setup (char *name);
void rgauss_setup (char *name);
void qfgauss_setup (char *name);
void qrgauss_setup (char *name);
void pgauss_psf_setup (char *name);
void qgauss_psf_setup (char *name);
void sgauss_psf_setup (char *name);

void rgauss_pol_setup (char *name);
void fgauss_pol_setup (char *name);

void trail_setup (char *name);

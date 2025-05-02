# include "uniphot.h"

FWHMTable *load_fwhm_table (char *filename, int *nfwhm) {

  char name[1024], photcode[256];
  int Nfwhm, NFWHM;
  FWHMTable *fwhm;
  double fwhm_major, fwhm_minor, mjd;

  FILE *f;

  f = fopen (filename, "r");
  if (!f) {
    fprintf (stderr, "ERROR: cannot open fwhm table file %s\n", filename);
    exit (1);
  }

  Nfwhm = 0;
  NFWHM = 100;
  ALLOCATE (fwhm, FWHMTable, NFWHM);

  // format is fixed: (time in mjd) (fwhm_major) (fwhm_minor)
  int status;
  while ((status = fscanf (f, "%s %lf %s %lf %lf", name, &mjd, photcode, &fwhm_major, &fwhm_minor)) == 5) {
    fwhm[Nfwhm].fwhm_major = fwhm_major;
    fwhm[Nfwhm].fwhm_minor = fwhm_minor;
    fwhm[Nfwhm].time       = ohana_mjd_to_sec (mjd);
    fwhm[Nfwhm].photcode   = GetPhotcodeCodebyName (photcode);
    fwhm[Nfwhm].found      = FALSE;

    if (fwhm[Nfwhm].photcode == 0) {
      fprintf (stderr, "error in photcode %s\n", photcode);
      abort();
    }

    Nfwhm ++;
    CHECK_REALLOCATE (fwhm, FWHMTable, NFWHM, Nfwhm, 100);
  }

  if (status != EOF) {
    fprintf (stderr, "unexpected formatting on line %d (status = %d)\n", Nfwhm, status);
    exit (2);
  }

  fprintf (stderr, "loaded %d fwhm values\n", Nfwhm);

  *nfwhm = Nfwhm;
  return fwhm;
}

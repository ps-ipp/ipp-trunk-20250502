# include "lightcurve.h"

void make_table (sources, Nsources, unique, images, Nimages)
Star   *sources;
int     Nsources;
Unique *unique;
Image  *images;
int     Nimages;
{

  double  N, M, dM;
  int     i, j, n, im;
  Star   *next_measurement;
  FILE   *f;
  double  dR, dD;

  if (!strcmp(OUTFILE, "-")) {
    fprintf (stderr, "using stdout\n");
    f = stdout;
  }
  else {
    fprintf (stderr, "using %s for output\n", OUTFILE);
    f = fopen (OUTFILE, "w");
    if (f == NULL) {
      fprintf (stderr, "could not open output file, using stdout\n");
      f = stdout;
    }
  }
  
  fprintf (f, "# This is a table of data from relphot.\n");
  fprintf (f, "# This table contains the following data:\n");
  if (PIXELS) 
    fprintf (f, "# star number, <X> dX  <Y> dY  <Mrel> dMrel  Nstars\n");
  else
    fprintf (f, "# star number, <RA> dRA  <Dec> dDec  <Mrel> dMrel  Nstars\n");

  for (j = 0; j < Nsources; j++) {
    fprintf (f, "%5d %10.6f %10.6f ", j, sources[j].RA, sources[j].Dec);
    if (sources[j].unique_number == EMPTY) {
      fprintf (f, "%5d\n", 0.0);
      continue;
    }
    N = unique[sources[j].unique_number].Nmeasurements;
    fprintf (f, "%5.0f\n", N);
    next_measurement = unique[sources[j].unique_number].first_this_unique;
    M = 100.0; dM = 0.0;
    for (n = 0; n < N; n++) {
      im = next_measurement[0].image_number;
      dR = (next_measurement[0].RA - images[im].RA_O);
      dD = (next_measurement[0].Dec - images[im].DEC_O);
      M = next_measurement[0].m - MCAL(images[im], dR, dD);
      dM = next_measurement[0].dm;
      dR = (next_measurement[0].RA - sources[j].RA);
      dD = (next_measurement[0].Dec - sources[j].Dec);
      fprintf (f, "%-40s  %15.6f  %7.3f %7.3f  %7.3f %7.3f\n", images[im].name, images[im].JD, dR, dD, M, dM);
      next_measurement = next_measurement[0].next_this_unique;
    }
  }
  fclose (f);
}

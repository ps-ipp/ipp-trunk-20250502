# include <ohana.h>

int main (int argc, char **argv) {

  int i, Npts;
  double mean, sigma, f;

  if (argc != 4) {
    fprintf (stderr, "USAGE: mkgauss (mean) (sigma) (npts)\n");
    exit (2);
  }

  mean = atof (argv[1]);
  sigma = atof (argv[2]);
  Npts = atoi (argv[3]);

  ohana_gaussdev_init ();

  for (i = 0; i < Npts; i++) {
    f = ohana_gaussdev_rnd (mean, sigma);
    fprintf (stdout, "%f\n", f);
  }

  exit (0);
}


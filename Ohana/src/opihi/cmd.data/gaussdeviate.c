# include "data.h"

int gaussdeviate (int argc, char **argv) {
  
  int i, Npts;
  double mean, sigma;
  Vector *vec;

  if (argc != 5) goto usage;

  if ((vec = SelectVector (argv[1], ANYVECTOR, TRUE)) == NULL) return (FALSE);    

  Npts = atoi (argv[2]);
  mean = atof (argv[3]);
  sigma = atof (argv[4]);

  ResetVector (vec, OPIHI_FLT, Npts);

  ohana_gaussdev_init ();
  for (i = 0; i < Npts; i++) {
    vec[0].elements.Flt[i] = ohana_gaussdev_rnd (mean, sigma);
  }
  return (TRUE);

 usage:
  gprint (GP_ERR, "USAGE: gaussdeviate (vector) Npts mean sigma\n");
  return (FALSE);
    
}

int gaussintegral (int argc, char **argv) {
  OHANA_UNUSED_PARAM(argc);
  OHANA_UNUSED_PARAM(argv);
  
  gprint (GP_ERR, "fix gaussintegral\n");
  return (FALSE);

/*
  int i, Npts;
  Vector *vec;

  if (argc != 5) goto usage;

  if ((vec = SelectVector (argv[1], ANYVECTOR, TRUE)) == NULL) return (FALSE);    

  Npts = atoi (argv[2]);
  // mean = atof (argv[3]);
  // sigma = atof (argv[4]);

  ResetVector (vec, OPIHI_FLT, Npts);

  ohana_gaussdev_init ();
  for (i = 0; i < Npts; i++) {
    vec[0].elements.Flt[i] = gaussian_int (i);
  }
  return (TRUE);

 usage:
  gprint (GP_ERR, "USAGE: gaussintegral Npts mean sigma\n");
  return (FALSE);
*/
    
}

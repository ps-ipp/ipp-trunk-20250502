# include "mosastro.h"

void print_help () {
  fprintf (stderr, "USAGE: mkobs (RA) (DEC) (output) [-p param value]\n");
  exit (1);
}

void args (int *argc, char **argv) {
  
  int N, No, Np, Nx, Ny;
  double theta;
  char line[500];

  if (get_argument (*argc, argv, "--help")) print_help ();
  if (get_argument (*argc, argv, "-h")) print_help ();

  FOCAL_PLANE = NULL;
  if ((N = get_argument (*argc, argv, "-fp"))) {
    remove_argument (N, argc, argv);
    FOCAL_PLANE = strcreate (argv[N]);
    remove_argument (N, argc, argv);
  }

  NO_CHIPS = FALSE;
  if ((N = get_argument (*argc, argv, "-nochips"))) {
    remove_argument (N, argc, argv);
    NO_CHIPS = TRUE;
  }

  SIGMA = 0;
  if ((N = get_argument (*argc, argv, "-sigma"))) {
    remove_argument (N, argc, argv);
    SIGMA = atof (argv[N]);
    remove_argument (N, argc, argv);
  }

  fake_field_defaults ();

  while ((N = get_argument (*argc, argv, "-p"))) {
    remove_argument (N, argc, argv);
    
    if (!strcmp (argv[N], "help")) {
      fprintf (stderr, "valid parameters:\n");
      fprintf (stderr, "cdelt (arcsec/pix)\n");
      fprintf (stderr, "crpix (Xo) (Yo)\n");
      fprintf (stderr, "theta (angle)\n");
      exit (2);
    }

    if (!strcmp (argv[N], "cdelt")) {
      remove_argument (N, argc, argv);
      field.project.cdelt2 = field.project.cdelt1 = atof(argv[N])/3600.0;
      remove_argument (N, argc, argv);
      continue;
    }
    if (!strcmp (argv[N], "crpix")) {
      remove_argument (N, argc, argv);
      field.project.crpix1 = atof(argv[N]);
      remove_argument (N, argc, argv);
      field.project.crpix2 = atof(argv[N]);
      remove_argument (N, argc, argv);
      continue;
    }
    if (!strcmp (argv[N], "theta")) {
      remove_argument (N, argc, argv);
      theta = atof(argv[N]);
      remove_argument (N, argc, argv);
      field.project.pc1_1 = +cos (RAD_DEG*theta);
      field.project.pc1_2 = -sin (RAD_DEG*theta);
      field.project.pc2_1 = +sin (RAD_DEG*theta);
      field.project.pc2_2 = +cos (RAD_DEG*theta);
      continue;
    }
    if (!strncmp (argv[N], "pca", 3)) {
      No = argv[N][3] - '0';
      Nx = argv[N][5] - '0';
      Ny = argv[N][7] - '0';
      if ((Nx + Ny > 3) || (No > 1)) {
	fprintf (stderr, "PCA out of range\n");
	exit (1);
      }
      Np = mkpolyterm (Nx, Ny);
      remove_argument (N, argc, argv);
      field.distort.polyterms[Np][No] = atof(argv[N]);
      remove_argument (N, argc, argv);
      continue;
    }
  }

  if (*argc != 4) print_help ();
}

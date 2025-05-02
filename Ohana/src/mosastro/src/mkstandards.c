# include "mosastro.h"
# define DX 30.0
# define DY 30.0

extern double drand48();
void init_random ();

/* build a grid of reference stars */
int main (int argc, char **argv) {

  int i, N, Nstars, NSTARS, Random;
  double x, y, m, dX, dY;
  Header *header;
  Coords coords;
  SMPData *stars;

  Random = FALSE;
  if ((N = get_argument (argc, argv, "-random"))) {
    Random = TRUE;
    remove_argument (N, &argc, argv);
    Nstars = atoi (argv[N]);
    remove_argument (N, &argc, argv);
  }

  if (argc != 5) {
    fprintf (stderr, "USAGE: mkstandards (RA) (DEC) (half-width) (output) [-random Nstars]\n");
    exit (2);
  }

  /* generate grid of stars */
  dX = dY = 3600*atof (argv[3]);

  /* bore site center guess */
  InitCoords (&coords, "DEC--TAN");
  coords.crval1 = atof(argv[1]);
  coords.crval2 = atof(argv[2]);
  coords.crpix1 = dX;
  coords.crpix2 = dX;
  coords.cdelt1 = 1/3600.0;
  coords.cdelt2 = 1/3600.0;

  if (Random) {
    
    init_random ();

    ALLOCATE (stars, SMPData, Nstars);
    for (i = 0; i < Nstars; i++) {
      x = 2*dX*drand48();
      y = 2*dY*drand48();
      m = 14.0 + 4.0*drand48();
      stars[i].X = x;
      stars[i].Y = y;
      stars[i].M = m;
      stars[i].dM = 0.02;
    }
  } else {

    Nstars = 0;
    NSTARS = 1000;
    ALLOCATE (stars, SMPData, NSTARS);

    for (x = 0; x < 2*dX; x += DX) {
      for (y = 0; y < 2*dY; y += DY) {
	stars[Nstars].X = x;
	stars[Nstars].Y = y;
	stars[Nstars].M = 16.0;

	Nstars ++;
	if (Nstars >= NSTARS) {
	  NSTARS += 1000;
	  REALLOCATE (stars, SMPData, NSTARS);
	}
      }
    }
  }

  for (i = 0; i < Nstars; i++) {
    stars[i].dophot = 1;
    stars[i].sky = 1.0;
    stars[i].Mgal = 16.0;
    stars[i].Map = 16.0;
    stars[i].fx = 1.0;
    stars[i].fy = 1.0;
    stars[i].df = 0.0;
  }

  header = mkheader (2*dX, 2*dY, Nstars, &coords);
  wstars (argv[4], stars, Nstars, header);
  gfits_free_header (header);

  exit (0);
}

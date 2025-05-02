# include "mosastro.h"

int main (int argc, char **argv) {

  int i, j, N, Nrefcat, Nchip, Nstars, NSTARS;
  double Ro, Do, Po, Qo, Lo, Mo;
  double dX, dY, Dist;
  double RA, DEC;
  char filename[64];
  Coords map, coords;
  StarData *refcat;
  SMPData *stars;
  Header *header;

  ConfigInit (&argc, argv);
  args (&argc, argv);
  init_random ();

  RA  = atof(argv[1]);
  DEC = atof(argv[2]);
  OUTPUT = argv[3];
  fake_field_center (RA, DEC);

  /* chip size in pixels (output is 5x5 grid of chips */
  dX = 0.1 / field.project.cdelt1;
  dY = 0.1 / field.project.cdelt2;

  refcat = greference (&Nrefcat);
  project_refcat (refcat, Nrefcat);
  if (FOCAL_PLANE != NULL) {
    FILE *f;
    f = fopen (FOCAL_PLANE, "w");
    dump_stars (f, refcat, Nrefcat);
    fclose (f);
  }
  if (NO_CHIPS) exit (0);

  Nchip = 0;
  for (i = -2; i < 3; i++) {
    for (j = -2; j < 3; j++) {

      /* find chip center in TP, FP and Sky */
      Po = 2*dX*i;
      Qo = 2*dY*j;
      RD_to_XY (&Lo, &Mo, Po, Qo, &field.distort);
      XY_to_RD (&Ro, &Do, Po, Qo, &field.project);

      /* FP-Chip terms */
      strcpy (map.ctype, "DEC--WRP");
      map.crval1 = Lo;
      map.crval2 = Mo;
      map.crpix1 = dX;
      map.crpix2 = dX;
  
      map.cdelt1 = 1.0;
      map.cdelt2 = 1.0;

      map.pc1_1  = 1;
      map.pc2_2  = 1;
      map.pc1_2  = 0;
      map.pc2_1  = 0;

      map.Npolyterms = 2;
      for (N = 0; N < 7; N++) {
	map.polyterms[N][0] = 0;
	map.polyterms[N][1] = 0;
      }
      map.polyterms[2][1] = 1e-5;

      /* project catalog stars to chip */
      FPtoChip (refcat, Nrefcat, &map);

      Nstars = 0;
      NSTARS = 1000;
      ALLOCATE (stars, SMPData, NSTARS);

      /* find catalog stars on chip */
      for (N = 0; N < Nrefcat; N++) {
	if (refcat[N].X < 0) continue;
	if (refcat[N].Y < 0) continue;
	if (refcat[N].X >= 2*dX) continue;
	if (refcat[N].Y >= 2*dY) continue;

	/* add random noise - not gaussian noise */
	stars[Nstars].X = refcat[N].X + SIGMA*(drand48() - 0.5);
	stars[Nstars].Y = refcat[N].Y + SIGMA*(drand48() - 0.5);
	stars[Nstars].M = 16.0;
	stars[Nstars].dM = 0.02;
	stars[Nstars].dophot = 1;
	stars[Nstars].sky = 1.0;
	stars[Nstars].Mgal = 16.0;
	stars[Nstars].Map = 16.0;
	stars[Nstars].fx = 1.0;
	stars[Nstars].fy = 1.0;
	stars[Nstars].df = 0.0;

	Nstars ++;
	if (Nstars >= NSTARS) {
	  NSTARS += 1000;
	  REALLOCATE (stars, SMPData, NSTARS);
	}
      }


      /* Chip-Sky terms */
      InitCoords (&coords, "DEC--WRP");
      coords.crval1 = Ro;
      coords.crval2 = Do;
      coords.crpix1 = dX;
      coords.crpix2 = dY;
  
      { 
	double dP, dQ, dL, dM;
	double Mx, Lx, scale;

	dP = 10;
	dQ = 10;
	RD_to_XY (&Lx, &Mx, Po+dP, Qo+dQ, &field.distort);
	dL = Lx - Lo;
	dM = Mx - Mo;
	scale = hypot (10.0, 10.0) / hypot (dL, dM);
	coords.cdelt1 = scale/3600.0;
	coords.cdelt2 = scale/3600.0;
      }

      header = mkheader (2*dX, 2*dY, Nstars, &coords);

      sprintf (filename, "%s.%02d.fits", OUTPUT, Nchip);
      wstars (filename, stars, Nstars, header);
      gfits_free_header (header);
      free (stars);
      Nchip ++;
    }
  }
  exit (0);
}

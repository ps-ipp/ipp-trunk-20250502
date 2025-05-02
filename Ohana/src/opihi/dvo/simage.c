# include "dvoshell.h"
# define D_NSTARS 1000
# define BYTES_STAR 31
# define BLOCK 1000

int simage (int argc, char **argv) {

  char *buffer;
  Vector Xvec, Yvec, Zvec;
  double R, D, X, Y, M, zero, range;
  FILE *f;
  Header header;
  Coords coords;
  int i, j, kapa, Nstars, nstars, Nbytes, nbytes, N;
  Graphdata graphmode;

  if (!GetGraph (&graphmode, &kapa, NULL)) return (FALSE);

  zero = 17.0;
  range = -5.0;
  if ((N = get_argument (argc, argv, "-m"))) {
    remove_argument (N, &argc, argv);
    range = atof(argv[N]);
    remove_argument (N, &argc, argv);
    zero  = atof(argv[N]);
    range = range - zero;
    remove_argument (N, &argc, argv);
  }

  if (argc != 2) {
    gprint (GP_ERR, "USAGE: image (filename)\n");
    return (FALSE);
  }

  gprint (GP_ERR, "not working at the moment (cmp format)\n");
  return (FALSE);
  
  /* read header */
  if (!gfits_read_header (argv[1], &header)) {
    gprint (GP_ERR, "ERROR: can't find image file %s\n", argv[1]);
    return (FALSE);
  }
  /* get astrometry information */
  if (!GetCoords (&coords, &header)) {
    gprint (GP_ERR, "ERROR: can't get coord info from header\n");
    return (FALSE);
  }

  /* find number of stars */
  gfits_scan (&header, "NSTARS", "%d", 1, &Nstars);
  if (Nstars == 0) {
    gprint (GP_ERR, "no stars in file\n");
    return (FALSE);
  }

  /* open file data */
  f = fopen (argv[1], "r");
  if (f == NULL) {
    gprint (GP_ERR, "can't find data in file %s\n", argv[1]);
    return (FALSE);
  }
  fseeko (f, header.datasize, SEEK_SET); 

  /* set up storage buffers */
  SetVector (&Xvec, OPIHI_FLT, Nstars);
  SetVector (&Yvec, OPIHI_FLT, Nstars);
  SetVector (&Zvec, OPIHI_FLT, Nstars);
  ALLOCATE (buffer, char, (BLOCK*BYTES_STAR));

  /* load in stars by blocks of 1000 */
  nstars = 0;
  Nbytes = Nstars*BYTES_STAR;
  for (i = 0; i < (int)(Nbytes / (BLOCK*BYTES_STAR)); i++) {
    nbytes = fread (buffer, 1, (BLOCK*BYTES_STAR), f);
    if (nbytes != BLOCK*BYTES_STAR) {
      gprint (GP_ERR, "failed to read in stars (1)\n");
      free (Xvec.elements.Flt);
      free (Yvec.elements.Flt);
      free (Zvec.elements.Flt);
      free (buffer);
      return (FALSE);
    }
    for (j = 0; j < BLOCK; j++, nstars++) {
      dparse (&X,  1, &buffer[j*BYTES_STAR]);
      dparse (&Y,  2, &buffer[j*BYTES_STAR]);
      dparse (&M,  3, &buffer[j*BYTES_STAR]);
      XY_to_RD (&R, &D, X, Y, &coords);
      RD_to_XY (&Xvec.elements.Flt[nstars], &Yvec.elements.Flt[nstars], R, D, &graphmode.coords);
      Zvec.elements.Flt[nstars] = MIN (1.0, MAX (0.01, (M - zero) / range));
    }
  }
  /* left over fraction of a block */
  nbytes = fread (buffer, 1, (Nbytes % (BLOCK*BYTES_STAR)), f);
  if (nbytes != (Nbytes % (BLOCK*BYTES_STAR))) {
    gprint (GP_ERR, "ERROR: failed to read in stars (2)\n");
    free (Xvec.elements.Flt);
    free (Yvec.elements.Flt);
    free (Zvec.elements.Flt);
    free (buffer);
    return (FALSE);
  }
  for (j = 0; j < nbytes / BYTES_STAR; j++, nstars++) {
    dparse (&X,  1, &buffer[j*BYTES_STAR]);
    dparse (&Y,  2, &buffer[j*BYTES_STAR]);
    dparse (&M,  3, &buffer[j*BYTES_STAR]);
    XY_to_RD (&R, &D, X, Y, &coords);
    RD_to_XY (&Xvec.elements.Flt[nstars], &Yvec.elements.Flt[nstars], R, D, &graphmode.coords);
    Zvec.elements.Flt[nstars] = MIN (1.0, MAX (0.01, (M - zero) / range));
  }
  
  if (nstars != Nstars) {
    gprint (GP_ERR, "ERROR: failed to read in all stars (%d of %d)\n", nstars, Nstars);
    free (Xvec.elements.Flt);
    free (Yvec.elements.Flt);
    free (Zvec.elements.Flt);
    free (buffer);
    return (FALSE);
  }

  graphmode.style = KAPA_PLOT_POINTS;
  graphmode.size = -1;
  graphmode.etype = 0;

  PlotVectorTriplet (kapa, &Xvec, &Yvec, &Zvec, NULL, &graphmode);

  free (Xvec.elements.Flt);
  free (Yvec.elements.Flt);
  free (Zvec.elements.Flt);
  free (buffer);

  return (TRUE);

}



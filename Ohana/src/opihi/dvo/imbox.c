# include "dvoshell.h"

// LARGEFILES: this function is currently limited to images with Nx,Ny each < 2^31
int imbox (int argc, char **argv) {
  
  off_t Nskip;
  int j, kapa, status, InPic, flipped, N, haveNx, haveNy, Nx, Ny, SOLO_PHU, Npts, NPTS;
  Vector Xvec, Yvec;
  double r, d, x[4], y[4], Rmin, Rmax, Rmid;
  Header header;
  Coords coords;
  Coords mosaic;
  Graphdata graphmode;
  FILE *f;

  if (!style_args (&graphmode, &argc, argv, &kapa)) return FALSE;

  char *xaxis = NULL;
  if ((N = get_argument (argc, argv, "-xaxis"))) {
    remove_argument (N, &argc, argv);
    xaxis = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }
  char *yaxis = NULL;
  if ((N = get_argument (argc, argv, "-yaxis"))) {
    remove_argument (N, &argc, argv);
    yaxis = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }

  SOLO_PHU = FALSE;
  if ((N = get_argument (argc, argv, "-phu"))) {
    SOLO_PHU = TRUE;
    remove_argument (N, &argc, argv);
  }

  if (argc != 2) {
    gprint (GP_ERR, "USAGE: imbox (filename)\n");
    return (FALSE);
  }

  f = fopen (argv[1], "r");
  if (f == NULL) {
    gprint (GP_ERR, "file not found\n");
    return (FALSE);
  }
  
  Rmin = graphmode.coords.crval1 - 182.0;
  Rmax = graphmode.coords.crval1 + 182.0;
  Rmid = 0.5*(Rmin + Rmax);
  
  /* project this image to screen display coords */
  Npts = 0;
  NPTS = 200; 
  SetVector (&Xvec, OPIHI_FLT, NPTS);
  SetVector (&Yvec, OPIHI_FLT, NPTS);

  mosaic.ctype[0] = 0;  

  while (gfits_fread_header (f, &header)) {
    if (!GetCoords (&coords, &header)) goto skip;
    if (!SOLO_PHU && !strcmp (&coords.ctype[4], "-DIS")) {
      mosaic = coords;
      goto skip;
    }
    if (!strcmp (&coords.ctype[4], "-WRP")) {
      if (!mosaic.ctype[0]) {
	fprintf (stderr, "PHU mosaic not found\n");
	return FALSE;
      }
      coords.mosaic = &mosaic;
    }

    // XXX currently, image uses an unsigned short for NX,XY. this is rather restrictive
    // and needs to be at least checked.
    haveNx = FALSE;
    if (xaxis) {
      haveNx = gfits_scan (&header, xaxis, "%d", 1, &Nx);
    }
    if (!haveNx) {
      haveNx = gfits_scan (&header, "IMNAXIS1",   "%d", 1, &Nx);
    } 
    if (!haveNx) {
	haveNx = gfits_scan (&header, "ZNAXIS1",   "%d", 1, &Nx);
    }
    if (!haveNx) {
	haveNx = gfits_scan (&header, "NAXIS1",   "%d", 1, &Nx);
    }

    haveNy = FALSE;
    if (yaxis) {
      haveNy = gfits_scan (&header, yaxis, "%d", 1, &Ny);
    }
    if (!haveNy) {
      haveNy = gfits_scan (&header, "IMNAXIS2",   "%d", 1, &Ny);
    }
    if (!haveNy) {
	haveNy = gfits_scan (&header, "ZNAXIS2",   "%d", 1, &Ny);
    }
    if (!haveNy) {
	haveNy = gfits_scan (&header, "NAXIS2",   "%d", 1, &Ny);
    }

    if (!haveNx || !haveNy) {
	fprintf (stderr, "missing image dimensions in header\n");
	goto skip;
    }

    x[0] = 0;  y[0] = 0;
    x[1] = Nx; y[1] = 0;
    x[2] = Nx; y[2] = Ny;
    x[3] = 0;  y[3] = Ny;
    status = FALSE;
    flipped = FALSE;
    for (j = 0; j < 4; j++) {
      XY_to_RD (&r, &d, x[j], y[j], &coords);
      r = ohana_normalize_angle (r);
      while ((j == 0) && (r < Rmin)) { flipped = TRUE; r += 360.0; }
      while ((j == 0) && (r > Rmax)) { flipped = TRUE; r -= 360.0; }
      if ((j > 0) && flipped) {
	while (r < Rmid) r+= 360.0;
	while (r > Rmid) r-= 360.0;
      }
      status |= RD_to_XY (&Xvec.elements.Flt[Npts + 2*j], &Yvec.elements.Flt[Npts + 2*j], r, d, &graphmode.coords);
      if (j > 0) {
	Xvec.elements.Flt[Npts + 2*j - 1] = Xvec.elements.Flt[Npts + 2*j];
	Yvec.elements.Flt[Npts + 2*j - 1] = Yvec.elements.Flt[Npts + 2*j];
      }
    }
    Xvec.elements.Flt[Npts + 7] = Xvec.elements.Flt[Npts + 0];
    Yvec.elements.Flt[Npts + 7] = Yvec.elements.Flt[Npts + 0];

    InPic = FALSE;
    for (j = 0; j < 8; j+=2) {
      if ((Xvec.elements.Flt[Npts + j] >= graphmode.xmin) && 
    	  (Xvec.elements.Flt[Npts + j] <= graphmode.xmax) && 
    	  (Yvec.elements.Flt[Npts + j] >= graphmode.ymin) && 
    	  (Yvec.elements.Flt[Npts + j] <= graphmode.ymax))
    	InPic = TRUE;
    }
    if (!InPic) continue;

    Npts += 8;
    if (Npts + 8 >= NPTS) {  /* need to leave room for 4 point image */
      NPTS += 200;
      REALLOCATE (Xvec.elements.Flt, opihi_flt, NPTS);
      REALLOCATE (Yvec.elements.Flt, opihi_flt, NPTS);
    }

  skip:
    Nskip = gfits_data_size (&header);
    fseeko (f, Nskip, SEEK_CUR); 
    gfits_free_header (&header);
  }

  Xvec.Nelements = Yvec.Nelements = Npts;
  if (Npts > 0) {
    graphmode.style = KAPA_PLOT_POINTS; /* points */
    graphmode.ptype = KAPA_POINT_PAIR_CONNECT; /* connect pairs of points */
    graphmode.etype = 0;
    PlotVectorPair (kapa, &Xvec, &Yvec, NULL, &graphmode);
  }

  fclose (f);
  free (Xvec.elements.Flt);
  free (Yvec.elements.Flt);
  if (xaxis) free (xaxis);
  if (yaxis) free (yaxis);
  return (TRUE);

}



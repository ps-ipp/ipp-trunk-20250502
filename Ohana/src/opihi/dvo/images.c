# include "dvoshell.h"
# define BETA 0.41421

int wordhash (char *word) {
  int value;

  value = *(int *)word;
  return value;
}

int images (int argc, char **argv) {

  off_t i, Nimage, Nmosaic;
  int j, status, InPic, leftside, TimeSelect, ByName;
  // int *plist, n, npts; -- what is/was plist?
  int WITH_MOSAIC, SOLO_MOSAIC;
  time_t tzero, tend;
  int n, N, NPTS, Npts, kapa, *foundMosaic;
  Vector Xvec, Yvec;
  double r[8], d[8], x[8], y[8], Rmin, Rmax, Rmid, trange, Radius;
  Image *image;
  Graphdata graphmode;
  char name[256];
  int typehash;
  PhotCode *photcode;
  int Nsec, photcodeEquiv;

  if (!InitPhotcodes ()) return (FALSE);
  if (!style_args (&graphmode, &argc, argv, &kapa)) goto usage;

  WITH_MOSAIC = FALSE;
  if ((N = get_argument (argc, argv, "+mosaic"))) {
    remove_argument (N, &argc, argv);
    WITH_MOSAIC = TRUE;
  }

  SOLO_MOSAIC = FALSE;
  foundMosaic = NULL;
  if ((N = get_argument (argc, argv, "-mosaic"))) {
    remove_argument (N, &argc, argv);
    SOLO_MOSAIC = TRUE;
    WITH_MOSAIC = TRUE;
  }

  // int HIDDEN = FALSE;
  // if ((N = get_argument (argc, argv, "-hidden"))) {
  //   remove_argument (N, &argc, argv);
  //   HIDDEN = TRUE;
  // }

  photcode = NULL;
  photcodeEquiv = FALSE;
  if ((N = get_argument (argc, argv, "-p"))) {
    remove_argument (N, &argc, argv);
    photcode = GetPhotcodebyName (argv[N]);
    if (photcode == NULL) {
      gprint (GP_ERR, "photcode %s not found\n", argv[N]);
      return FALSE;
    }
    remove_argument (N, &argc, argv);
    Nsec = GetPhotcodeNsec (photcode->code);
    if (Nsec != -1) photcodeEquiv = TRUE;
  }
  if ((N = get_argument (argc, argv, "-photcode"))) {
    remove_argument (N, &argc, argv);
    photcode = GetPhotcodebyName (argv[N]);
    if (photcode == NULL) {
      gprint (GP_ERR, "photcode %s not found\n", argv[N]);
      return FALSE;
    }
    remove_argument (N, &argc, argv);
    Nsec = GetPhotcodeNsec (photcode->code);
    if (Nsec != -1) photcodeEquiv = TRUE;
  }

  ByName = FALSE;
  if ((N = get_argument (argc, argv, "-name"))) {
    remove_argument (N, &argc, argv);
    strcpy (name, argv[N]);
    remove_argument (N, &argc, argv);
    ByName = TRUE;
  }

  Radius = 45;
  if ((N = get_argument (argc, argv, "-radius"))) {
    remove_argument (N, &argc, argv);
    Radius = atof (argv[N]);
    remove_argument (N, &argc, argv);
  }

  TimeSelect = FALSE;
  if ((N = get_argument (argc, argv, "-time"))) {
    remove_argument (N, &argc, argv);
    if (!ohana_str_to_time (argv[N], &tzero)) {
      gprint (GP_ERR, "syntax error\n");
      goto usage;
    }
    remove_argument (N, &argc, argv);
    if (!ohana_str_to_dtime (argv[N], &trange)) { 
      gprint (GP_ERR, "syntax error\n");
      goto usage;
    }
    remove_argument (N, &argc, argv);
    if (trange < 0) {
      trange = fabs (trange);
      tzero -= trange;
    }
    TimeSelect = TRUE;
  }
  if ((N = get_argument (argc, argv, "-trange"))) {
    remove_argument (N, &argc, argv);
    if (!ohana_str_to_time (argv[N], &tzero)) { 
      gprint (GP_ERR, "syntax error\n");
      goto usage;
    }
    remove_argument (N, &argc, argv);
    if (!ohana_str_to_time (argv[N], &tend)) { 
      gprint (GP_ERR, "syntax error\n");
      goto usage;
    }
    remove_argument (N, &argc, argv);
    trange = tend - tzero;
    if (trange < 0) {
      trange = fabs (trange);
      tzero -= trange;
    }
    TimeSelect = TRUE;
  }
 
  if (argc != 1) goto usage;
  
  /* it is not an error for the database not to have any images */
  if ((image = LoadImagesDVO (&Nimage)) == NULL) return (TRUE);
  // BuildChipMatch (image, Nimage);

  if (SOLO_MOSAIC && photcode) {
    ALLOCATE(foundMosaic, int, Nimage);
    memset(foundMosaic, 0, Nimage*sizeof(int));
  }

  Rmin = graphmode.coords.crval1 - 180.0;
  Rmax = graphmode.coords.crval1 + 180.0;
  Rmid = 0.5*(Rmin + Rmax);
  
  int DistortImage = wordhash ("-DIS");
  int ChipImage    = wordhash ("-WRP");
  int TriangleUp   = wordhash ("TRP-");
  int TriangleDn   = wordhash ("TRM-");
  int TrianglePts  = wordhash ("TRI-");

  // npts = 200
  NPTS = 200;
  SetVector (&Xvec, OPIHI_FLT, NPTS);
  SetVector (&Yvec, OPIHI_FLT, NPTS);

  Image *mosaic = NULL;

  /******** stage 1 : find the images to plot *********/

  // ALLOCATE (plist, int, NPTS);
  n = 0; // tracks number of images
  N = 0; // tracks number of vertices
  for (i = 0; i < Nimage; i++) {
    if (ByName && strncmp (image[i].name, name, strlen(name))) continue;
    if (TimeSelect && ((image[i].tzero < tzero) || (image[i].tzero+image[i].trate*image[i].NY > tzero + trange))) continue;

    mosaic = image[i].parent;
    Nmosaic = (SOLO_MOSAIC && mosaic) ? mosaic - image : -1;

    if (photcode) {
      if ( photcodeEquiv && (photcode[0].code != GetPhotcodeEquivCodebyCode(image[i].photcode))) continue;
      if (!photcodeEquiv && (photcode[0].code != image[i].photcode)) continue;
    }

    Npts = 4;
    status = TRUE;
    leftside = FALSE;

    typehash = wordhash (&image[i].coords.ctype[4]);

    if (photcode && SOLO_MOSAIC && (Nmosaic >= 0)) {
      // mosaic (DIS) images are not currently given a photcode : plot these via the WRP entries
      /* DIS images represent a field, not a chip */
      if (typehash != ChipImage) continue;
      if (foundMosaic[Nmosaic]) continue;
      x[0] = -0.5*mosaic->NX; y[0] = -0.5*mosaic->NY;
      x[1] = +0.5*mosaic->NX; y[1] = -0.5*mosaic->NY;
      x[2] = +0.5*mosaic->NX; y[2] = +0.5*mosaic->NY;
      x[3] = -0.5*mosaic->NX; y[3] = +0.5*mosaic->NY;
      for (j = 0; j < Npts; j++) {
	status = XY_to_RD (&r[j], &d[j], x[j], y[j], &mosaic->coords);
	if (!status) break;
	r[j] = ohana_normalize_angle (r[j]);
	while (r[j] < Rmin) { r[j] += 360.0; }
	while (r[j] > Rmax) { r[j] -= 360.0; }
	if (j == 0) {
	  leftside = (r[j] < Rmid);
	} 
	if (j > 0) { 
	  if ( leftside && (r[j] > Rmid + 90)) { r[j] -= 360.0; }
	  if (!leftside && (r[j] < Rmid - 90)) { r[j] += 360.0; }
	}
      }
      foundMosaic[Nmosaic] = TRUE;
      goto plot_points;
    }

    /* DIS images represent a field, not a chip */
    if ((typehash == DistortImage) && !WITH_MOSAIC) continue; // do not plot the mosaic images
    if ((typehash != DistortImage) &&  SOLO_MOSAIC) continue; // plot only the mosaic images

    if (typehash == DistortImage) {
      x[0] = -0.5*image[i].NX; y[0] = -0.5*image[i].NY;
      x[1] = +0.5*image[i].NX; y[1] = -0.5*image[i].NY;
      x[2] = +0.5*image[i].NX; y[2] = +0.5*image[i].NY;
      x[3] = -0.5*image[i].NX; y[3] = +0.5*image[i].NY;
      goto got_type;
    }

    typehash = wordhash (image[i].coords.ctype);
    if (typehash == TriangleUp) {
      Npts = 3;
      x[0] =                0; y[0] = +0.5*image[i].NY;
      x[1] = +0.5*image[i].NX; y[1] = -0.5*image[i].NY;
      x[2] = -0.5*image[i].NX; y[2] = -0.5*image[i].NY;
      goto got_type;
    }
    if (typehash == TriangleDn) {
      Npts = 3;
      x[0] =                0; y[0] = -0.5*image[i].NY;
      x[1] = +0.5*image[i].NX; y[1] = +0.5*image[i].NY;
      x[2] = -0.5*image[i].NX; y[2] = +0.5*image[i].NY;
      goto got_type;
    }
    // For 'TrianglePts' (TRI-), we are absurdly overloaded values otherwise not used
    if (typehash == TrianglePts) {
      Npts = 3;
      x[0] = image[0].dXpixSys;      y[0] = image[0].dYpixSys;
      x[1] = image[0].dMagSys;       y[1] = image[0].nFitAstrom;
      x[2] = image[0].photom_map_id; y[2] = image[0].astrom_map_id;
      goto got_type;
    }

    // default layout
    {
      x[0] = 0;           y[0] = 0;
      x[1] = image[i].NX; y[1] = 0;
      x[2] = image[i].NX; y[2] = image[i].NY;
      x[3] = 0;           y[3] = image[i].NY;
    }

  got_type:

    /* project this image to screen display coords */
    // check for boundary overlap?
    for (j = 0; j < Npts; j++) {
      status = XY_to_RD (&r[j], &d[j], x[j], y[j], &image[i].coords);
      if (!status) break;
      r[j] = ohana_normalize_angle (r[j]);
      while (r[j] < Rmin) { r[j] += 360.0; }
      while (r[j] > Rmax) { r[j] -= 360.0; }
      if (j == 0) {
	leftside = (r[j] < Rmid);
      } 
      if (j > 0) { 
	if ( leftside && (r[j] > Rmid + 90)) { r[j] -= 360.0; }
	if (!leftside && (r[j] < Rmid - 90)) { r[j] += 360.0; }
      }
    }

    // extremely large-scale images with certain projections will have odd boundaries.
    // eg, the ASCA images are essentially circles of radius ~60 degrees.  plot these as 
    // octagons with some dummy size.
    if (!status) {
      int jp, xo, yo;
      double rc, dc;
      xo = 0.5*image[i].NX;
      yo = 0.5*image[i].NY;
      // is the image center on the screen?
      status = XY_to_RD (&rc, &dc, 0.5*x[2], 0.5*y[2], &image[i].coords);
      if (status && (image[i].NX * image[i].coords.cdelt2 > 90)) {
	// draw an octagon with radius 45 degrees
	double dX = Radius / image[i].coords.cdelt2;
	x[0] = xo+dX;      y[0] = yo+BETA*dX;
	x[1] = xo+BETA*dX; y[1] = yo+dX;
	x[2] = xo-BETA*dX; y[2] = yo+dX;
	x[3] = xo-dX;      y[3] = yo+BETA*dX;
	x[4] = xo-dX;      y[4] = yo-BETA*dX;
	x[5] = xo-BETA*dX; y[5] = yo-dX;
	x[6] = xo+BETA*dX; y[6] = yo-dX;
	x[7] = xo+dX;      y[7] = yo-BETA*dX;
	Npts = 8;
	j = 0;
	for (jp = 0; jp < 8; jp++) {
	  status = XY_to_RD (&r[j], &d[j], x[jp], y[jp], &image[i].coords);
	  if (!status) continue;
	  r[j] = ohana_normalize_angle (r[j]);
	  while (r[j] < Rmin) { r[j] += 360.0; }
	  while (r[j] > Rmax) { r[j] -= 360.0; }
	  if (j == 0) {
	    leftside = (r[j] < Rmid);
	  } 
	  if (j > 0) { 
	    if ( leftside && (r[j] > Rmid + 90)) { r[j] -= 360.0; }
	    if (!leftside && (r[j] < Rmid - 90)) { r[j] += 360.0; }
	  }
	  j++;
	}
	Npts = j;
      } else {
	continue;
      }
    }
    if (Npts == 0) continue;

  plot_points:

    status = FALSE;
    for (j = 0; j < Npts; j++) {
      status |= RD_to_XY (&Xvec.elements.Flt[N+2*j], &Yvec.elements.Flt[N+2*j], r[j], d[j], &graphmode.coords);
      if (j > 0) {
	Xvec.elements.Flt[N+2*j - 1] = Xvec.elements.Flt[N+2*j];
	Yvec.elements.Flt[N+2*j - 1] = Yvec.elements.Flt[N+2*j];
      }
    }
    Xvec.elements.Flt[N+2*Npts-1] = Xvec.elements.Flt[N];
    Yvec.elements.Flt[N+2*Npts-1] = Yvec.elements.Flt[N];
    if (!status) continue;
    // if none of the points are on the visible side of the projection, do not plot the image

    InPic = FALSE;
    for (j = 0; j < 2*Npts; j+=2) {
      if ((Xvec.elements.Flt[N+j] >= graphmode.xmin) && 
	  (Xvec.elements.Flt[N+j] <= graphmode.xmax) && 
	  (Yvec.elements.Flt[N+j] >= graphmode.ymin) && 
	  (Yvec.elements.Flt[N+j] <= graphmode.ymax))
	InPic = TRUE;
    }
    // if (!status && HIDDEN) status = TRUE;
    if (!InPic) continue;

    // plist[n] = i;
    // n++;
    // if (n > npts - 1) {
    //   npts += 200;
    //   REALLOCATE (plist, int, npts);
    // }
    n ++;
    N += 2*Npts;
    if (N + 16 >= NPTS) {  /* need to leave room for 8 point image */
      NPTS += 400;
      REALLOCATE (Xvec.elements.Flt, opihi_flt, NPTS);
      REALLOCATE (Yvec.elements.Flt, opihi_flt, NPTS);
    }
  }

  gprint (GP_ERR, "plotting %d images\n", n);
  Xvec.Nelements = Yvec.Nelements = N;
  if (N > 0) {
    graphmode.style = KAPA_PLOT_POINTS; /* points */
    graphmode.ptype = KAPA_POINT_PAIR_CONNECT; /* connect pairs of points */
    graphmode.etype = 0;
    PlotVectorPair (kapa, &Xvec, &Yvec, NULL, &graphmode);
  }

  free (Xvec.elements.Flt);
  free (Yvec.elements.Flt);
  FreeImagesDVO (image);
  FREE (foundMosaic);
  return (TRUE);


 usage:
  gprint (GP_ERR, "USAGE: image [options]\n");
  gprint (GP_ERR, "  +mosaic : show mosaic outline\n");
  gprint (GP_ERR, "  -mosaic : only mosaic outline\n");
  gprint (GP_ERR, "  -hidden : (deprecated)\n");
  gprint (GP_ERR, "  -name   : only names matching (start of name)\n");
  gprint (GP_ERR, "  -radius : display all-sky images with given radius octagon\n");
  gprint (GP_ERR, "  -time (start) (range)  : show images for time and interval\n");
  gprint (GP_ERR, "  -trange (start) (stop) : show images within time range\n");
  return (FALSE);
}



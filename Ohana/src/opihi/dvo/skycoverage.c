# include "dvoshell.h"

// enum to define possible modes
enum {COVERAGE, DENSITY, MIN_UBERCAL, MIN_DMAG_SYS, MIN_MCAL, MAX_MCAL, MIN_TIME, MAX_TIME};

int wordhash (char *word);

int skycoverage (int argc, char **argv) {

  int WITH_MOSAIC, SOLO_MOSAIC, mode;
  off_t i, Nimage;
  int N, status, TimeSelect, ByName, xs, ys;
  time_t tzero, tend;
  double pixscale, dX, dY, Npts, r, d, Xi, Yi, Xs, Ys, x[2], y[2], trange, RaCenter, DecCenter;
  Image *image;
  char name[256], projection[16];
  float *V;
  int Nx, Ny;
  Buffer *buf;
  Coords coords;
  int typehash;
  int PhotcodeSelect;
  PhotCode *PhotcodeValue;

  time_t TimeReference;
  int TimeFormat;

  GetTimeFormat (&TimeReference, &TimeFormat);

  WITH_MOSAIC = FALSE;
  if ((N = get_argument (argc, argv, "+mosaic"))) {
    remove_argument (N, &argc, argv);
    WITH_MOSAIC = TRUE;
  }

  SOLO_MOSAIC = FALSE;
  if ((N = get_argument (argc, argv, "-mosaic"))) {
    remove_argument (N, &argc, argv);
    SOLO_MOSAIC = TRUE;
    WITH_MOSAIC = TRUE;
  }

  RaCenter = 0.0;
  DecCenter = 0.0;
  if ((N = get_argument (argc, argv, "-center"))) {
    remove_argument (N, &argc, argv);
    RaCenter = atof (argv[N]);
    remove_argument (N, &argc, argv);
    DecCenter = atof (argv[N]);
    remove_argument (N, &argc, argv);
  }

  Nx = 0;
  Ny = 0;
  if ((N = get_argument (argc, argv, "-size"))) {
    remove_argument (N, &argc, argv);
    Nx = atof (argv[N]);
    remove_argument (N, &argc, argv);
    Ny = atof (argv[N]);
    remove_argument (N, &argc, argv);
  }

  pixscale = 1.0;
  if ((N = get_argument (argc, argv, "-scale"))) {
    remove_argument (N, &argc, argv);
    pixscale = atof (argv[N]);
    remove_argument (N, &argc, argv);
  }

  mode = COVERAGE;
  if ((N = get_argument (argc, argv, "-density"))) {
    remove_argument (N, &argc, argv);
    mode = DENSITY;
  }
  if ((N = get_argument (argc, argv, "-min-ubercal"))) {
    remove_argument (N, &argc, argv);
    mode = MIN_UBERCAL;
  }
  if ((N = get_argument (argc, argv, "-min-time"))) {
    remove_argument (N, &argc, argv);
    mode = MIN_TIME;
  }
  if ((N = get_argument (argc, argv, "-max-time"))) {
    remove_argument (N, &argc, argv);
    mode = MAX_TIME;
  }
  if ((N = get_argument (argc, argv, "-min-dmag-sys"))) {
    remove_argument (N, &argc, argv);
    mode = MIN_DMAG_SYS;
  }
  if ((N = get_argument (argc, argv, "-min-mcal"))) {
    remove_argument (N, &argc, argv);
    mode = MIN_MCAL;
  }
  if ((N = get_argument (argc, argv, "-max-mcal"))) {
    remove_argument (N, &argc, argv);
    mode = MAX_MCAL;
  }

  ByName = FALSE;
  if ((N = get_argument (argc, argv, "-name"))) {
    remove_argument (N, &argc, argv);
    strcpy (name, argv[N]);
    remove_argument (N, &argc, argv);
    ByName = TRUE;
  }

  strcpy (projection, "DEC--AIT");
  if ((N = get_argument (argc, argv, "-proj"))) {
    remove_argument (N, &argc, argv);
    if (!strcasecmp(argv[N], "TAN")) {
	strcpy (projection, "DEC--TAN");
    }	
    if (!strcasecmp(argv[N], "SIN")) {
	strcpy (projection, "DEC--SIN");
    }	
    if (!strcasecmp(argv[N], "GLS")) {
	strcpy (projection, "DEC--GLS");
    }	
    if (!strcasecmp(argv[N], "PAR")) {
	strcpy (projection, "DEC--PAR");
    }	
    remove_argument (N, &argc, argv);
  }

  PhotcodeValue = NULL;
  PhotcodeSelect = FALSE;
  if ((N = get_argument (argc, argv, "-photcode"))) {
    if (!InitPhotcodes ()) return (FALSE);
    PhotcodeSelect = TRUE;
    remove_argument (N, &argc, argv);
    PhotcodeValue = GetPhotcodebyName (argv[N]);
    if (PhotcodeValue == NULL) {
      gprint (GP_ERR, "photcode not found in photcode table\n");
      return (FALSE);
    }
    remove_argument (N, &argc, argv);
  }

  TimeSelect = FALSE;
  if ((N = get_argument (argc, argv, "-time"))) {
    remove_argument (N, &argc, argv);
    if (!ohana_str_to_time (argv[N], &tzero)) { 
      gprint (GP_ERR, "syntax error\n");
      return (FALSE);
    }
    remove_argument (N, &argc, argv);
    if (!ohana_str_to_dtime (argv[N], &trange)) { 
      gprint (GP_ERR, "syntax error\n");
      return (FALSE);
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
      return (FALSE);
    }
    remove_argument (N, &argc, argv);
    if (!ohana_str_to_time (argv[N], &tend)) { 
      gprint (GP_ERR, "syntax error\n");
      return (FALSE);
    }
    remove_argument (N, &argc, argv);
    trange = tend - tzero;
    if (trange < 0) {
      trange = fabs (trange);
      tzero -= trange;
    }
    TimeSelect = TRUE;
  }
 
  if (argc != 3) {
    gprint (GP_ERR, "USAGE: skycoverage (buffer) (Npts)\n");
    gprint (GP_ERR, "  options: [-scale pixscale] [-center ra dec] [-size Nx Nx] [-proj projection] [-time start range] [-trange start stop] [-name name] [-photcode name] [+mosaic] [-mosaic] [-density]\n");
    gprint (GP_ERR, "       (buffer) saves bitmapped image\n");
    gprint (GP_ERR, "       (Npts) gives the number of test points per image in each dimension\n");
    gprint (GP_ERR, "       -scale (pixscale)  : specifies the pixel size in degrees [1.0]\n");
    gprint (GP_ERR, "       -center (ra) (dec) : specifies the center of the field [0.0, 0.0]\n");
    gprint (GP_ERR, "       -size (Nx) (Ny)    : specifies the size of the image [360/scale, 180/scale]\n");
    gprint (GP_ERR, "       -proj (projection) : specifies the projection choice [AIT]\n");
    gprint (GP_ERR, "       -density           : create image with relative density (else binary on/off)\n");
    gprint (GP_ERR, "       note: we need 64800 / (pixscale)^2 pixels to represent the sky\n");
    return (FALSE);
  }
  
  if ((buf = SelectBuffer (argv[1], ANYBUFFER, TRUE)) == NULL) return (FALSE);
  Npts = atof(argv[2]);

  if (!Nx || !Ny) {
    Nx = 360/pixscale;
    Ny = 180/pixscale;
  }

  gfits_free_matrix (&buf[0].matrix);
  gfits_free_header (&buf[0].header);
  if (!CreateBuffer (buf, Nx, Ny, -32, 0.0, 1.0)) return FALSE;
  strcpy (buf[0].file, "(empty)");

  InitCoords (&coords, projection);
  coords.crval1 = RaCenter;
  coords.crval2 = DecCenter;
  coords.crpix1 = 0.5*Nx;
  coords.crpix2 = 0.5*Ny;
  coords.pc1_1 = -1.0; // sky parity
  coords.pc2_2 = +1.0;
  coords.cdelt1 = coords.cdelt2 = pixscale;

  PutCoords (&coords, &buf[0].header);

  image = LoadImagesDVO(&Nimage);
  if (image == NULL) {
      return (FALSE);
  }
  // BuildChipMatch (image, Nimage);

  V = (float *)buf[0].matrix.buffer;

  // init the V array (depends on mode)
  for (ys = 0; ys < Ny; ys++) {
    for (xs = 0; xs < Nx; xs++) {
      status = XY_to_RD (&r, &d, (double)(xs), (double)(ys), &coords);
      r = ohana_normalize_angle (r);

      // are we in a part of the projection covering the sky or not?
      if (status) {
	switch (mode) {
	  case COVERAGE:
	    V[ys*Nx + xs] = 2;
	    break;
	  case DENSITY:
	    V[ys*Nx + xs] = 0;
	    break;
	  case MIN_UBERCAL:
	  case MIN_DMAG_SYS:
	  case MIN_MCAL:
	  case MIN_TIME:
	    V[ys*Nx + xs] = 1E9;
	    break;
	  case MAX_MCAL:
	  case MAX_TIME:
	    V[ys*Nx + xs] = -1E9;
	    break;
	}
      } else {
	switch (mode) {
	  case COVERAGE:
	    V[ys*Nx + xs] = 0;
	    break;
	  case DENSITY:
	  case MIN_UBERCAL:
	  case MIN_DMAG_SYS:
	  case MIN_MCAL:
	  case MAX_MCAL:
	  case MIN_TIME:
	  case MAX_TIME:
	    V[ys*Nx + xs] = NAN;
	    break;
	}
      }
    }
  }

  int DistortImage = wordhash ("-DIS");

  for (i = 0; i < Nimage; i++) {
    if (ByName && strcmp (name, image[i].name)) continue;
    if (TimeSelect && ((image[i].tzero < tzero) || (image[i].tzero+image[i].trate*image[i].NY > tzero + trange))) continue;

    if (PhotcodeSelect) {
      if (PhotcodeValue[0].type == PHOT_DEP) {
	if (PhotcodeValue[0].code != image[i].photcode) continue;
      } else {
	if (PhotcodeValue[0].code != GetPhotcodeEquivCodebyCode (image[i].photcode)) continue;
      }
    }

    typehash = wordhash (&image[i].coords.ctype[4]);

    /* DIS images represent a field, not a chip */
    if ((typehash == DistortImage) && !WITH_MOSAIC) continue;
    if ((typehash != DistortImage) &&  SOLO_MOSAIC) continue;

    /* project this image to screen display coords */
    /* DIS images represent a field, not a chip */
    if (!strcmp(&image[i].coords.ctype[4], "-DIS")) {
      x[0] = -0.5*image[i].NX; y[0] = -0.5*image[i].NY;
      x[1] = +0.5*image[i].NX; y[1] = +0.5*image[i].NY;
    } else {
      x[0] = 0;                y[0] = 0;
      x[1] = image[i].NX;      y[1] = image[i].NY;
    }
    status = FALSE;
    
    dX = (x[1] - x[0]) / Npts;
    dY = (y[1] - y[0]) / Npts;
    
    for (Yi = y[0] + 0.5*dY; Yi < y[1]; Yi += dY) {
      for (Xi = x[0] + 0.5*dX; Xi < x[1]; Xi += dX) {
	XY_to_RD (&r, &d, Xi, Yi, &image[i].coords);
	r = ohana_normalize_angle (r);
	if (r - RaCenter > +180.0) r -= 360.0;
	if (r - RaCenter < -180.0) r += 360.0;
	status = RD_to_XY (&Xs, &Ys, r, d, &coords);
	if (Xs < 0) continue;
	if (Ys < 0) continue;
	if (Xs >= Nx) continue;
	if (Ys >= Ny) continue;
	if (status) {
	  xs = (int)Xs;
	  ys = (int)Ys;
	  switch (mode) {
	    case COVERAGE:
	      V[ys*Nx + xs] = 1;
	      break;
	    case DENSITY:
	      V[ys*Nx + xs] += 1;
	      break;
	    case MIN_UBERCAL:
	      V[ys*Nx + xs] = MIN(V[ys*Nx + xs], image[i].ubercalDist);
	      break;
	    case MIN_DMAG_SYS:
	      V[ys*Nx + xs] = MIN(V[ys*Nx + xs], image[i].dMagSys);
	      break;
	    case MIN_MCAL:
	      V[ys*Nx + xs] = MIN(V[ys*Nx + xs], image[i].McalPSF);
	      break;
	    case MAX_MCAL:
	      V[ys*Nx + xs] = MAX(V[ys*Nx + xs], image[i].McalPSF);
	      break;
	    case MIN_TIME: {
	      double timeVal = TimeValue (image[i].tzero, TimeReference, TimeFormat);
	      V[ys*Nx + xs] = MIN(V[ys*Nx + xs], timeVal);
	      break; }
	    case MAX_TIME: {
	      double timeVal = TimeValue (image[i].tzero, TimeReference, TimeFormat);
	      V[ys*Nx + xs] = MAX(V[ys*Nx + xs], timeVal);
	      break; }
	  }
	}
      }
    }
  }
  FreeImagesDVO(image);
  return (TRUE);
}



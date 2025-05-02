# include "dvoshell.h"

int wordhash (char *word);

int objectcoverage (int argc, char **argv) {

  int ShowDensity;
  int N, status, xs, ys;
  double pixscale, r, d, Xs, Ys, RaCenter, DecCenter;
  char projection[16];
  float *V;
  int k, j, invalid, Nx, Ny;
  Buffer *buf;
  Coords coords;
  int Nsecfilt;
  SkyList *skylist;
  SkyRegionSelection *selection;

  Catalog catalog; 
  catalog.average = NULL; 
  catalog.secfilt = NULL;
  catalog.measure = NULL;
  skylist = NULL;
  selection = NULL;
  RaCenter = 0.0;
  DecCenter = 0.0;
  if ((selection = SetRegionSelection (&argc, argv)) == NULL) goto escape;
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

  ShowDensity = FALSE;
  if ((N = get_argument (argc, argv, "-density"))) {
    remove_argument (N, &argc, argv);
    ShowDensity = TRUE;
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

  // double trange;
  // time_t tzero, tend;
  // int TimeSelect = FALSE;
  // if ((N = get_argument (argc, argv, "-time"))) {
  //   remove_argument (N, &argc, argv);
  //   if (!ohana_str_to_time (argv[N], &tzero)) { 
  //     gprint (GP_ERR, "syntax error\n");
  //     return (FALSE);
  //   }
  //   remove_argument (N, &argc, argv);
  //   if (!ohana_str_to_dtime (argv[N], &trange)) { 
  //     gprint (GP_ERR, "syntax error\n");
  //     return (FALSE);
  //   }
  //   remove_argument (N, &argc, argv);
  //   if (trange < 0) {
  //     trange = fabs (trange);
  //     tzero -= trange;
  //   }
  //   TimeSelect = TRUE;
  // }
  // if ((N = get_argument (argc, argv, "-trange"))) {
  //   remove_argument (N, &argc, argv);
  //   if (!ohana_str_to_time (argv[N], &tzero)) { 
  //     gprint (GP_ERR, "syntax error\n");
  //     return (FALSE);
  //   }
  //   remove_argument (N, &argc, argv);
  //   if (!ohana_str_to_time (argv[N], &tend)) { 
  //     gprint (GP_ERR, "syntax error\n");
  //     return (FALSE);
  //   }
  //   remove_argument (N, &argc, argv);
  //   trange = tend - tzero;
  //   if (trange < 0) {
  //     trange = fabs (trange);
  //     tzero -= trange;
  //   }
  //   TimeSelect = TRUE;
  // }
 
  if (argc != 3) {
    gprint (GP_ERR, "USAGE: objectcoverage (buffer) (photcode)\n");
    gprint (GP_ERR, "  options: [-scale pixscale] [-center ra dec] [-size Nx Nx] [-proj projection] [-time start range] [-trange start stop] [-name name] [+mosaic] [-mosaic] [-density]\n");
    gprint (GP_ERR, "       (buffer) saves bitmapped image\n");
    gprint (GP_ERR, "       (photcode) ..........\n");
    gprint (GP_ERR, "       -scale (pixscale)  : specifies the pixel size in degrees [1.0]\n");
    gprint (GP_ERR, "       -center (ra) (dec) : specifies the center of the field [0.0, 0.0]\n");
    gprint (GP_ERR, "       -size (Nx) (Ny)    : specifies the size of the image [360/scale, 180/scale]\n");
    gprint (GP_ERR, "       -proj (projection) : specifies the projection choice [AIT]\n");
    gprint (GP_ERR, "       -density           : create image with relative density (else binary on/off)\n");
    gprint (GP_ERR, "       note: we need 64800 / (pixscale)^2 pixels to represent the sky\n");
    return (FALSE);
  }
  
  if ((buf = SelectBuffer (argv[1], ANYBUFFER, TRUE)) == NULL) return (FALSE);
  int myCode = GetPhotcodeCodebyName (argv[2]);
  if (myCode == 0) {
      gprint (GP_ERR, "invalid photcode\n");
      return (FALSE);
  }

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

  V = (float *)buf[0].matrix.buffer;

  for (ys = 0; ys < Ny; ys++) {
    for (xs = 0; xs < Nx; xs++) {
      status = XY_to_RD (&r, &d, (double)(xs), (double)(ys), &coords);
      r = ohana_normalize_angle (r);
      if (status) {
	V[ys*Nx + xs] = ShowDensity ?  0 : 2;
      } else {
	V[ys*Nx + xs] = ShowDensity ? -1 : 0;
      }
    }
  }

  Nsecfilt = GetPhotcodeNsecfilt();
  // grab data from all selected sky regions

  /* load region corresponding to selection above */
  if ((skylist = SelectRegions (selection)) == NULL) goto escape;

  struct sigaction *old_sigaction = SetInterrupt();

  /* loop over regions, extract data for each region */
  for (k = 0; (k < skylist[0].Nregions) && !interrupt; k++) {
    /* lock, load, unlock catalog */
    dvo_catalog_init (&catalog, TRUE);
    catalog.filename = skylist[0].filename[k];
    catalog.catflags = DVO_LOAD_AVERAGE | DVO_LOAD_MEASURE | DVO_LOAD_SECFILT;
    catalog.Nsecfilt = 0;

    // an error exit status here is a significant error
    if (!dvo_catalog_open (&catalog, NULL, FALSE, "r")) {
      fprintf (stderr, "ERROR: failure to open catalog file %s\n", catalog.filename);
      exit (2);
    }
    dvo_catalog_unlock (&catalog);

    int Nsec = GetPhotcodeNsec(myCode);

    for (j = 0; j <  catalog.Naverage; j++) {
	if (catalog.average[j].Nmeasure < 2) { continue; }

	
	if (catalog.secfilt[j*Nsecfilt+Nsec].Ncode < 2) { continue; }

	invalid = ((catalog.secfilt[j*Nsecfilt + Nsec].MpsfChp < 1.0) || (isnan(catalog.secfilt[j*Nsecfilt + Nsec].MpsfChp)));
	if (invalid) continue;
	
	double r = catalog.average[j].R;
	double d = catalog.average[j].D;
	status = RD_to_XY (&Xs, &Ys, r, d, &coords);
	if (Xs < 0) continue;
	if (Ys < 0) continue;
	if (Xs >= Nx) continue;
	if (Ys >= Ny) continue;
	if (status) {
	    xs = (int)Xs;
	    ys = (int)Ys;
	    if (ShowDensity) {
		V[ys*Nx + xs] += 1;
	    } else {
		V[ys*Nx + xs] = 1;
	    }
	}
    }
  }
  ClearInterrupt (old_sigaction);
  return (TRUE);

escape:
  return (FALSE);
}



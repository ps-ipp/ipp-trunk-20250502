# include "dvoshell.h"

// XXX this needs some help: it should define the region for image_subset or otherwise
// limit the input selection
int gimages (int argc, char **argv) {
  
  off_t i, j, Nimage, *subset, Nsubset;
  int N, Nfound, status;
  double ra, dec, Ra, Dec, X, Y, Yo;
  double trange, t;
  int TimeSelect, PixelCoords, TimeFormat, PhotCodeSelect;
  time_t tzero, TimeReference;
  char name[64], *date;
  int typehash;
  SkyRegionSelection *selection;

  PhotCode *code;
  Image *image;

  if (!InitPhotcodes ()) return (FALSE);

  GetTimeFormat (&TimeReference, &TimeFormat);

  // parse skyregion options
  if ((selection = SetRegionSelection (&argc, argv)) == NULL) {
    gprint (GP_ERR, "invalid sky region selection\n");
    return FALSE;
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
    TimeSelect = TRUE;
  }
  if ((N = get_argument (argc, argv, "-tref"))) {
    remove_argument (N, &argc, argv);
    TimeSelect = TRUE;

    t = atof (argv[N]);
    tzero = TimeRef (t, TimeReference, TimeFormat);
    remove_argument (N, &argc, argv);

    if (!ohana_str_to_dtime (argv[N], &trange)) { 
      gprint (GP_ERR, "syntax error\n");
      return (FALSE);
    }
    remove_argument (N, &argc, argv);
  }

  code = NULL;
  PhotCodeSelect = FALSE;
  if ((N = get_argument (argc, argv, "-photcode"))) {
    remove_argument (N, &argc, argv);
    if ((code = GetPhotcodebyName (argv[N])) == NULL) {
      gprint (GP_ERR, "ERROR: photcode not found in photcode table\n");
      return (FALSE);
    }
    PhotCodeSelect = TRUE;
    remove_argument (N, &argc, argv);
  }

  PixelCoords = FALSE;
  if ((N = get_argument (argc, argv, "-pix"))) {
    remove_argument (N, &argc, argv);
    PixelCoords = TRUE;
  }

  if (argc != 3) {
    gprint (GP_ERR, "USAGE: gimages RA DEC [-time t dt] [-pix]\n");
    return (FALSE);
  }

  if (!ohana_str_to_radec (&Ra, &Dec, argv[1], argv[2])) return (FALSE);

  if ((image = LoadImagesDVO (&Nimage)) == NULL) return (FALSE);
  image_subset (image, Nimage, &subset, &Nsubset, selection, tzero, trange, TimeSelect);
  // BuildChipMatch (image, Nimage);

  int DistortImage = wordhash ("-DIS");
  int TriangleUp   = wordhash ("TRP-");
  int TriangleDn   = wordhash ("TRM-");

  Nfound = 0;
  for (j = 0; j < Nsubset; j++) {
    i = subset[j];
    if (PhotCodeSelect) {
      if ((code[0].type == PHOT_REF) || (code[0].type == PHOT_DEP)) {
	if (code[0].code != image[i].photcode) continue;
      } 
      if (code[0].type == PHOT_SEC) {
	if (code[0].code != GetPhotcodeEquivCodebyCode (image[i].photcode)) continue;
      } 
    }      

    typehash = wordhash (&image[i].coords.ctype[4]);

    // for non-linear astrometry solutions, a point which is far from the image may
    // potentially appear to land in the image: at large field-position, the high order
    // terms which are supposed to be perturbations may dominate the solution

    // XXX this code checks if the image is even in the ballpark.  this is not a great
    // solution.  the better way to do this would be to use the linear portions of the fit
    // only for RD_to_XY, then see if the result is close.
    { 
      Coords local;
      double Ro, Do, Xo, Yo, Xs, Ys, Radius;
      
      if (typehash == DistortImage) {
	Xo = 0.0;
	Yo = 0.0;
      } else {
	Xo = 0.5*image[i].NX;
	Yo = 0.5*image[i].NY;
      }

      // find coordinates of image center
      XY_to_RD (&Ro, &Do, Xo, Yo, &image[i].coords);
      if (fabs(Ro - Ra) > 120.0) continue;

      InitCoords (&local, "DEC--TAN");
      local.crval1 = Ro;
      local.crval2 = Do;
      local.cdelt1 = local.cdelt2 = 1.0/3600.0;

      if (typehash == DistortImage) {
	Xs = -0.5*image[i].NX;
	Ys = -0.5*image[i].NY;
      } else {
	Xs = 0.0;
	Ys = 0.0;
      }
      
      // find coordinates of an image corner
      XY_to_RD (&Ro, &Do, Xs, Ys, &image[i].coords);

      // find radius of image in arcsec
      RD_to_XY (&Xo, &Yo, Ro, Do, &local);
      Radius = hypot (Xo, Yo);
      // fprintf (stderr, "%s: %f %f    %f ", image[i].name, local.crval1, local.crval2, Radius);

      // check for distances to coordinates in arcsec
      RD_to_XY (&Xo, &Yo, Ra, Dec, &local);
      // fprintf (stderr, " : %f\n", hypot(Xo,Yo));

      // skip images with center too far from coordinaes
      if (hypot(Xo,Yo) > 1.5*Radius) continue;
      // fprintf (stderr, " ** try me **\n");
    }

    status = RD_to_XY (&X, &Y, Ra, Dec, &image[i].coords);
    if (!finite(X)) continue;
    if (!finite(Y)) continue;
    if (!status) continue;

    if (typehash == DistortImage) {
      if (X < -0.5*image[i].NX) continue;
      if (Y < -0.5*image[i].NY) continue;
      if (X > +0.5*image[i].NX) continue;
      if (Y > +0.5*image[i].NY) continue;
      goto got_spot;
    } 

    typehash = wordhash (image[i].coords.ctype);
    if (typehash == TriangleUp) {
      if (Y < -0.5*image[i].NY) continue;
      Yo = +0.5*image[i].NY + 2.0*(image[i].NY/image[i].NX)*X;
      if (Y > Yo) continue;
      Yo = +0.5*image[i].NY - 2.0*(image[i].NY/image[i].NX)*X;
      if (Y > Yo) continue;
      goto got_spot;
    }
    if (typehash == TriangleDn) {
      if (Y > +0.5*image[i].NY) continue;
      Yo = -0.5*image[i].NY + 2.0*(image[i].NY/image[i].NX)*X;
      if (Y < Yo) continue;
      Yo = -0.5*image[i].NY - 2.0*(image[i].NY/image[i].NX)*X;
      if (Y < Yo) continue;
      goto got_spot;
    }

    {
      if (X < 0) continue;
      if (Y < 0) continue;
      if (X > image[i].NX) continue;
      if (Y > image[i].NY) continue;
    }

    // XXX Mcal = applyMcal (&image[i], 2048.0, 2048.0);

  got_spot:
    date = ohana_sec_to_date (image[i].tzero);

    // double-check coorsd:
# if 0
    {
	double Rout, Dout;
	status = XY_to_RD (&Rout, &Dout, X, Y, &image[i].coords);
	fprintf (stderr, "r,d = %f,%f\n", Rout, Dout);
	status = RD_to_XY (&X, &Y, Ra, Dec, &image[i].coords);
	fprintf (stderr, "x,y = %f,%f\n", X, Y);
    }
# endif

    if (PixelCoords) {
      gprint (GP_LOG, "%3d %5d %s %6.1f %6.1f %20s %5d %2d %4.2f %6.3f %5.3f %5.3f %4x %7d\n", 
	      Nfound, (int) i, image[i].name, X, Y, date, image[i].nstar, image[i].photcode, image[i].secz, image[i].McalPSF, image[i].dMcal, image[i].exptime, image[i].flags, image[i].imageID);
    } else {
      XY_to_RD (&ra, &dec, 0.5*image[i].NX, 0.5*image[i].NY, &image[i].coords);
      gprint (GP_LOG, "%3d %5d %s %8.4f %8.4f %20s %5d %2d %4.2f %6.3f %5.3f %5.3f %4x %7d\n", 
	      Nfound, (int) i, image[i].name, ra, dec, date, image[i].nstar, image[i].photcode, image[i].secz, image[i].McalPSF, image[i].dMcal, image[i].exptime, image[i].flags, image[i].imageID);
    }
    sprintf (name, "IMAGEx:%d", Nfound);
    set_variable     (name, X);
    sprintf (name, "IMAGEy:%d", Nfound);
    set_variable     (name, Y);
    sprintf (name, "IMAGEt:%d", Nfound);
    set_str_variable (name, date);
    sprintf (name, "IMAGEccd:%d", Nfound);
    set_int_variable (name, image[i].ccdnum);
    sprintf (name, "IMAGEname:%d", Nfound);
    set_str_variable (name, image[i].name);
    sprintf (name, "IMAGEphotcode:%d", Nfound);
    set_int_variable (name, image[i].photcode);
    Nfound ++;
    free (date);
  }
  set_int_variable ("IMAGEx:n", Nfound);
  set_int_variable ("IMAGEy:n", Nfound);
  set_int_variable ("IMAGEt:n", Nfound);
  set_int_variable ("IMAGEccd:n", Nfound);
  set_int_variable ("IMAGEname:n", Nfound);
  set_int_variable ("IMAGEphotcode:n", Nfound);

  FreeImagesDVO (image);
  free (subset);

  return (TRUE);

}

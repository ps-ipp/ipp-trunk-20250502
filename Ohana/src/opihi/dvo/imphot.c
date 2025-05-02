# include "dvoshell.h"

int imphot (int argc, char **argv) {
  
  time_t tzero;
  double trange;
  int N, GreyScale;
  off_t i, j, Nimage, Nsubset, *subset;
  char bufname[64];
  float *p;
  double x, y;
  Image *image;
  Buffer *buf;
  SkyRegionSelection *selection;

  GreyScale = FALSE;
  if ((N = get_argument (argc, argv, "-g"))) {
    remove_argument (N, &argc, argv);
    strcpy (bufname, argv[N]);
    remove_argument (N, &argc, argv);
    GreyScale = TRUE;
  }

  // parse skyregion options
  if ((selection = SetRegionSelection (&argc, argv)) == NULL) {
    gprint (GP_ERR, "invalid sky region selection\n");
    return FALSE;
  }

  if (argc != 3) {
    gprint (GP_ERR, "USAGE: imphot tzero trange [-g buffer]\n");
    return (FALSE);
  }

  buf = NULL;
  if (GreyScale) {
    if ((buf = SelectBuffer (bufname, ANYBUFFER, TRUE)) == NULL) return (FALSE);
    if (!CreateBuffer (buf, 100, 200, -32, 0.0, 1.0)) return FALSE;
  }

  /* load image(s) in time range given */
  if (!ohana_str_to_time (argv[1], &tzero)) { 
    gprint (GP_ERR, "syntax error\n");
    return (FALSE);
  }
  if (!ohana_str_to_dtime (argv[2], &trange)) { 
    gprint (GP_ERR, "syntax error\n");
    return (FALSE);
  }    
  gprint (GP_ERR, "searching in range %ds - %ds (%f seconds)\n", (int)tzero, (int)(tzero + trange), trange);
  
  if ((image = LoadImagesDVO (&Nimage)) == NULL) return (FALSE);
  image_subset (image, Nimage, &subset, &Nsubset, selection, tzero, trange, TRUE);

  if ((Nsubset > 1) && GreyScale) {
    gprint (GP_ERR, "more than one image selected, making GreyScale of first only\n");
  }

  if (GreyScale && Nsubset) {
    // double fx = image[subset[0]].NX / 100;
    // double fy = image[subset[0]].NY / 200;
    p = (float *) buf[0].matrix.buffer;
    for (y = 0; y < 200; y+=1.0) {
      for (x = 0; x < 100; x+=1.0, p++) {
	// *p = applyMcal (&image[subset[0]], (fx*x), (fy*y));
	*p = image[subset[0]].McalPSF;
      }
    }
  }

  for (j = 0; j < Nsubset; j++) {
    i = subset[j];
    gprint (GP_ERR, "%s: %f\n", image[i].name, image[i].McalPSF);

// XXX old code when we had the option of a 2D zero point model
# if (0)      
    switch (image[i].order) {
    case 0:
      gprint (GP_ERR, "%s: %d - %f\n", image[i].name, image[i].order, image[i].McalPSF);
      break;
    case 1:
      gprint (GP_ERR, "%s: %d - %f, %d %d\n", image[i].name, image[i].order, image[i].McalPSF, image[i].Mx, image[i].My);
      break;
    case 2:
      gprint (GP_ERR, "%s: %d - %f, %d %d, %d %d %d\n", image[i].name, image[i].order, image[i].McalPSF, image[i].Mx, image[i].My, image[i].Mxx, image[i].Mxy, image[i].Myy);
      break;
    case 3:
      gprint (GP_ERR, "%s: %d - %f, %d %d, %d %d %d, %d %d %d %d\n", image[i].name, image[i].order, image[i].McalPSF, image[i].Mx, image[i].My, 
	       image[i].Mxx, image[i].Mxy, image[i].Myy, image[i].Mxxx, image[i].Mxxy, image[i].Mxyy, image[i].Myyy);
      break;
    case 4:
      gprint (GP_ERR, "%s: %d - %f, %d %d, %d %d %d, %d %d %d %d, %d %d %d %d %d\n", image[i].name, image[i].order, image[i].McalPSF, image[i].Mx, image[i].My, 
	       image[i].Mxx, image[i].Mxy, image[i].Myy, image[i].Mxxx, image[i].Mxxy, image[i].Mxyy, image[i].Myyy,
	       image[i].Mxxxx, image[i].Mxxxy, image[i].Mxxyy, image[i].Mxyyy, image[i].Myyyy);
      break;
    }
# endif
  }

  FreeImagesDVO(image);
  free (subset);
  return (TRUE);
}


# include "dvoshell.h"

int coordimage (int argc, char **argv) {

  int N;
  off_t i, j, Nimage, *subset, Nsubset;
  time_t tzero;
  double trange;
  Buffer *bufX, *bufY;
  SkyRegionSelection *selection;
  Image *image;

  if (!InitPhotcodes ()) return (FALSE);

  // parse skyregion options
  if ((selection = SetRegionSelection (&argc, argv)) == NULL) {
    gprint (GP_ERR, "invalid sky region selection\n");
    return FALSE;
  }

  int TimeSelect = FALSE;
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
    gprint (GP_ERR, "plotting in range %ds - %ds (%f seconds)\n", (int)tzero, (int)(tzero + trange), trange);
  }

  PhotCode *Photcode = NULL;
  if ((N = get_argument (argc, argv, "-photcode"))) {
    remove_argument (N, &argc, argv);
    Photcode = GetPhotcodebyName (argv[N]);
    if (Photcode == NULL) {
      gprint (GP_ERR, "photcode not found in photcode table\n");
      return (FALSE);
    }
    remove_argument (N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "-Nphotcode"))) {
    remove_argument (N, &argc, argv);
    Photcode = GetPhotcodebyCode (atoi(argv[N]));
    if (Photcode == NULL) {
      gprint (GP_ERR, "photcode not found in photcode table\n");
      return (FALSE);
    }
    remove_argument (N, &argc, argv);
  }

  char *Name = NULL;
  if ((N = get_argument (argc, argv, "-name"))) {
    remove_argument (N, &argc, argv);
    Name = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }

  int chipID = -1;
  if ((N = get_argument (argc, argv, "-chip"))) {
    remove_argument (N, &argc, argv);
    chipID = atoi (argv[N]);
    remove_argument (N, &argc, argv);
  }
  
  if (argc != 5) goto syntax;

  if ((bufX = SelectBuffer (argv[1], ANYBUFFER, TRUE)) == NULL) goto escape;
  if ((bufY = SelectBuffer (argv[2], ANYBUFFER, TRUE)) == NULL) goto escape;
  int Nx = atoi (argv[3]);
  int Ny = atoi (argv[4]);

  gfits_free_matrix (&bufX[0].matrix);
  gfits_free_header (&bufX[0].header);
  if (!CreateBuffer (bufX, Nx, Ny, -32, 0.0, 1.0)) return FALSE; // initialized to 0.0 here
  strcpy (bufX[0].file, "(empty)");

  gfits_free_matrix (&bufY[0].matrix);
  gfits_free_header (&bufY[0].header);
  if (!CreateBuffer (bufY, Nx, Ny, -32, 0.0, 1.0)) return FALSE; // initialized to 0.0 here
  strcpy (bufY[0].file, "(empty)");

  if ((image = LoadImagesDVO (&Nimage)) == NULL) return (FALSE);
  image_subset (image, Nimage, &subset, &Nsubset, selection, tzero, trange, TimeSelect);

  float Nsum = 0.0;
  float *vX = (float *) bufX[0].matrix.buffer;
  float *vY = (float *) bufY[0].matrix.buffer;

  for (j = 0; j < Nsubset; j++) {
    i = subset[j];
    if (Name && (strstr (image[i].name, Name) == (char *) NULL)) continue;
    if (Photcode) {
      if (Photcode[0].type == PHOT_DEP) {
	if (Photcode[0].code != image[i].photcode) continue;
      } else {
	if (Photcode[0].code != GetPhotcodeEquivCodebyCode (image[i].photcode)) continue;
      }
    }
    if (chipID > -1) {
      if (image[i].ccdnum != chipID) continue;
    }

    float dX = image[i].NX / (float) Nx;
    float dY = image[i].NY / (float) Ny;

    Coords coords = image[i].coords;
    coords.Npolyterms = 0;

    int ix, iy;
    for (iy = 0; iy < Ny; iy++) {
      double fy = iy * dY; // coordinate in full image
      for (ix = 0; ix < Nx; ix++) {
	double fx = ix * dX; // coordinate in full image

	double Lo, Lm, dL;
	double Mo, Mm, dM;

	XY_to_LM (&Lo, &Mo, fx, fy, &coords);
	XY_to_LM (&Lm, &Mm, fx, fy, &image[i].coords);
	dL = Lo - Lm;
	dM = Mo - Mm;

	vX[ix + iy*Nx] += dL;
	vY[ix + iy*Nx] += dM;
      }
    }
    Nsum ++;
  }
  fprintf (stderr, "Nsum: %f\n", Nsum);
  return TRUE;

 syntax:
  gprint (GP_ERR, "USAGE: coordimage buffX buffY Nx Ny\n");
 escape:
  return (FALSE);
}

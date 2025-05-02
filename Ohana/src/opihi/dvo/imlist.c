# include "dvoshell.h"

int imlist (int argc, char **argv) {
  
  off_t i, j, Nimage, *subset, Nsubset;
  int N, TimeSelect, TimeFormat, NameSelect;
  int PhotcodeSelect;
  time_t tzero, TimeReference;
  double r, d, trange, t;
  char *name;
  Image *image;
  PhotCode *PhotcodeValue;
  SkyRegionSelection *selection;

  if (!InitPhotcodes ()) return (FALSE);

  // parse skyregion options
  if ((selection = SetRegionSelection (&argc, argv)) == NULL) {
    gprint (GP_ERR, "invalid sky region selection\n");
    return FALSE;
  }

  int VERBOSE = TRUE;
  if ((N = get_argument (argc, argv, "-quiet"))) {
    VERBOSE = FALSE;
    remove_argument (N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "-q"))) {
    VERBOSE = FALSE;
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
    TimeSelect = TRUE;
    if (VERBOSE) gprint (GP_ERR, "plotting in range %ds - %ds (%f seconds)\n", (int)tzero, (int)(tzero + trange), trange);
  }

  PhotcodeValue = NULL;
  PhotcodeSelect = FALSE;
  if ((N = get_argument (argc, argv, "-photcode"))) {
    PhotcodeSelect = TRUE;
    remove_argument (N, &argc, argv);
    PhotcodeValue = GetPhotcodebyName (argv[N]);
    if (PhotcodeValue == NULL) {
      gprint (GP_ERR, "photcode not found in photcode table\n");
      return (FALSE);
    }
    remove_argument (N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "-Nphotcode"))) {
    PhotcodeSelect = TRUE;
    remove_argument (N, &argc, argv);
    PhotcodeValue = GetPhotcodebyCode (atoi(argv[N]));
    if (PhotcodeValue == NULL) {
      gprint (GP_ERR, "photcode not found in photcode table\n");
      return (FALSE);
    }
    remove_argument (N, &argc, argv);
  }

  name = NULL;
  NameSelect = FALSE;
  if ((N = get_argument (argc, argv, "-name"))) {
    remove_argument (N, &argc, argv);
    name = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
    NameSelect = TRUE;
  }

  int showUL = FALSE;
  if ((N = get_argument (argc, argv, "-show-ul"))) {
    remove_argument (N, &argc, argv);
    showUL = TRUE;
  }
  int showUR = FALSE;
  if ((N = get_argument (argc, argv, "-show-ur"))) {
    remove_argument (N, &argc, argv);
    showUR = TRUE;
  }
  int showLL = FALSE;
  if ((N = get_argument (argc, argv, "-show-ll"))) {
    remove_argument (N, &argc, argv);
    showLL = TRUE;
  }
  int showLR = FALSE;
  if ((N = get_argument (argc, argv, "-show-lr"))) {
    remove_argument (N, &argc, argv);
    showLR = TRUE;
  }

  int MAX_LIST = -1;
  if ((N = get_argument (argc, argv, "-max-list"))) {
    remove_argument (N, &argc, argv);
    MAX_LIST = atoi (argv[N]);
    remove_argument (N, &argc, argv);
  }

  if (argc != 1) {
    gprint (GP_ERR, "USAGE: image [-time start range] [-region] [-name string] [-photcode code] [-Nphotcode N] [-max-list N]\n");
    return (FALSE);
  }
  
  if ((image = LoadImagesDVO (&Nimage)) == NULL) return (FALSE);
  image_subset (image, Nimage, &subset, &Nsubset, selection, tzero, trange, TimeSelect);
  MAX_LIST = MAX_LIST < 0 ? Nsubset : MIN(MAX_LIST, Nsubset);

  GetTimeFormat (&TimeReference, &TimeFormat);

  int Nfound = 0;

  for (j = 0; j < MAX_LIST; j++) {
    i = subset[j];
    if (NameSelect && (strstr (image[i].name, name) == (char *) NULL)) continue;
    if (PhotcodeSelect) {
      if (PhotcodeValue[0].type == PHOT_DEP) {
	if (PhotcodeValue[0].code != image[i].photcode) continue;
      } else {
	if (PhotcodeValue[0].code != GetPhotcodeEquivCodebyCode (image[i].photcode)) continue;
      }
    }
    t = TimeValue (image[i].tzero, TimeReference, TimeFormat);
    if (!strcmp(&image[i].coords.ctype[4], "-DIS")) {
      XY_to_RD (&r, &d, 0.0, 0.0, &image[i].coords);
    } else {
      XY_to_RD (&r, &d, 0.5*image[i].NX, 0.5*image[i].NY, &image[i].coords);
    }


    if (VERBOSE) {
      gprint (GP_LOG, "%3lld %s %8lld %8.4f %8.4f %f %5d %2d %4.2f %5.3f %5.3f", 
			 (long long) i, image[i].name, (long long) image[i].imageID, r, d, t, image[i].nstar, image[i].photcode, image[i].secz, image[i].McalPSF, image[i].dMcal);

      if (showUR) {
	if (!strcmp(&image[i].coords.ctype[4], "-DIS")) {
	  XY_to_RD (&r, &d, 0.5*image[i].NX, 0.5*image[i].NY, &image[i].coords);
	} else {
	  XY_to_RD (&r, &d, image[i].NX, image[i].NY, &image[i].coords);
	}
	gprint (GP_LOG, " %8.4f %8.4f", r, d);
      }
      if (showUL) {
	if (!strcmp(&image[i].coords.ctype[4], "-DIS")) {
	  XY_to_RD (&r, &d, -0.5*image[i].NX, 0.5*image[i].NY, &image[i].coords);
	} else {
	  XY_to_RD (&r, &d, 0.0, image[i].NY, &image[i].coords);
	}
	gprint (GP_LOG, " %8.4f %8.4f", r, d);
      }
      if (showLL) {
	if (!strcmp(&image[i].coords.ctype[4], "-DIS")) {
	  XY_to_RD (&r, &d, -0.5*image[i].NX, -0.5*image[i].NY, &image[i].coords);
	} else {
	  XY_to_RD (&r, &d, 0.0, 0.0, &image[i].coords);
	}
	gprint (GP_LOG, " %8.4f %8.4f", r, d);
      }
      if (showLR) {
	if (!strcmp(&image[i].coords.ctype[4], "-DIS")) {
	  XY_to_RD (&r, &d, 0.5*image[i].NX, -0.5*image[i].NY, &image[i].coords);
	} else {
	  XY_to_RD (&r, &d, image[i].NX, 0.0, &image[i].coords);
	}
	gprint (GP_LOG, " %8.4f %8.4f", r, d);
      }
      gprint (GP_LOG, "\n");
    }

    char name[80];
    sprintf (name, "imlist:%d", Nfound);
    set_str_variable (name, image[i].name);
    Nfound ++;
  }
  set_int_variable ("imlist:n", Nfound);

  FreeImagesDVO(image);
  free (subset);
  return (TRUE);
}

# include "relphot.h"

// the PS1 Synthetic magnitudes (Pitts & Magnier) have known systematic errors due to the
// POSS plate photometry.  This file contains support code to correct that error based on
// sky maps of the mean offsets.

static char *extname[5] = {"map_g", "map_r", "map_i", "map_z", "map_y"};

static SynthZeroPoints *zpts = NULL;

int SynthZeroPointsLoad (char *filename) {

  FILE *f = fopen (filename, "r");
  if (!f) {
    fprintf (stderr, "ERROR: cannot open file %s\n", filename);
    return FALSE;
  }

  // SynthZeroPoint file contains:
  // PHU header (including WCS)
  // map_g
  // map_r
  // map_i
  // map_z
  // map_y

  ALLOCATE (zpts, SynthZeroPoints, 1);
  
  if (!gfits_fread_header (f, &zpts->PHU)) {
    if (VERBOSE) fprintf (stderr, "can't read header\n");
    fclose (f);
    return FALSE;
  }

  int i;
  for (i = 0; i < 5; i++) {
    if (!gfits_find_Xheader(f, &zpts->header[i], extname[i])) {
      fprintf (stderr, "ERROR: file %s is missing extension %s\n", filename, extname[i]);
      exit (2);
    }

    if (!gfits_fread_matrix (f, &zpts->matrix[i], &zpts->header[i])) {
      if (VERBOSE) fprintf (stderr, "can't read image for %s\n", extname[i]);
      exit (2);
    }
  }

  GetCoords (&zpts->coords, &zpts->PHU);
  zpts->Nx = zpts->matrix[0].Naxis[0];
  zpts->Ny = zpts->matrix[0].Naxis[1];

  return TRUE;
}

SynthZeroPoints *SynthZeroPointsGet () {
  return zpts;
}

# include "astro.h"

int wcs (int argc, char **argv) {
  
  int N;

  int flipeast = TRUE;
  if ((N = get_argument (argc, argv, "-ew"))) {
    remove_argument (N, &argc, argv);
    flipeast = TRUE;
  }
  if ((N = get_argument (argc, argv, "+ew"))) {
    remove_argument (N, &argc, argv);
    flipeast = FALSE;
  }

  int flipnorth = FALSE;
  if ((N = get_argument (argc, argv, "-ns"))) {
    remove_argument (N, &argc, argv);
    flipnorth = TRUE;
  }
  if ((N = get_argument (argc, argv, "+ns"))) {
    remove_argument (N, &argc, argv);
    flipnorth = FALSE;
  }

  float Angle = 0.0;
  if ((N = get_argument (argc, argv, "-angle"))) {
    remove_argument (N, &argc, argv);
    Angle = atof(argv[N]);
    remove_argument (N, &argc, argv);
  }

  if ((argc != 5) && (argc != 6)) {
    gprint (GP_ERR, "USAGE: wcs (buffer) Ra Dec platescale [projection] [orientation]\n");
    gprint (GP_ERR, "  [-ew] [+ew] [-ns] [+ns] [-angle theta]\n");
    return (FALSE);
  }
  
  double Ra, Dec;
  if (!ohana_str_to_radec (&Ra, &Dec, argv[2], argv[3])) return (FALSE);
  float platescale = atof (argv[4]); // arcsec per pixel

  Buffer *buf = NULL;
  if ((buf = SelectBuffer (argv[1], OLDBUFFER, TRUE)) == NULL) {
    gprint (GP_ERR, "cannot define buffer %s\n", argv[1]);
    return FALSE;
  }

  Coords coords;
  InitCoords (&coords, "DEC--TAN");

  if (argc == 6) {
    if (!strcasecmp (argv[5], "TAN")) 
      strcpy (coords.ctype, "DEC--TAN");
    if (!strcasecmp (argv[5], "SIN")) 
      strcpy (coords.ctype, "DEC--SIN");
    if (!strcasecmp (argv[5], "ARC")) 
      strcpy (coords.ctype, "DEC--ARC");
    if (!strcasecmp (argv[5], "STG")) 
      strcpy (coords.ctype, "DEC--STG");
    if (!strcasecmp (argv[5], "ZEA"))
      strcpy (coords.ctype, "DEC--ZEA");
    if (!strcasecmp (argv[5], "AIT")) 
      strcpy (coords.ctype, "DEC--AIT");
    if (!strcasecmp (argv[5], "GLS")) 
      strcpy (coords.ctype, "DEC--GLS");
    if (!strcasecmp (argv[5], "PAR")) 
      strcpy (coords.ctype, "DEC--PAR");
  }
  
  coords.crval1 = Ra;
  coords.crval2 = Dec;

  // reference pixel is the image center
  coords.crpix1 = 0.5 * buf[0].header.Naxis[0]; // Ohana / IPP center of a pixel is X.5,X.5
  coords.crpix2 = 0.5 * buf[0].header.Naxis[1]; // Ohana / IPP center of a pixel is X.5,X.5

  float pc1_1 = flipeast  ? -1 : 1;
  float pc2_2 = flipnorth ? -1 : 1;

  coords.pc1_1 =  cos(Angle*RAD_DEG)*pc1_1;
  coords.pc1_2 =  sin(Angle*RAD_DEG)*pc2_2;
  coords.pc2_1 = -sin(Angle*RAD_DEG)*pc1_1;
  coords.pc2_2 =  cos(Angle*RAD_DEG)*pc2_2;

  coords.cdelt1 = platescale / 3600.0;
  coords.cdelt2 = platescale / 3600.0;

  PutCoords (&coords, &buf[0].header);
  return (TRUE);
}


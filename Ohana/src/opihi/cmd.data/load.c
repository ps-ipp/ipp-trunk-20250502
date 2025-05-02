# include "data.h"

int load (int argc, char **argv) {
  
  int i, N, ISCEL;
  int kapa, Noverlay, NOVERLAY;
  char *c, type[10], string[128], line[1024];
  double x, y, dx, dy, x1, y1;
  double dra, ddec, ra1, dec1, ra, dec;
  FILE *f;
  char *buffer, *name;
  Coords coords;
  Buffer *buf;
  KiiOverlay *overlay;
  KapaImageData data;
  
  name = NULL;
  if ((N = get_argument (argc, argv, "-n"))) {
    remove_argument (N, &argc, argv);
    name = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }
  if (!GetImageData (&data, &kapa, name)) return (FALSE);
  FREE (name);

  int channel = 0;
  if ((N = get_argument (argc, argv, "-ch"))) {
    channel = GetKapaChannelFromString (argv[N]);
    if (!channel) return FALSE;
    KiiSetChannel (kapa, channel - 1);
  }

  ISCEL = FALSE;
  if ((N = get_argument (argc, argv, "-c"))) {
    remove_argument (N, &argc, argv);
    ISCEL = TRUE;
  }

  if (argc != 3) {
    gprint (GP_ERR, "USAGE: load (overlay) <filename>\n [-c] [-n]");
    gprint (GP_ERR, "  -c: read overlay in celestial coords\n");
    return (FALSE);
  }
  
  if (!strcmp (argv[2], "-")) {
    f = stdin;
  } else {
    f = fopen (argv[2], "r");
  }
  if (f == NULL) {
    gprint (GP_ERR, "can't find file %s\n", argv[2]);
    return (FALSE);
  }

  if (ISCEL) {
    if ((buf = SelectBuffer (data.name, OLDBUFFER, TRUE)) == NULL) return (FALSE);
    GetCoords (&coords, &buf[0].header);
  }

  Noverlay = 0;
  NOVERLAY = 1000;
  ALLOCATE (overlay, KiiOverlay, NOVERLAY);

  ALLOCATE (buffer, char, 65536);  /* space for 512 lines of 128 bytes */
  bzero (buffer, 65536);

  dx = dy = 0;
  while (scan_line (f, line) != EOF) {
    c = strchr (line, '#');
    if (c != (char *) NULL) 
      *c = 0;  /* force end of line at comment! */
    while ((c = strchr (line, '(')) != (char *) NULL) 
      *c = ' ';
    while ((c = strchr (line, ')')) != (char *) NULL) 
      *c = ' ';
    while ((c = strchr (line, ',')) != (char *) NULL) 
      *c = ' ';
    /* we could use some syntactial checks here */
    /* have to get all three for this to be any valid object, if the line is commented out,
     we should get none, so check that N == 3 before continuing: */
    N = sscanf (line, "%s %lf %lf %lf %lf", type, &ra, &dec, &dra, &ddec);
    switch (N) {
    case 0:
    case -1:
      continue;
    case 1:
    case 2:
    case 3:
      if (strcmp (type, "TEXT")) {
	gprint (GP_ERR, "syntax error in line:\n   %s\n", line);
	continue;
      }
      sscanf (line, "%s %lf %lf %127s", type, &ra, &dec, string);
    case 4:
      ddec = dra;
    case 5:
      x = ra;
      y = dec;
      dx = dra;
      dy = ddec;
      if (ISCEL) {
	if (!strcmp (type, "LINE")) {
	  RD_to_XY (&x, &y, ra, dec, &coords);
	  ra1 = ra + dra;
	  dec1 = dec + ddec;
	  RD_to_XY (&x1, &y1, ra1, dec1, &coords);
	  dy = (y1 - y);
	  dx = (x1 - x);
	} else {
	  RD_to_XY (&x, &y, ra, dec, &coords);
	  ra1 = ra;
	  dec1 = dec + ddec;
	  RD_to_XY (&x1, &y1, ra1, dec1, &coords);
	  dy = (fabs(x1 - x) + fabs(y1 - y));
	  ra1 = ra + dra/cos(dec*RAD_DEG);;
	  dec1 = dec;
	  RD_to_XY (&x1, &y1, ra1, dec1, &coords);
	  dx = (fabs(x1 - x) + fabs(y1 - y));
	}
      }
    }
    overlay[Noverlay].type = KiiOverlayTypeByName (type);
    if (overlay[Noverlay].type == KII_OVERLAY_TEXT) {
      overlay[Noverlay].text = strcreate (string);
    } else {
      overlay[Noverlay].text = NULL;
    }
    overlay[Noverlay].x = x;
    overlay[Noverlay].y = y;
    overlay[Noverlay].dx = dx;
    overlay[Noverlay].dy = dy;
    Noverlay++;
    CHECK_REALLOCATE (overlay, KiiOverlay, NOVERLAY, Noverlay, 1000);
  }

  KiiLoadOverlay (kapa, overlay, Noverlay, argv[1]);

  for (i = 0; i < Noverlay; i++) {
    if (overlay[i].text == NULL) continue;
    free (overlay[i].text);
  }
  free (overlay);

  gprint (GP_ERR, "loaded %d objects\n", Noverlay);

  if (f != stdin) {
    fclose (f);
  }
  return (TRUE);
}


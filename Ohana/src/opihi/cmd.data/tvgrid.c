# include "data.h"
# define TEN(X) (pow(10.0, (double)(X)))

int tvgrid (int argc, char **argv) {
  
  int ndig1, ndig2, NX, NY, connect;
  int tDEC, tRA;
  double ra, dec, ra0, dec0, ra1, dec1;
  double x0, y0, x1, y1;
  double dDEC, fDEC, dRA, fRA;
  char format[16];
  Coords coords;
  int kapa, N, Noverlay, NOVERLAY;
  char *name;
  Buffer *buf;
  KiiOverlay *overlay;

  name = NULL;
  if ((N = get_argument (argc, argv, "-n"))) {
    remove_argument (N, &argc, argv);
    name = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }
  if (!GetImage (NULL, &kapa, name)) return (FALSE);
  FREE (name);

  int channel = 0;
  if ((N = get_argument (argc, argv, "-ch"))) {
    channel = GetKapaChannelFromString (argv[N]);
    if (!channel) return FALSE;
    KiiSetChannel (kapa, channel - 1);
  }

  if (argc != 3) {
    gprint (GP_ERR, "USAGE: tvgrid (overlay) (buffer)\n");
    gprint (GP_ERR, " (overlay) may be: red, green, blue, yellow\n");
    return (FALSE);
  }

  if ((buf = SelectBuffer (argv[2], OLDBUFFER, TRUE)) == NULL) return (FALSE);
  GetCoords (&coords, &buf[0].header);

  XY_to_RD (&ra0, &dec0, 0.0, 0.0, &coords);
  XY_to_RD (&ra1, &dec1, (double)buf[0].header.Naxis[0], (double)buf[0].header.Naxis[1], &coords);
  gprint (GP_ERR, "%f %f  %f %f\n", ra0, dec0, ra1, dec1);
  
  Noverlay = 0;
  NOVERLAY = 1000;
  ALLOCATE (overlay, KiiOverlay, NOVERLAY);

  dDEC = fabs(dec1 - dec0);
  tDEC = log10(dDEC) - log10(5.0);
  fDEC = log10(dDEC) - tDEC;
  if ((fDEC > log10(0.5)) && (fDEC < log10(1.0))) {
    dDEC = TEN (tDEC + log10(0.1));
  }
  if ((fDEC > log10(1.0)) && (fDEC < log10(2.0))) {
    dDEC = TEN (tDEC + log10(0.2));
  }
  if ((fDEC > log10(2.0)) && (fDEC < log10(5.0))) {
    dDEC = TEN (tDEC + log10(0.5));
  }
  if ((fDEC > log10(5.0)) && (fDEC < log10(10.0))) {
    dDEC = TEN (tDEC + log10(1.0));
  }
  if ((fDEC > log10(10.0)) && (fDEC < log10(20.0))) {
    dDEC = TEN (tDEC + log10(2.0));
  }
  if ((fDEC > log10(20.0)) && (fDEC < log10(50.0))) {
    dDEC = TEN (tDEC + log10(5.0));
  }
  ndig2 = ((log10(dDEC) < 0) ? fabs(log10(dDEC)) : 0);
  ndig1 = 3 + log10(MAX(dec0, dec1)) + ndig2;
  sprintf (format, "%%%d.%df", ndig1, ndig2);
  gprint (GP_ERR, "format: %s..\n", format);

  NX = buf[0].header.Naxis[0];
  NY = buf[0].header.Naxis[1];

  x0 = y0 = 0;
  dRA = MAX (fabs(ra1 - ra0) / 100.0, 0.1);
  connect = FALSE;
  for (dec = dDEC * ((int)(MIN(dec0,dec1)/dDEC) + 1); dec < MAX(dec0,dec1); dec += dDEC) {
    for (ra = 0; ra < 361; ra += dRA) {
      RD_to_XY (&x1, &y1, ra, dec, &coords);
      if ((x1 >= 0) && (x1 < NX) && (y1 >= 0) && (y1 < NY)) {
	if (connect) {
	  overlay[Noverlay].type = KII_OVERLAY_LINE;
	  overlay[Noverlay].x = x0;
	  overlay[Noverlay].y = y0;
	  overlay[Noverlay].dx = x1 - x0;
	  overlay[Noverlay].dy = y1 - y0;
	  Noverlay ++;
	  CHECK_REALLOCATE (overlay, KiiOverlay, NOVERLAY, Noverlay, 1000);
	}
	x0 = x1;
	y0 = y1;
	connect = TRUE;
      } else {
	connect = FALSE;
      }
    }
  }

  dRA = fabs(ra1 - ra0);
  tRA = log10(dRA) - log10(5.0);
  fRA = log10(dRA) - tRA;
  if ((fRA > log10(0.5)) && (fRA < log10(1.0))) {
    dRA = TEN (tRA + log10(0.1));
  }
  if ((fRA > log10(1.0)) && (fRA < log10(2.0))) {
    dRA = TEN (tRA + log10(0.2));
  }
  if ((fRA > log10(2.0)) && (fRA < log10(5.0))) {
    dRA = TEN (tRA + log10(0.5));
  }
  if ((fRA > log10(5.0)) && (fRA < log10(10.0))) {
    dRA = TEN (tRA + log10(1.0));
  }
  if ((fRA > log10(10.0)) && (fRA < log10(20.0))) {
    dRA = TEN (tRA + log10(2.0));
  }
  if ((fRA > log10(20.0)) && (fRA < log10(50.0))) {
    dRA = TEN (tRA + log10(5.0));
  }
  ndig2 = ((log10(dRA) < 0) ? fabs(log10(dRA)) : 0);
  ndig1 = 3 + log10(MAX(ra0, ra1)) + ndig2;
  sprintf (format, "%%%d.%df", ndig1, ndig2);
  gprint (GP_ERR, "format: %s..\n", format);

  dDEC = MAX (fabs(dec1 - dec0) / 100.0, 0.1);
  connect = FALSE;
  for (ra = dRA * ((int)(MIN(ra0,ra1)/dRA) + 1); ra < MAX(ra0,ra1); ra += dRA) {
    for (dec = -90; dec < 90; dec += dDEC) {
      RD_to_XY (&x1, &y1, ra, dec, &coords);
      if ((x1 >= 0) && (x1 < NX) && (y1 >= 0) && (y1 < NY)) {
	if (connect) {
	  overlay[Noverlay].type = KII_OVERLAY_LINE;
	  overlay[Noverlay].x = x0;
	  overlay[Noverlay].y = y0;
	  overlay[Noverlay].dx = x1 - x0;
	  overlay[Noverlay].dy = y1 - y0;
	  Noverlay ++;
	  CHECK_REALLOCATE (overlay, KiiOverlay, NOVERLAY, Noverlay, 1000);
	}
	x0 = x1;
	y0 = y1;
	connect = TRUE;
      } else {
	connect = FALSE;
      }
    }
  }

  KiiLoadOverlay (kapa, overlay, Noverlay, argv[1]);
  free (overlay);

  return (TRUE);
}

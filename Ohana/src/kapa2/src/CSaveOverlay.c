# include "Ximage.h"

int CSaveOverlay (int sock) {

  char filename[256], *type;
  int i, N;
  double ra, dec, ra1, dec1, x1, y1, dra, ddec;
  FILE *f;
  Section *section;
  KapaImageWidget *image;

  KiiScanMessage (sock, "%*s %d %s", &N, filename);
  
  section = GetActiveSection();
  image   = section->image;
  if (image == NULL) return (TRUE);

  f = fopen (filename, "w");
  if (f == NULL) {
    fprintf (stderr, "could not open %s\n", filename);
    return (TRUE);
  }

  for (i = 0; i < image[0].overlay[N].Nobjects; i++) {
    if (image[0].overlay[N].objects[i].type == KII_OVERLAY_LINE) {
      XY_to_RD (&ra, &dec, image[0].overlay[N].objects[i].x, image[0].overlay[N].objects[i].y, &image[0].image[0].coords);
      x1 = image[0].overlay[N].objects[i].x + image[0].overlay[N].objects[i].dx;
      y1 = image[0].overlay[N].objects[i].y + image[0].overlay[N].objects[i].dy;
      XY_to_RD (&ra1, &dec1, x1, y1, &image[0].image[0].coords);
      dra = (ra1 - ra);
      ddec = (dec1 - dec);
    } else {
      XY_to_RD (&ra, &dec, image[0].overlay[N].objects[i].x, image[0].overlay[N].objects[i].y, &image[0].image[0].coords);
      x1 = image[0].overlay[N].objects[i].x;
      y1 = image[0].overlay[N].objects[i].y + image[0].overlay[N].objects[i].dy;
      XY_to_RD (&ra1, &dec1, x1, y1, &image[0].image[0].coords);
      ddec = fabs (dec1 - dec);
      x1 = image[0].overlay[N].objects[i].x + image[0].overlay[N].objects[i].dx;
      y1 = image[0].overlay[N].objects[i].y;
      XY_to_RD (&ra1, &dec1, x1, y1, &image[0].image[0].coords);
      dra = cos (dec*RAD_DEG) * fabs (ra1 - ra);
    }
    type = KiiOverlayTypeByNumber (image[0].overlay[N].objects[i].type);
    if (type == NULL) continue;
    fprintf (f, "%s %lf %lf %lf %lf\n", type, ra, dec, dra, ddec);
   }
  fclose (f);
  return (TRUE);
}

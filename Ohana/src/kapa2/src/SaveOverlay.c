# include "Ximage.h"

int SaveOverlay (int sock) {

  int i, N;
  char filename[256], *type;
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
    type = KiiOverlayTypeByNumber (image[0].overlay[N].objects[i].type);
    if (type == NULL) continue;
    fprintf (f, "%s %lf %lf %lf %lf\n", 
	     type,
	     image[0].overlay[N].objects[i].x,
	     image[0].overlay[N].objects[i].y,
	     image[0].overlay[N].objects[i].dx,
	     image[0].overlay[N].objects[i].dy);
  }
  fclose (f);
  return (TRUE);
}

/* this is asymmetric with LoadOverlay.c.  In that case, the client reads the file and sends
 * the overlay objects to kapa.  In this case, kapa writes the file directly... 
 */

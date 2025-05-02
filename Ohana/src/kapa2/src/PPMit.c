# include "Ximage.h"

int PPMit (int sock) {

  FILE *f;
  char *line, filename[1024];
  int dx, dy, i, j, Npalette, color;
  png_color *palette;
  bDrawBuffer *buffer;
  Graphic *graphic;

  graphic = GetGraphic();

  /* expect a line telling the number of bytes and a filename */
  KiiScanMessage (sock, "%s", filename);

  f = fopen (filename, "w");
  if (f == (FILE *) NULL) {
    fprintf (stderr, "can't open output file %s\n", filename);
    return (TRUE);  /* true because otherwise it quits kapa! */
  }
  
  palette = KapaPNGPalette (&Npalette);

  dx = graphic->dx;
  dy = graphic->dy;
  
  fprintf (f, "P6\n");
  fprintf (f, "%d %d\n", dx, dy);
  fprintf (f, "255\n");

  buffer = bDrawIt (palette, Npalette, 1);

  ALLOCATE (line, char, 3*dx);

  for (i = 0; i < dy; i++) {
    for (j = 0; j < dx; j++) {
      color = buffer[0].pixels[i][j];
      line[3*j + 0] = palette[color].red;
      line[3*j + 1] = palette[color].green;
      line[3*j + 2] = palette[color].blue;
    }
    fwrite (line, 3, dx, f);
  }
  fclose (f);
  
  bDrawBufferFree (buffer);
  free (palette);

  return (TRUE);
}

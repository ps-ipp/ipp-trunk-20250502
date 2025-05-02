# include "mosastro.h"

void output (char *ext, char *phu) {

  int i, N;
  char *p, outname[256];
  FILE *f;
  Header *header;
  
  for (i = 0; i < Nchip; i++) {
    
    /* convert chip[i].file to output name */
    
    p = strrchr (chip[i].file, '.');
    N = p - chip[i].file + 1;
    if (p == NULL) N = strlen (chip[i].file);

    bzero (outname, 256);
    strncpy_nowarn (outname, chip[i].file, N);

    strcat (outname, ext);

    /* convert chip from PLY to WRP (linked to field model) */
    strcpy (chip[i].map.ctype, "DEC--WRP");
    PutCoords (&chip[i].map, &chip[i].header);
    wchip (outname, &chip[i]);
  }

  field_combine ();

  f = fopen (phu, "w");
  if (f == (FILE *) NULL) {
    fprintf (stderr, "ERROR: can't create output file %s\n", phu);
    exit (1);
  }

  header = mkmosaic (1, 1, 0, &field.project);
  if (SAVE_RESID) {
    SaveResiduals (f, header);
    return;
  }

  fwrite (header[0].buffer, 1, header[0].datasize, f);
  fclose (f);
}

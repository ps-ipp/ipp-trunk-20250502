# include <ohana.h>
# include <gfitsio.h>

/*********************** fits read matrix ***********************************/
int gfits_read_matrix (char *filename, Matrix *matrix) {

  FILE *f;
  Header header;
  int status;

  status = gfits_read_header (filename, &header);
  if (!status) {
    fprintf (stderr, "error reading header of FITS file %s\n", filename);
    return (FALSE);
  }

  f = fopen (filename, "r");
  if (f == NULL) return (FALSE);

  fseeko (f, header.datasize, 0);
  
  status = gfits_load_matrix (f, matrix, &header);

  fclose (f);

  return (status);

}


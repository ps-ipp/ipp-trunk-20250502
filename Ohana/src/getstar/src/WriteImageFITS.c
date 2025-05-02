# include "dvoImageExtract.h"

/* given image, find catalog images which overlap it.  this function actually creates an
 * image-less file, filling in the header, but not the pixels.
 */
int WriteImageFITS (FILE *f, Image *image) {
  
  int Nstars;
  Header header;
  Header theader;
  FTable table;

  gfits_init_header (&header);
  header.extend = TRUE;
  header.Naxes = 2;
  if (image) {
    header.Naxis[0] = image[0].NX;
    header.Naxis[1] = image[0].NY;
  }
  gfits_create_header (&header);
  gfits_modify (&header, "NAXIS", "%d", 1, 0);

  if (image) {
    PutCoords (&image[0].coords, &header);
  }
  /* do not create data matrix - the matrix is defined to be empty (NAXIS=0)
     gfits_create_matrix (&header, &matrix);
  */
  gfits_fwrite_header  (f, &header);

  // gfits_fwrite_matrix  (f, &matrix);
  return (TRUE);

  Nstars = 0;
  if (image) Nstars = image[0].nstar;

  table.header = &theader;
  gfits_table_set_SMPData (&table, NULL, Nstars, TRUE);
  gfits_fwrite_Theader (f, &theader);
  gfits_fwrite_table   (f, &table);

  return TRUE;
}

// XXX this is a temporary hack to get skycell output working

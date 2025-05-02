# include "mosastro.h"

void wfits (char *filename, SMPData *stars, int Nstars, Header *header) {

  Matrix matrix;
  Header theader;
  FTable table;

  header[0].extend = TRUE;
  header[0].Naxes = 0;
  gfits_modify (header, "NAXIS",   "%d", 1, 0);
  gfits_modify_alt (header, "EXTEND",  "%t", 1, TRUE);
  gfits_modify (header, "NEXTEND", "%d", 1, 1);

  /* add in some keywords to specify the datatype & software version? */

  /* create (empty) data matrix */
  gfits_create_matrix (header, &matrix);
    
  table.header = &theader;
  gfits_table_set_SMPData (&table, stars, Nstars, TRUE);

  gfits_write_header  (filename, header);
  gfits_write_matrix  (filename, &matrix);
  gfits_write_Theader (filename, &theader);
  gfits_write_table   (filename, &table);
  return;
}


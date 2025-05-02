# include <ohana.h>
# include <gfitsio.h>
# include <stdio.h>
# define TRUE 1

main () {

  char filename[80];
  int i, j, Nrow;
  Header header, theader1, theader2;
  Matrix matrix;
  FTable table1, table2;
  float *zpobs, *zpref;
  unsigned long int *time;
  char *tchar;
  
  table1.header = &theader1;
  table2.header = &theader2;

  strcpy (filename, "test.fits");
  gfits_read_header  (filename, &header);
  gfits_read_matrix  (filename, &matrix);
  gfits_read_ftable  (filename, &table1, "ZERO_POINTS");
  gfits_read_ftable  (filename, &table2, "ASCII_PTS");
  
  /* set table column based on array, extend NAXIS2 as needed/appropriate */
  gfits_get_bintable_column (&theader1, &table1, "ZP_OBS", &zpobs);
  gfits_get_bintable_column (&theader1, &table1, "ZP_REF", &zpref);
  gfits_get_bintable_column (&theader1, &table1, "TIME",   &time);
  gfits_get_bintable_column (&theader1, &table1, "TCHAR",  &tchar);
  gfits_scan (&theader1, "NAXIS2", "%d", 1, &Nrow);

  for (i = 0; i < Nrow; i++) {
    fprintf (stderr, "%d %f %f %d   ", i, zpobs[i], zpref[i], time[i]);
    for (j = 0; j < 8; j++) { fprintf (stderr, "%c", tchar[i*8 + j]); }
    fprintf (stderr, "\n");
  }

  /* set table column based on array, extend NAXIS2 as needed/appropriate */
  gfits_get_table_column (&theader2, &table2, "ZP_OBS", &zpobs);
  gfits_get_table_column (&theader2, &table2, "ZP_REF", &zpref);
  gfits_get_table_column (&theader2, &table2, "TIME",   &time);
  gfits_get_table_column (&theader2, &table2, "TCHAR",  &tchar);
  gfits_scan (&theader2, "NAXIS2", "%d", 1, &Nrow);

  for (i = 0; i < Nrow; i++) {
    fprintf (stderr, "%d %f %f %d   ", i, zpobs[i], zpref[i], time[i]);
    for (j = 0; j < 8; j++) { fprintf (stderr, "%c", tchar[i*8 + j]); }
    fprintf (stderr, "\n");
  }


}      

/*
  gfits_set_table_column (&theader2, &table2, "ZP_OBS", zpobs, Nrow);
  gfits_set_table_column (&theader2, &table2, "ZP_REF", zpref, Nrow);
  gfits_set_table_column (&theader2, &table2, "TIME",   time,  Nrow);
  gfits_set_table_column (&theader2, &table2, "TCHAR",  tchar, Nrow);
*/

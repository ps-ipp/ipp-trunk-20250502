# include <ohana.h>
# include <gfitsio.h>
# define TRUE 1

main () {

  char filename[80];
  int i, Nrow;
  Header header, theader1, theader2;
  Matrix matrix;
  FTable table1, table2;
  float zpobs[1000];
  float zpref[1000];
  unsigned long int time[1000];
  char tchar[1000];
  
  strcpy (filename, "test.fits");
  Nrow = 20;
  for (i = 0; i < Nrow; i++) {
    zpobs[i] = i + 5;
    zpref[i] = i + 15;
    time[i] = i + 300000;
    sprintf (&tchar[8*i], "%8d", i);
  }

  /* create primary header */
  gfits_init_header (&header);    header.extend = TRUE;
  gfits_create_header (&header);
  gfits_create_matrix (&header, &matrix);
  gfits_print (&header, "NEXTEND", "%d", 1, 2);

  /* define bintable header & layout */
  gfits_create_table_header (&theader1, "BINTABLE", "ZERO_POINTS");

  gfits_define_bintable_column (&theader1, "E", "ZP_OBS", "measured zero point", "mag", 1.0, 0.0);
  gfits_define_bintable_column (&theader1, "E", "ZP_REF", "measured zero point", "mag", 1.0, 0.0);
  gfits_define_bintable_column (&theader1, "J", "TIME",   "time of data", "seconds since Jan 1, 1970 UT", 1.0, 0.0);
  gfits_define_bintable_column (&theader1, "8A", "TCHAR",   "time of data", "seconds since Jan 1, 1970 UT", 1.0, 0.0);

  /* create table, add data values */
  gfits_create_table (&theader1, &table1);

  /* set table column based on array, extend NAXIS2 as needed/appropriate */
  gfits_set_bintable_column (&theader1, &table1, "ZP_OBS", zpobs, Nrow);
  gfits_set_bintable_column (&theader1, &table1, "ZP_REF", zpref, Nrow);
  gfits_set_bintable_column (&theader1, &table1, "TIME",   time,  Nrow);
  gfits_set_bintable_column (&theader1, &table1, "TCHAR",  tchar,  Nrow);

  /* define ASCII table header */
  gfits_create_table_header (&theader2, "TABLE", "ASCII_PTS");

  gfits_define_table_column (&theader2, "F5.2", "ZP_OBS", "measured zero point", "mag");
  gfits_define_table_column (&theader2, "F5.2", "ZP_REF", "measured zero point", "mag");
  gfits_define_table_column (&theader2, "I8", "TIME",    "time of data", "seconds");
  gfits_define_table_column (&theader2, "A8", "TCHAR",   "time of data", "YYYYMMDD");

  /* create table, add data values */
  gfits_create_table (&theader2, &table2);

  gfits_set_table_column (&theader2, &table2, "ZP_OBS", zpobs, Nrow);
  gfits_set_table_column (&theader2, &table2, "ZP_REF", zpref, Nrow);
  gfits_set_table_column (&theader2, &table2, "TIME",   time,  Nrow);
  gfits_set_table_column (&theader2, &table2, "TCHAR",  tchar, Nrow);

  /* write header, matrix, table1, table2 */
  gfits_write_header  (filename, &header);
  gfits_write_matrix  (filename, &matrix);
  gfits_write_Theader (filename, &theader1);
  gfits_write_table   (filename, &table1);
  gfits_write_Theader (filename, &theader2);
  gfits_write_table   (filename, &table2);

}

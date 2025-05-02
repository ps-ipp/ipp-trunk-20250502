# include <ohana.h>
# include <gfitsio.h>

/*********************** fits init table *******************************/
int gfits_init_table (FTable *table) {
  table[0].header     = NULL;
  table[0].buffer     = NULL;
  table[0].datasize   = 0;
  table[0].heap_start = 0;
  table[0].validsize  = 0;
  return (TRUE);

}

/*********************** fits init table *******************************/
int gfits_init_vtable (VTable *table) {

  table[0].header   = NULL;
  table[0].buffer   = NULL;
  table[0].row      = NULL;
  table[0].Nrow     = 0;
  table[0].datasize = 0;
  table[0].pad      = 0;
  return (TRUE);
}

/*********************** fits create table *******************************/
int gfits_create_table (Header *header, FTable *table) {

  off_t Nbytes;
  char type[80];

  gfits_scan (header, "XTENSION", "%s", 1, type);

  table[0].header = header;
  
  Nbytes = gfits_data_size (header);
  ALLOCATE (table[0].buffer, char, MAX (Nbytes, 1));
  if (!strcmp (type, "TABLE")) {
    memset (table[0].buffer, ' ', Nbytes);
  } else {
    memset (table[0].buffer, 0, Nbytes);
  }
  table[0].datasize = Nbytes;
  table[0].heap_start = gfits_heap_start (header);
  return (TRUE);

}

/*********************** fits create table *******************************/
int gfits_set_table_rows (Header *header, FTable *table, off_t Nrows) {

    header[0].Naxis[1] = Nrows;
    gfits_modify (header, "NAXIS2", OFF_T_FMT, 1,  Nrows);

    off_t Nbytes = gfits_data_size (header);
    REALLOCATE (table[0].buffer, char, Nbytes);
    bzero (table[0].buffer, Nbytes);
    table[0].datasize = Nbytes;
    table[0].heap_start = gfits_heap_start (header);
  return (TRUE);
}

/* init table structure, allocate space for data array (min 1 byte for 0 length) */

/*********************** fits create table *******************************/
int gfits_create_vtable (Header *header, VTable *table, int Nrow) {

  off_t i, Nx, Ny;
  char type[80];

  gfits_scan (header, "XTENSION", "%s", 1, type);

  table[0].header = header;
  Nx = table[0].header[0].Naxis[0];
  Ny = table[0].header[0].Naxis[0];
  table[0].datasize = gfits_data_size (header);
  // table[0].heap_start = gfits_heap_start (header);
  table[0].pad = table[0].datasize - Nx*Ny;
 
  table[0].Nrow = Nrow;
  ALLOCATE (table[0].buffer, char *, MAX (Nrow, 1));
  ALLOCATE (table[0].row, off_t, MAX (Nrow, 1));
  for (i = 0; i < Nrow; i++) {
    ALLOCATE (table[0].buffer[i], char, Nx);
  }
  return (TRUE);
}

/* init table structure, allocate space for data array (min 1 byte for 0 length) */

# include <ohana.h>
# include <gfitsio.h>

/*********************** fits table column ****************************/
int gfits_add_rows (FTable *table, char *data, off_t Nrow, off_t Nbytes) {

  off_t Nx, Ny;
  off_t nbytes, Nstart;
  Header *header;

  header = table[0].header;

  gfits_scan (header, "NAXIS1", OFF_T_FMT, 1,  &Nx);
  gfits_scan (header, "NAXIS2", OFF_T_FMT, 1,  &Ny);
  
  if (header[0].Naxis[1] != Ny) return (FALSE);
  if (header[0].Naxis[0] != Nx) return (FALSE);
  if (header[0].Naxis[0] != Nbytes) return (FALSE);
  
  Nstart = Nx*Ny;

  /* update header y dimension */
  Ny += Nrow;
  header[0].Naxis[1] = Ny;
  gfits_modify (header, "NAXIS2",  OFF_T_FMT, 1,  Ny);

  nbytes = gfits_data_size (header);
  REALLOCATE (table[0].buffer, char, MAX (nbytes, 1));
  table[0].datasize = nbytes;
  table[0].heap_start = gfits_heap_start (header);
  
  memcpy (&table[0].buffer[Nstart], data, Nbytes*Nrow);
  myMemset (&table[0].buffer[Nx*Ny], ' ', nbytes - Nx*Ny);
  return (TRUE);
}

/*********************** fits add (real) rows to virtual table ****************************/
int gfits_vadd_rows (VTable *table, char *data, off_t Nrow, off_t Nbytes) {

  off_t i, Nx, Ny;
  off_t Nstart;
  Header *header;

  header = table[0].header;

  gfits_scan (header, "NAXIS1", OFF_T_FMT, 1,  &Nx);
  gfits_scan (header, "NAXIS2", OFF_T_FMT, 1,  &Ny);
  
  if (header[0].Naxis[1] != Ny) return (FALSE);
  if (header[0].Naxis[0] != Nx) return (FALSE);
  if (header[0].Naxis[0] != Nbytes) return (FALSE);
  
  /* Nstart is end of data in memory */
  Nstart = table[0].Nrow;
  table[0].Nrow += Nrow;
  REALLOCATE (table[0].buffer, char *, table[0].Nrow);
  REALLOCATE (table[0].row, off_t, table[0].Nrow);
  for (i = 0; i < Nrow; i++) {
    ALLOCATE (table[0].buffer[Nstart+i], char, MAX (1, Nx));
    memcpy (table[0].buffer[Nstart+i], &data[i*Nx], Nx);
    table[0].row[Nstart+i] = Ny + i;
  }

  /* update header y dimension */
  Ny += Nrow;
  header[0].Naxis[1] = Ny;
  gfits_modify (header, "NAXIS2", OFF_T_FMT, 1,  Ny);

  table[0].datasize = gfits_data_size (table[0].header);
  // table[0].heap_start = gfits_heap_start (header);
  table[0].pad      = table[0].datasize - Nx*Ny;

  return (TRUE);
}

/*********************** fits table column ****************************/
int gfits_delete_rows (FTable *table, off_t Nstart, off_t Nrow) {

  off_t Nx, Ny, N0, N1, N2;
  off_t nbytes;
  Header *header;

  header = table[0].header;

  gfits_scan (header, "NAXIS1", OFF_T_FMT, 1,  &Nx);
  gfits_scan (header, "NAXIS2", OFF_T_FMT, 1,  &Ny);
  
  if (header[0].Naxis[1] != Ny) return (FALSE);
  if (header[0].Naxis[0] != Nx) return (FALSE);
  if (Ny > Nstart + Nrow) return (FALSE);
  
  /* shrink buffer by Nrow entries */
  N0 = Nx*Nstart;
  N1 = Nx*(Nstart + Nrow);
  N2 = Nx*(Ny - Nstart - Nrow);
  memmove (&table[0].buffer[N0], &table[0].buffer[N1], N2);

  /* update header y dimension */
  Ny -= Nrow;
  header[0].Naxis[1] = Ny;
  gfits_modify (header, "NAXIS2", OFF_T_FMT, 1,  Ny);

  nbytes = gfits_data_size (header);
  REALLOCATE (table[0].buffer, char, MAX (nbytes, 1));
  table[0].datasize = nbytes;
  table[0].heap_start = gfits_heap_start (header);
  return (TRUE);
}


# include <ohana.h>
# include <gfitsio.h>

/*********************** fits write header *********************************/
int gfits_write_table (char *filename, FTable *table) {
  
  FILE *f = fopen (filename, "a+");
  if (f == (FILE *) NULL) return (FALSE);
  
  int status = fseeko (f, 0LL, SEEK_END);  /* write table to end of file! */
  if (status) { perror ("fseeko: "); return (FALSE);  }
  status = gfits_fwrite_table (f, table);

  fclose (f);
  return (status);
}	

/*********************** fits write table *********************************/
int gfits_fwrite_table (FILE *f, FTable *table) {
  
  off_t Nbytes;

# ifdef OHANA_MEMORY  
  { // check memory before writing:
    // memblock of supplied pointer
    OhanaMemblock *myBlock = (OhanaMemblock *) table[0].buffer - 1;
    myAssert (myBlock->startblock == OHANA_MEMMAGIC, "bad memory");
    myAssert (myBlock->endblock == OHANA_MEMMAGIC, "bad memory");
    myAssert (myBlock->size >= (size_t) table[0].datasize, "overflow");
  }
# endif

  Nbytes = fwrite (table[0].buffer, sizeof(char), table[0].datasize, f);
  if (Nbytes != table[0].datasize) { 
    perror ("fwrite: "); 
    return (FALSE);  
  }
  return (TRUE);
}	

/*********************** fits write virtual table *********************************/
int gfits_fwrite_vtable (FILE *f, VTable *table) {
  
  off_t i, Nx, Ny, Npad, offset, start;
  off_t Nbytes, *row, Nrow;
  char *pad;

  Nrow = table[0].Nrow;
  row = table[0].row;
  gfits_scan (table[0].header, "NAXIS1", OFF_T_FMT, 1,  &Nx);
  gfits_scan (table[0].header, "NAXIS2", OFF_T_FMT, 1,  &Ny);

  /* file pointer is at beginning of desired table data */
  start = ftello (f);
  if (start < 0) { perror ("ftello: "); return FALSE; }
  
  for (i = 0; i < Nrow; i++) {
    offset = start + Nx*row[i];
    fseeko (f, offset, SEEK_SET);
    Nbytes = fwrite (table[0].buffer[i], sizeof (char), Nx, f);
    if (Nbytes != Nx) { perror ("fwrite: "); return (FALSE); }
  }
  
  Npad = table[0].datasize - Nx*Ny;
  ALLOCATE (pad, char, Npad);
  bzero (pad, Npad);

  offset = start + Nx*Ny;
  int status = fseeko (f, offset, SEEK_SET);
  if (status) { perror ("fseeko: "); return FALSE; }

  Nbytes = fwrite (pad, sizeof (char), Npad, f);
  if (Nbytes != Npad) { perror ("fwrite: "); return (FALSE); }
  free (pad);

  return (TRUE);
}	


/* this will add data beyond the end of the table in the file if needed,
   filling intervening gap with 0 */

/*********************** fits read ftable data ***********************************/
int gfits_fwrite_ftable_range (FILE *f, FTable *ftable, off_t start, off_t Nrows, off_t Ndisk, off_t Ntotal) {

  off_t Nbytes, Nwrite, Nskip, Nx, Npad;
  char *pad;

  if (start < 0) return (FALSE);
  
  /* modify vtable to represent full disk table */
  gfits_modify (ftable[0].header, "NAXIS2", OFF_T_FMT, 1,  Ntotal);
  ftable[0].header[0].Naxis[1] = Ntotal;

  Nx = ftable[0].header[0].Naxis[0]; // final output table size on disk 
  ftable[0].datasize = gfits_data_size (ftable[0].header);
  ftable[0].heap_start = gfits_heap_start (ftable[0].header);

  Nskip = start * Nx;
  Nbytes = Nrows * Nx;

  // cursor must be at start of the table header
  if (!gfits_fwrite_Theader (f, ftable[0].header)) {
    fprintf (stderr, "can't write table header");
    return (FALSE);
  }

  // cursor must be at start of the table (after table header)
  if (fseeko (f, Nskip, SEEK_CUR)) {
    perror ("fseeko: ");
    fprintf (stderr, "can't seek table start\n");
    return (FALSE);
  }

  Nwrite = fwrite (ftable[0].buffer, sizeof (char), Nbytes, f);
  if (Nwrite != Nbytes) { perror ("fwrite: "); return (FALSE); }

  if (Ntotal >= Ndisk) {
    Npad = ftable[0].datasize - Nx*Ntotal;
    ALLOCATE (pad, char, Npad);
    bzero (pad, Npad);
    Nbytes = fwrite (pad, sizeof (char), Npad, f);
    free (pad);

    if (Nbytes != Npad) { perror ("fwrite: "); return (FALSE);  }
  }

  return (TRUE);
}	


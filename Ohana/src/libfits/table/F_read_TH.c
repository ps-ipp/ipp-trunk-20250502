# include <ohana.h>
# include <gfitsio.h>

/*********************** fits read Theader ***********************************/
int gfits_read_Theader (char *filename, Header *Theader) {
  
  FILE *f;
  Header header;
  int status;
  off_t Nbytes;
  
  status = gfits_read_header (filename, &header);
  if (!status) {
    fprintf (stderr, "error reading header of FITS file %s\n", filename);
    return (FALSE);
  }

  f = fopen (filename, "r");
  if (f == NULL) {
    Theader[0].buffer = NULL;
    gfits_free_header (&header);
    return (FALSE);
  }

  Nbytes = gfits_data_size (&header);
  fseeko (f, Nbytes, SEEK_SET);
  gfits_free_header (&header);

  status = gfits_load_Theader (f, Theader);
  fclose (f);
  return (status);
}	

/* load table from STREAM positioned at beginning of table header */
/*********************** fits load Theader ***********************************/
int gfits_load_Theader (FILE *f, Header *Theader) {
  
  char *p;
  int i, done, status;
  off_t Nbytes;
  
  Theader[0].datasize = 0;
  done = FALSE;
  ALLOCATE (Theader[0].buffer, char, 1);

  for (i = 0; !done; i++) {
    REALLOCATE (Theader[0].buffer, char, (i + 1)*FT_RECORD_SIZE);
    Nbytes = fread (&Theader[0].buffer[i*FT_RECORD_SIZE], sizeof(char), FT_RECORD_SIZE, f);
    if (Nbytes != FT_RECORD_SIZE) {
      perror ("fits matrix read error");
    }

    Theader[0].datasize += Nbytes;
    if (Nbytes != FT_RECORD_SIZE) {
      done = TRUE;
    }
    p = gfits_header_field (Theader, "END", 1);
    if (p != NULL)
      done = TRUE;
  }

  Theader[0].bscale = 0;
  Theader[0].bzero  = 0;
  for (i = 0; i < FT_MAX_NAXES; i++)
    Theader[0].Naxis[i] = 0;

  status = TRUE;
  status &= gfits_scan (Theader,  "BITPIX", "%d", 1, &Theader[0].bitpix);
  status &= gfits_scan (Theader,  "NAXIS",  "%d", 1, &Theader[0].Naxes);
  if (!status) return (FALSE);
				                           
  gfits_scan (Theader,  "NAXIS1", OFF_T_FMT, 1,  &Theader[0].Naxis[0]);
  gfits_scan (Theader,  "NAXIS2", OFF_T_FMT, 1,  &Theader[0].Naxis[1]);
  gfits_scan (Theader,  "NAXIS3", OFF_T_FMT, 1,  &Theader[0].Naxis[2]);
  gfits_scan (Theader,  "NAXIS4", OFF_T_FMT, 1,  &Theader[0].Naxis[3]);
  gfits_scan (Theader,  "NAXIS5", OFF_T_FMT, 1,  &Theader[0].Naxis[4]);
  gfits_scan (Theader,  "NAXIS6", OFF_T_FMT, 1,  &Theader[0].Naxis[5]);
  gfits_scan (Theader,  "NAXIS7", OFF_T_FMT, 1,  &Theader[0].Naxis[6]);
  gfits_scan (Theader,  "NAXIS8", OFF_T_FMT, 1,  &Theader[0].Naxis[7]);
  gfits_scan (Theader,  "NAXIS9", OFF_T_FMT, 1,  &Theader[0].Naxis[8]);
  gfits_scan (Theader, "NAXIS10", OFF_T_FMT, 1,  &Theader[0].Naxis[9]);

  return (TRUE);
}	

/*********************** fits fread Theader ***********************************/
int gfits_fread_Theader (FILE *f, Header *Theader) {
  
  int status;
  
  status = gfits_load_Theader (f, Theader);
  return (status);
}

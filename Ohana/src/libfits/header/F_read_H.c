# include <ohana.h>
# include <gfitsio.h>

/*********************** fits read header ***********************************/
int gfits_read_header (char *filename, Header *header) {
  
  FILE *f;
  
  f = fopen (filename, "r");
  if (f == NULL) {
    header[0].buffer = NULL;
    return (FALSE);
  }
  if (!gfits_load_header (f, header)) {
    fclose (f);
    return (FALSE);
  }

  fclose (f);
  return (TRUE);
}	

int gfits_load_header (FILE *f, Header *header) {

  off_t i, Nbytes;
  int done, t1, t2;
  char *p;

  header[0].datasize = 0;
  done = FALSE;
  ALLOCATE (header[0].buffer, char, FT_RECORD_SIZE);

  for (i = 0; !done; i++) {
    REALLOCATE (header[0].buffer, char, (i + 1)*FT_RECORD_SIZE);
    Nbytes = fread (&header[0].buffer[i*FT_RECORD_SIZE], sizeof(char), FT_RECORD_SIZE, f);
    if (Nbytes != FT_RECORD_SIZE) {
      if (feof(f)) return (FALSE);
      perror ("gfits_load_header : failed to read all data");
      return (FALSE);
    }
    
    header[0].datasize += Nbytes;

    if (i == 0) { 
      /* on first block, verify it is a FITS table: SIMPLE .. or XTENSION ... */
      t1 = strncmp (header[0].buffer, "SIMPLE", 6);
      t2 = strncmp (header[0].buffer, "XTENSION", 8);
      if (t1 && t2) return (FALSE);
    }

    p = gfits_header_field (header, "END", 1);
    if (p != NULL)
      done = TRUE;
  }

  /* if these are not found in the header, they should be set to FALSE */
  header[0].simple = FALSE;
  header[0].extend = FALSE;

  header[0].unsign = gfits_get_unsign_mode();
  header[0].bscale = 1;
  header[0].bzero  = 0;

  for (i = 0; i < FT_MAX_NAXES; i++)
    header[0].Naxis[i] = 0;

  gfits_scan_alt (header,  "SIMPLE", "%t",   1, &header[0].simple);
  gfits_scan (header,  "BITPIX", "%d",   1, &header[0].bitpix);
  gfits_scan (header,  "NAXIS",  "%d",   1, &header[0].Naxes);
				                           
  gfits_scan_alt (header,  "EXTEND", "%t",   1, &header[0].extend);
  gfits_scan_alt (header,  "UNSIGN", "%t",   1, &header[0].unsign);
  gfits_scan (header,  "BSCALE", "%lf",  1, &header[0].bscale);
  gfits_scan (header,  "BZERO",  "%lf",  1, &header[0].bzero);
				       
  gfits_scan (header,  "NAXIS1", OFF_T_FMT, 1,  &header[0].Naxis[0]);
  gfits_scan (header,  "NAXIS2", OFF_T_FMT, 1,  &header[0].Naxis[1]);
  gfits_scan (header,  "NAXIS3", OFF_T_FMT, 1,  &header[0].Naxis[2]);
  gfits_scan (header,  "NAXIS4", OFF_T_FMT, 1,  &header[0].Naxis[3]);
  gfits_scan (header,  "NAXIS5", OFF_T_FMT, 1,  &header[0].Naxis[4]);
  gfits_scan (header,  "NAXIS6", OFF_T_FMT, 1,  &header[0].Naxis[5]);
  gfits_scan (header,  "NAXIS7", OFF_T_FMT, 1,  &header[0].Naxis[6]);
  gfits_scan (header,  "NAXIS8", OFF_T_FMT, 1,  &header[0].Naxis[7]);
  gfits_scan (header,  "NAXIS9", OFF_T_FMT, 1,  &header[0].Naxis[8]);
  gfits_scan (header, "NAXIS10", OFF_T_FMT, 1,  &header[0].Naxis[9]);

  if (!gfits_scan (header, "PCOUNT",  OFF_T_FMT, 1, &header[0].pcount)) {
    header[0].pcount = 0;
  }
  if (!gfits_scan (header, "GCOUNT",  "%d", 1, &header[0].gcount)) {
    header[0].gcount = 1;
  }

  return (TRUE);

}

int gfits_fread_header (FILE *f, Header *header) {

  int status;
  
  status = gfits_load_header (f, header);
  return (status);
}

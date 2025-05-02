# include <ohana.h>
# include <gfitsio.h>

/*********************** fits read matrix ***********************************/
int gfits_read_matrix_segment (char *filename, Matrix *matrix, char *region) {

  FILE *f;
  Header header;
  int status;

  f = fopen (filename, "r");
  if (f == NULL) {
    return (FALSE);
  }

  status = gfits_fread_header (f, &header);
  if (!status) {
    fclose (f);
    fprintf (stderr, "error reading header of FITS file %s\n", filename);
    return (FALSE);
  }

  status = gfits_fread_matrix_segment (f, matrix, &header, region);
  if (!status) {
    fclose (f);
    fprintf (stderr, "error reading header of FITS file %s\n", filename);
    return (FALSE);
  }

  fclose (f);
  return (TRUE);
}

// read header before calling this function
// file pointer should be at start of Matrix data segment
int gfits_fread_matrix_segment (FILE *f, Matrix *matrix, Header *header, char *region) {

  int status;
  off_t i, nbytes, Nbytes, Nskip, NbytesData;
  int wantaxis[FT_MAX_NAXES][2];
  double tmp;

  for (i = 0; i < FT_MAX_NAXES; i++) {
    wantaxis[i][0] = wantaxis[i][1] = 0;
  }

  for (i = 0; i < header[0].Naxes; i++) {
    status = dparse (&tmp, 2*i + 1, region);
    if (!status || (tmp < 0))
      wantaxis[i][0] = 0;
    else 
      wantaxis[i][0] = (tmp > header[0].Naxis[i] - 1) ? header[0].Naxis[i] - 1 : tmp;
    status = dparse (&tmp, 2*i + 2, region);
    if (!status || (tmp < 0))
      wantaxis[i][1] = header[0].Naxis[i];
    else 
      wantaxis[i][1] = (tmp > header[0].Naxis[i]) ? header[0].Naxis[i] : tmp;
  }

  matrix[0].bitpix = header[0].bitpix;
  matrix[0].unsign = header[0].unsign;
  matrix[0].bscale = header[0].bscale;
  matrix[0].bzero  = header[0].bzero;
  matrix[0].Naxes  = header[0].Naxes;
  for (i = 0; i < FT_MAX_NAXES; i++)
    matrix[0].Naxis[i] = wantaxis[i][1] - wantaxis[i][0];

  matrix[0].datasize = abs (matrix[0].bitpix) / 8;
  for (i = 0; i < matrix[0].Naxes; i++)
    matrix[0].datasize *= matrix[0].Naxis[i];

  Nskip = wantaxis[2][0]*matrix[0].Naxis[0]*matrix[0].Naxis[1]*abs(matrix[0].bitpix)/8;
  NbytesData = Nskip + matrix[0].datasize;

  if (NbytesData % FT_RECORD_SIZE) 
    Nbytes = FT_RECORD_SIZE * ((off_t) (NbytesData / FT_RECORD_SIZE) + 1) - Nskip;
  else 
    Nbytes = FT_RECORD_SIZE * ((off_t) (NbytesData / FT_RECORD_SIZE)) - Nskip;

  ALLOCATE (matrix[0].buffer, char, MAX (Nbytes, 1));

# ifndef SEEK_CUR 
# define SEEK_SET 0   
# define SEEK_CUR 1   
# define SEEK_END 2
# endif

  /* currently only good for reading in full planes in 3-D */
  fseeko (f, Nskip, SEEK_CUR);
  nbytes = fread (matrix[0].buffer, sizeof(char), Nbytes, f);
  if (nbytes != Nbytes) {
    perror ("fits matrix read error");
  }

  matrix[0].datasize = Nbytes;

# define DOSWAP(A,B) { char tmp = A; A = B; B = tmp; }

# ifdef BYTE_SWAP  
 {
  unsigned char *byte0, *byte1, *byte2, *byte3, *byte4, *byte5, *byte6, *byte7;
  int perpix;

  perpix = abs(header[0].bitpix) / 8;
  if (perpix > 1) {
    byte0 = (unsigned char *) matrix[0].buffer;
    byte1 = (unsigned char *) matrix[0].buffer + 1;
    byte2 = (unsigned char *) matrix[0].buffer + 2;
    byte3 = (unsigned char *) matrix[0].buffer + 3;
    byte4 = (unsigned char *) matrix[0].buffer + 4;
    byte5 = (unsigned char *) matrix[0].buffer + 5;
    byte6 = (unsigned char *) matrix[0].buffer + 6;
    byte7 = (unsigned char *) matrix[0].buffer + 7;
    if (perpix == 2) {
      for (i = 0; i < nbytes; i+=2, byte0 += 2, byte1 += 2) {
	DOSWAP(*byte0, *byte1);
      }
    }
    if (perpix == 4) {
      for (i = 0; i < nbytes; i+=4, byte0 += 4, byte1 += 4, byte2 += 4, byte3 += 4) {
	DOSWAP (*byte0, *byte3);
	DOSWAP (*byte1, *byte2);
      }
    }
    if (perpix == 8) {
      for (i = 0; i < nbytes; i+=8, byte0 += 8, byte1 += 8, byte2 += 8, byte3 += 8, byte4 += 8, byte5 += 8, byte6 += 8, byte7 += 8) {
	DOSWAP (*byte0, *byte7);
	DOSWAP (*byte1, *byte6);
	DOSWAP (*byte2, *byte5);
	DOSWAP (*byte3, *byte4);
      }
    }
  }
 }
# endif

  if (nbytes < Nbytes - 2880) {  /* this is a bad FITS error: image is not OK */
    fprintf (stderr, "error reading in matrix data from FITS file ("OFF_T_FMT" < "OFF_T_FMT" - 2880)\n",  nbytes,  Nbytes);
    return (FALSE);
  }
  if (nbytes != Nbytes) {  /* this is a FITS error, but often the image is OK */
    fprintf (stderr, "incomplete block in FITS file: ("OFF_T_FMT", "OFF_T_FMT")\n",  nbytes,  Nbytes);
    return (TRUE); 
  }

  return (TRUE);

}

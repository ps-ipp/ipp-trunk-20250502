# include <ohana.h>
# include <gfitsio.h>

int gfits_fread_matrix (FILE *f, Matrix *matrix, Header *header) {
  
  int status;
  
  status = gfits_load_matrix (f, matrix, header);
  return (status);
}

/*********************** fits read matrix ***********************************/
int gfits_load_matrix (FILE *f, Matrix *matrix, Header *header) {

  
  off_t i, nbytes, Nbytes;

  matrix[0].bitpix = header[0].bitpix;
  matrix[0].unsign = header[0].unsign;
  matrix[0].bscale = header[0].bscale;
  matrix[0].bzero  = header[0].bzero;
  matrix[0].Naxes  = header[0].Naxes;
  for (i = 0; i < FT_MAX_NAXES; i++)
    matrix[0].Naxis[i] = header[0].Naxis[i];

  Nbytes = gfits_data_size (header);
  ALLOCATE (matrix[0].buffer, char, Nbytes);
  matrix[0].datasize = Nbytes;

  nbytes = fread (matrix[0].buffer, sizeof(char), Nbytes, f);
  if (nbytes != Nbytes) {
    perror ("FITS file is short in ##__func__");
    if (nbytes != gfits_data_min_size (header)) {
      fprintf (stderr, "error: fits read error in %s", __func__);
      return (FALSE);
    }
    fprintf (stderr, "warning: file missing pad\n");
  }

# define DOSWAP(A,B) { char tmp = A; A = B; B = tmp; }

# ifdef BYTE_SWAP  
 {
  int perpix;
  unsigned char *byte0, *byte1, *byte2, *byte3, *byte4, *byte5, *byte6, *byte7;

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
    fprintf (stderr, "error reading in matrix data from FITS file\n");
    return (FALSE);
  }
  if (nbytes != Nbytes) {  /* this is a FITS error, but often the image is OK */
    fprintf (stderr, "incomplete block in FITS file: ("OFF_T_FMT", "OFF_T_FMT")\n",  nbytes,  Nbytes);
    return (TRUE); 
  }

  return (TRUE);

}


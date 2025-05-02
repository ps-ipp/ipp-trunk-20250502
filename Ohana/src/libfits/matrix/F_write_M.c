# include <ohana.h>
# include <gfitsio.h>

/*********************** fits write matrix ***********************************/
int gfits_write_matrix (char *filename, Matrix *matrix) {

  FILE *f;
  int status;

  f = fopen (filename, "a");
  if (f == NULL) return (FALSE);
  
  fseeko (f, 0LL, SEEK_END);  /* write matrix to end of file! */

  status = gfits_fwrite_matrix (f, matrix);
  fclose (f);
  return (status);
}

/*********************** fits write matrix ***********************************/
int gfits_fwrite_matrix (FILE *f, Matrix *matrix) {

  int status;
  off_t nbytes, Nbytes;

  status = TRUE;

  /* this is a bit dangerous: we are not realloc'ing the matrix.
     if the size is wrong, it probably should simply pad the fwrite statement.
     better policy should be defined (ie, matrix.datasize always correct blocking?)
  */

  if (matrix[0].datasize % FT_RECORD_SIZE) 
    nbytes = FT_RECORD_SIZE * ((off_t) (matrix[0].datasize / FT_RECORD_SIZE) + 1);
  else 
    nbytes = FT_RECORD_SIZE * ((off_t) (matrix[0].datasize / FT_RECORD_SIZE));

  // fprintf (stderr, "write matrix: %d bytes (%d datasize)\n", (int) nbytes, (int) matrix[0].datasize);
  if (nbytes == 0) return TRUE;

# define DOSWAP(A,B) { char tmp = A; A = B; B = tmp; }

  /* this is a bit cumbersome: swap all words, write out file, then swap back... */
# ifdef BYTE_SWAP  
  { 
    off_t i;
    unsigned char *byte0, *byte1, *byte2, *byte3, *byte4, *byte5, *byte6, *byte7;
    int perpix;

    perpix = abs(matrix[0].bitpix) / 8;
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

  Nbytes = fwrite (matrix[0].buffer, sizeof(char), nbytes, f);
  if (Nbytes != nbytes) {
    status = FALSE;
  }

# ifdef BYTE_SWAP 
  {
    off_t i;
    unsigned char *byte0, *byte1, *byte2, *byte3, *byte4, *byte5, *byte6, *byte7;
    int perpix;

    perpix = abs(matrix[0].bitpix) / 8;
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

  return (status);

}


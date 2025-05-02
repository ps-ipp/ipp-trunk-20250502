# include <ohana.h>
# include <gfitsio.h>
# ifndef SEEK_CUR 
# define SEEK_SET 0   
# define SEEK_CUR 1   
# define SEEK_END 2
# endif

/*********************** fits read matrix ***********************************/
/** the result of this function is a matrix with dimension Npix * 1 **/
int gfits_read_portion (char *filename, Matrix *matrix, off_t Nskip, off_t Npix) {

  FILE *f;
  Header header;
  int status;
  off_t nbytes, Nbytes, Nrec;

  status = gfits_read_header (filename, &header);
  if (!status) {
    fprintf (stderr, "error reading header of FITS file %s\n", filename);
    return (FALSE);
  }

  matrix[0].bitpix = header.bitpix;
  matrix[0].unsign = header.unsign;
  matrix[0].bscale = header.bscale;
  matrix[0].bzero  = header.bzero;
  matrix[0].Naxes  = 2;
  matrix[0].Naxis[0] = Npix;
  matrix[0].Naxis[1] = 1;

  Nbytes = Npix  * abs (matrix[0].bitpix)/8;
  Nskip  = Nskip * abs (matrix[0].bitpix)/8;

  /* round up to next complete block */
  if (Nbytes % FT_RECORD_SIZE) {
    Nrec = 1 + (int) (Nbytes / FT_RECORD_SIZE);
    Nbytes = FT_RECORD_SIZE * Nrec;
  }
  matrix[0].datasize = Nbytes;
  ALLOCATE (matrix[0].buffer, char, Nbytes);

  /* open file, read data, close file */
  f = fopen (filename, "r");
  if (f == NULL) return (FALSE);
  fseeko (f, header.datasize + Nskip, SEEK_SET);
  nbytes = fread (matrix[0].buffer, sizeof(char), Nbytes, f);
  if (nbytes != Nbytes) {
    perror ("fits matrix read error");
  }

  fclose (f);

# define DOSWAP(A,B) { char tmp = A; A = B; B = tmp; }

# ifdef BYTE_SWAP  
  {
    off_t i;
    int perpix;
    unsigned char *byte0, *byte1, *byte2, *byte3, *byte4, *byte5, *byte6, *byte7;
    
    perpix = abs(header.bitpix) / 8;
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

  gfits_free_header (&header);

  if (nbytes < Nbytes - 2880) {  /* this is a bad FITS error: image is not OK */
    fprintf (stderr, "error reading in matrix data from FITS file %s ("OFF_T_FMT" < "OFF_T_FMT" - 2880)\n", filename,  nbytes,  Nbytes);
    return (FALSE);
  }
  if (nbytes != Nbytes) {  /* this is a FITS error, but often the image is OK */
    fprintf (stderr, "incomplete block in %s: ("OFF_T_FMT", "OFF_T_FMT")\n", filename,  nbytes,  Nbytes);
    return (TRUE); 
  }

  return (TRUE);

}


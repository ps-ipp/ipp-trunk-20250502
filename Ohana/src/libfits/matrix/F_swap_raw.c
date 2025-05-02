# include <ohana.h>
# include <gfitsio.h>
# define DOSWAP(A,B) { char tmp = A; A = B; B = tmp; }

/*********************** fits convert format ***********************************/
/* this function is safe in the number of axes (can even be 0) */
int gfits_swap_raw (Matrix *matrix) {

  off_t i;

  int bytes_per_pixel = (abs(matrix[0].bitpix) / 8);

  off_t Npixels = gfits_npix_matrix (matrix);

  char *byte = matrix[0].buffer;

  switch (bytes_per_pixel) {
    case 1:
      break;
    case 2:
      for (i = 0; i < Npixels; i++) {
	DOSWAP(byte[0], byte[1]);
	byte += 2;
      }
      break;
    case 4:
      for (i = 0; i < Npixels; i++) {
	DOSWAP(byte[0], byte[3]);
	DOSWAP(byte[1], byte[2]);
	byte += 4;
      }
      break;
    case 8:
      for (i = 0; i < Npixels; i++) {
	DOSWAP(byte[0], byte[7]);
	DOSWAP(byte[1], byte[6]);
	DOSWAP(byte[2], byte[5]);
	DOSWAP(byte[3], byte[4]);
	byte += 8;
      }
      break;
    default:
      myAbort ("invalid bytes_per_pixel");
  }
  return TRUE;
}

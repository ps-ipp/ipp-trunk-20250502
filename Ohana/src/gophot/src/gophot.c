# include "gophot.h"

int main (int argc, char **argv) {

  float tmp;

  if (argc < 4) {
    fprintf (stderr, "ERROR: usage: dophot imagename outfile paramfile\n");
    exit (2);
  }
  ConfigInit (&argc, argv);

  fprintf (stderr, "reading from %s, writing to %s, %s param file\n", argv[1], argv[2], argv[3]);

  strcpy (files[4], argv[2]);

  /* load image header and data */
  if (!gfits_read_header (argv[1], &header)) {
    fprintf (stderr, "ERROR: can't open FITS header %s\n", argv[1]);
    exit (1);
  }
  if (!gfits_read_matrix (argv[1], &matrix)) {
    fprintf (stderr, "ERROR: can't open FITS matrix %s\n", argv[1]);
    exit (1);
  }
  /* convert to float, set up noise array and axes */
  gfits_convert_format (&header, &matrix, -32, 1.0, 0.0, 0xffff, 0);
  ALLOCATE (noise, float, matrix.datasize);
  big = (float *) matrix.buffer;
  nfast = matrix.Naxis[0];
  nslow = matrix.Naxis[1];
 
  /* override config values with header values */ 
  if (gfits_scan (&header, "SATVALUE", "%f", 1, &tmp)) itop = tmp;
  if (gfits_scan (&header, "GAIN", "%f", 1, &tmp)) eperdn = tmp;
  if (gfits_scan (&header, "NEWGAIN", "%f", 1, &tmp)) eperdn = tmp;

  /* we are scaling the image to electrons, must also scale parameters */
  itop *= eperdn;
  ibot *= eperdn;
  tmax *= eperdn;
  cmax *= eperdn;
  ctpersat *= eperdn;

  dophot (); 

  fprintf (stderr, "SUCCESS\n");
  exit (0);
}

/* in the rest of the program:
   NFAST = matrix.Naxis[0]  -- # of X-axis pixels 
   NSLOW = matrix.Naxis[1]  -- # of Y-axis pixels 
*/

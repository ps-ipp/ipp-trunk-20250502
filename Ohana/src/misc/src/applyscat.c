# include <ohana.h>
# include <dvo.h>
# define BYTES_STAR 66

typedef struct {
  double X;
  double Y;
  double M;
} Stars;

int main (int argc, char **argv) {

  Header header;
  Matrix matrix;
  Stars *stars;
  int Nstar;
  off_t Nbytes, nbytes;
  FILE *f;
  char *buffer, line[10];
  double dmag, ratio;
  int i, x, y;

  if (argc != 4) {
    fprintf (stderr, "USAGE: applyscat (input) (scatter) (output)\n");
    exit (1);
  }

  /* read smp header */
  if (!gfits_read_header (argv[1], &header)) {
    fprintf (stderr, "ERROR: can't find image file %s\n", argv[1]);
    exit (1);
  }

  /* open file for star data */
  f = fopen (argv[1], "r");
  if (f == (FILE *) NULL) {
    fprintf (stderr, "ERROR: can't read data from %s\n", argv[1]);
    exit (1);
  }
  fseeko (f, header.datasize, SEEK_SET); 

  /* find expected number of stars */
  gfits_scan (&header, "NSTARS", "%d", 1, &Nstar);
  if (Nstar == 0) {
    fprintf (stderr, "ERROR: can't get NSTARS from header\n");
    exit (1);
  }

  /* allocate work space */
  Nbytes = Nstar*BYTES_STAR;
  ALLOCATE (stars, Stars, Nstar);
  ALLOCATE (buffer, char, Nbytes + 1);

  /* load in data */
  nbytes = fread (buffer, 1, Nbytes, f);
  if (nbytes != Nbytes) {
    fprintf (stderr, "ERROR: can't load in all stars\n");
    exit (1);
  }
  buffer[Nbytes] = 0;
  fclose (f);
  
  for (i = 0; i < Nstar; i++) {
    dparse (&stars[i].X,  1, &buffer[i*BYTES_STAR]);
    dparse (&stars[i].Y,  2, &buffer[i*BYTES_STAR]);
    dparse (&stars[i].M,  3, &buffer[i*BYTES_STAR]);
  }

  if (!gfits_read_matrix (argv[2], &matrix)) {
    fprintf (stderr, "ERROR: can't open scatter image file %s\n", argv[2]);
    exit (1);
  }
  
  /* flat' = flat*ratio
     flux' = flux / ratio
     mag'  = mag - dmag
     dmag = -2.5*log10(ratio) 

     ie, ratio = 1.1
     flux' = 0.9*flux (flux' < flux)
     dmag = -0.103
     mag' = mag + 0.103 (mag' > mag)
  */

  for (i = 0; i < Nstar; i++) {
    x = stars[i].X;
    y = stars[i].Y;
    ratio = gfits_get_matrix_value (&matrix, x, y);
    dmag = -2.5*log10(ratio);
    dmag = (dmag > +1.0) ? 0.0 : dmag;
    dmag = (dmag < -1.0) ? 0.0 : dmag;
    stars[i].M -= dmag;
  }

  /* full format line: "%6.1f %6.1f %6.3f %03d %2d %3.1f %6.3f %6.3f %6.2f %6.2f %5.1f", X, Y, M, dM, dophot, sky, Mgal, Map, fx, fy, df */
  for (i = 0; i < Nstar; i++) {
    sprintf (line, "%6.3f", stars[i].M);
    memcpy (&buffer[i*BYTES_STAR+14], line, 6);
  }

  /* write smp header */
  if (!gfits_write_header (argv[3], &header)) {
    fprintf (stderr, "ERROR: can't write output file header %s\n", argv[3]);
    exit (1);
  }

  /* open file for star data */
  f = fopen (argv[3], "a");
  if (f == (FILE *) NULL) {
    fprintf (stderr, "ERROR: can't open output file %s\n", argv[3]);
    exit (1);
  }
  fseeko (f, header.datasize, SEEK_SET); 

  /* load in data */
  nbytes = Fwrite (buffer, 1, Nbytes, f, "char");
  if (nbytes != Nbytes) {
    fprintf (stderr, "ERROR: problem writing stars\n");
    exit (1);
  }
  fclose (f);

  exit (0);
}




/* load in smp header
   load in smp stars
   load in scatter frame
   apply corrections
   write smp header
   write smp stars
*/

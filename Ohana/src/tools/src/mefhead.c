# include <ohana.h>
# include <gfitsio.h>

int main (int argc, char **argv) {

  struct stat filestat;
  int NEXTEND, Nextend, status;
  off_t Nmatrix;
  Header header;
  FILE *f, *g;

  if (argc != 3) {
    fprintf (stderr, "USAGE: mefhead (input) (output)\n");
    exit (2);
  }

  /* exit if file exists */
  status = stat (argv[2], &filestat);
  if (status != -1) {
    fprintf (stderr, "error: output file exists\n");
    exit (1);
  } 
  
  /* open stream for input */
  g = fopen (argv[1], "r");
  if (g == (FILE *) NULL) {
    fprintf (stderr, "error: can't open input file\n");
    exit (1);
  } 

  /* open stream for output */
  f = fopen (argv[2], "w");
  if (f == (FILE *) NULL) {
    fprintf (stderr, "error: can't create output file\n");
    exit (1);
  } 

  /* read PHU */
  status = gfits_fread_header (g, &header);
  if (!status) {
    fprintf (stderr, "error: can't read primary header unit\n");
    exit (1);
  }
  /* skip matrix */
  Nmatrix = gfits_data_size (&header);
  fseeko (g, Nmatrix, SEEK_CUR);

  /* write PHU */
  gfits_modify (&header, "NAXIS", "%d", 1, 0);
  status = gfits_fwrite_header (f, &header);
  if (!status) {
    fprintf (stderr, "error: can't write primary header unit\n");
    exit (1);
  }

  /* check if NEXTEND is accurate */
  gfits_scan (&header, "NEXTEND", "%d", 1, &NEXTEND);

  Nextend = 0;
  while (1) {
    
    /* read header */
    status = gfits_fread_header (g, &header);
    if (!status) break;

    /* skip matrix */
    Nmatrix = gfits_data_size (&header);
    fseeko (g, Nmatrix, SEEK_CUR);
    
    /* write header */
    gfits_modify (&header, "NAXIS", "%d", 1, 0);
    status = gfits_fwrite_header (f, &header);
    if (!status) {
      fprintf (stderr, "error: can't write header unit %d\n", Nextend);
      exit (1);
    }
    Nextend ++;
    gfits_free_header (&header);
  }

  if (Nextend != NEXTEND) { 
    fprintf (stderr, "warning: mismatch in NEXTEND\n");
  }

  fclose (f);
  fclose (g);
  exit (0);

}



/* mefhead (input) (output) 
   convert (input) MEF image file to (output) MEF header file:
   read all headers
   convert all lines NAXIS=2 to NAXIS=0 
   write only headers

*/

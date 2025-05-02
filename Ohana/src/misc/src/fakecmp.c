# include <ohana.h>
static char reserved[] = "COMMENT  Reserved space.  This line can be used to add a new FITS card.";

void main (int argc, char **argv) {

  off_t start_size, status;
  int i, j, N;
  int Nreserved;
  char *p, keyword[16], line[256];
  FILE *f;
  Header header;

  if (argc != 4) {
    fprintf (stderr, "USAGE: gfits_insert (input) (template) (output)\n");
    exit (2);
  }
  
  /*
    input file consists of: RA DEC MAG dMAG
    we generate an artificial image with 1 arcsec/pix
    and the pixel (0,0) at (<RA>,<DEC>)
    
    1) find <RA>, <DEC>
    2) convert X,Y to R,D using <RA>,<DEC> (ra---sin)
    3) fix template entries: CRVAL1, CRVAL2, NSTARS, (JD)
    4) write out template
    5) write out infile in correct format:
    X     Y      M      dM   T S   Ma     Mb        Fx     Fy  Th
    237.2 2646.1 16.748 085  1 2.0 17.930 17.768    nan    nan -48.0
  */

  /* allocate data space */
  NSTARS = 1000;
  ALLOCATE (ra, double, NSTARS);
  ALLOCATE (dec, double, NSTARS);
  ALLOCATE (mag, double, NSTARS);
  ALLOCATE (dmag, double, NSTARS);

  /* open data file */
  f = fopen (argv[1], "r");
  if (f == (FILE *) NULL) {
    fprintf (stderr, "can't open data file %s\n", argv[1]);
    exit (1);
  }

  /* load data file into array */
  for (i = 0;
       fscanf (f, "%lf %lf %lf %lf", &ra[i], &dec[i], &mag[i], &dmag[i]) != EOF;
       i++) {
    
    if (i == NSTARS - 1) {
      NSTARS += 1000;
      REALLOCATE (ra, double, NSTARS);
      REALLOCATE (dec, double, NSTARS);
      REALLOCATE (mag, double, NSTARS);
      REALLOCATE (dmag, double, NSTARS);
    }
  }
  NSTARS = i;

  /* calculate <ra>, <dec> */
  RA = DEC = 0;
  for (i = 0; i < NSTARS; i++) {
    RA += ra[i];
    

  /* load header from image file */
  if (!gfits_read_header (argv[1], &header)) {
    fprintf (stderr, "can't open fits file %s\n", argv[1]);
    exit (1);
  }
  start_size = header.datasize;

  /* run through the lines in the header data file */
  while (scan_line (f, line) != EOF) {
    
    /* fill in end of line with blanks, force end at 80 */
    for (N = strlen (line); N < 80; N++) {
      line[N] = ' ';
    }
    line[80] = 0;
    bzero (keyword, 10);
    strncpy_nowarn (keyword, line, 8);
    
    /* replace existing keywords, unless this is a COMMENT or HISTORY field */
    if (strncmp (keyword, "COMMENT ", 8) && strncmp (keyword, "HISTORY ", 8)) {
      p = gfits_header_field (&header, keyword, 1);
      if (p != (char *) NULL) {
	strncpy_nowarn (p, line, 80);
	continue;
      }
    }

    /* find first line with the reserved line */
    p = (char *) NULL;
    Nreserved = strlen (reserved);
    for (i = 0; (i < header.datasize) && (p == (char *) NULL) ; i+= FT_LINE_LENGTH) {
      if (!strncmp (&header.buffer[i], reserved, Nreserved)) {
	p = &header.buffer[i];
      }
    }
    if (p == (char *) NULL) {
      fprintf (stderr, "no more reserved spaces!\n");
      exit (1);
    }

    /* insert the new line here */
    strncpy_nowarn (p, line, 80);
  }

  fclose (f);

  /* now write the new header on top of the old one. 
     check first that the header size has not changed.
  */

  if (header.datasize != start_size) {
    fprintf (stderr, "header changed size: should not happen!\n");
    exit (1);
  }

  f = fopen (argv[1], "r+");
  if (f == NULL) {
    fprintf (stderr, "can't open file for update %s\n", argv[1]);
    exit (1);
  }

  fseeko (f, 0, SEEK_SET);
  status = fwrite (header.buffer, 1, header.datasize, f);
  if (status != header.datasize) {
    fprintf (stderr, "failed to write data to image header\n");
    exit (1);
  }
  fclose (f);

  exit (0);
}
    

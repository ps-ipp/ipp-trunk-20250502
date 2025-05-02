# include <ohana.h>
# include <gfitsio.h>

static char reserved[] =  "COMMENT  Reserved space.  This line can be used to add a new FITS card.         ";
static char blankline[] = "                                                                                ";

int main (int argc, char **argv) {

  off_t i, skip, Nbytes, start_size;
  int N, status, EXTNUM, Delete, DeleteStart, DeleteStop;
  int Nreserved;
  char *p, keyword[16], line[256];
  FILE *f;
  Header header;
  int COMMENT, Cnumber;
  char *Cline;

  Cline = NULL;
  Cnumber = 0;
  DeleteStart = DeleteStop = 0;

  /* check for command line options */
  COMMENT = FALSE;
  if ((N = get_argument (argc, argv, "-comment"))) {
    COMMENT = TRUE;
    remove_argument (N, &argc, argv);
    Cnumber = atof (argv[N]);
    remove_argument (N, &argc, argv);
    Cline = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }

  /* check for delete options */
  Delete = FALSE;
  if ((N = get_argument (argc, argv, "-delete"))) {
    Delete = TRUE;
    remove_argument (N, &argc, argv);
    DeleteStart = atoi (argv[N]);
    remove_argument (N, &argc, argv);
    DeleteStop = atoi (argv[N]);
    remove_argument (N, &argc, argv);
  }

  /* check for command line options */
  EXTNUM = -1; /* -1 is primary header */
  if ((N = get_argument (argc, argv, "-X"))) {
    remove_argument (N, &argc, argv);
    EXTNUM = atof (argv[N]);
    remove_argument (N, &argc, argv);
  }

  if (argc != 3) {
    fprintf (stderr, "USAGE: fits_insert (image.fits) (header.hdx) [-X N] [-comment N line] [-delete Nstart Nstop]\n");
    exit (2);
  }
  
  f = fopen (argv[1], "r");
  if (f == NULL) {
    fprintf (stderr, "can't open fits file %s\n", argv[1]);
    exit (1);
  }

  /* load header from image file */
  Nbytes = gfits_fread_Xheader (f, &header, EXTNUM);
  if (!Nbytes) {
      fprintf (stderr, "can't read extension %d\n", EXTNUM);
      exit (1);
  }
  start_size = header.datasize;
  skip = Nbytes - header.datasize;

  /* open header data file */
  f = fopen (argv[2], "r");
  if (f == (FILE *) NULL) {
    fprintf (stderr, "can't open header data file %s\n", argv[2]);
    exit (1);
  }

  /* wipe out COMMENT N and replace with given line */ 
  if (COMMENT) {

    /* create a pristine FITS line */
    snprintf (line, 81, "COMMENT  %-71s", Cline);

    p = gfits_header_field (&header, "COMMENT", Cnumber);
    if (p != (char *) NULL) {
      memcpy (p, line, 80); // do not use strncpy_nowarn: do NOT set last byte to NULL
    }
  }

  /* wipe out lines from DeleteStart to < DeleteStop */ 
  if (Delete) {
    for (i = DeleteStart; i < DeleteStop; i++) {
      p = gfits_header_lineno (&header, i);
      if (p != (char *) NULL) {
	memset (p, ' ', 80);
      }
    }
  }

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

    /* check that the line does not already exist */
    p = (char *) NULL;
    for (i = 0; (i < header.datasize) && (p == (char *) NULL) ; i+= FT_LINE_LENGTH) {
      if (!strncmp (&header.buffer[i], line, 80)) {
	p = &header.buffer[i];
      }
    }
    if (p != (char *) NULL) continue;

    /* find first line with the reserved line */
    p = (char *) NULL;
    Nreserved = strlen (reserved);
    for (i = 0; (i < header.datasize) && (p == (char *) NULL) ; i+= FT_LINE_LENGTH) {
      if (!strncmp (&header.buffer[i], "END     ", 8)) break;
      if (!strncmp (&header.buffer[i], reserved, Nreserved)) {
	p = &header.buffer[i];
      }
      if (!strncmp (&header.buffer[i], blankline, Nreserved)) {
	p = &header.buffer[i];
      }
    }
    if (p == (char *) NULL) {
      fprintf (stdout, "no more reserved spaces, trying for extra space in block\n");
      p = gfits_header_field (&header, "END", 1);
      if (p == (char *) NULL) {
	fprintf (stderr, "header is missing END\n");
	exit (1);
      }
      if (p - header.buffer + 80 == header.datasize) {
	fprintf (stderr, "no free space in block, can't insert keyword\n");
	exit (1);
      }
      memcpy (p+80, "END", 3); // do not use strncpy_nowarn, do NOT set last byte to NULL
      for (i = 3; i < 80; i++) { p[80+i] = ' '; }
    }
    /* insert the new line here */
    memcpy (p, line, 80); // do not use strncpy_nowarn, do NOT set last byte to NULL
  }
  if (fclose (f)) {
    fprintf (stderr, "error reading new keywords\n");
    exit (1);
  }

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

  fseeko (f, skip, SEEK_SET);
  status = fwrite (header.buffer, 1, header.datasize, f);
  if (status != header.datasize) {
    fprintf (stderr, "failed to write data to image header\n");
    exit (1);
  }
  if (fclose (f)) {
    fprintf (stderr, "error writing data to disk\n");
    exit (1);
  }

  exit (0);
}
    

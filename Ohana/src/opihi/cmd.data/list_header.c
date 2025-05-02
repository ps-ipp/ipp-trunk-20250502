# include "data.h"

# define SKIP_HEADER(NAME) { if (!strncmp(line, NAME, strlen(NAME))) continue; }

int header (int argc, char **argv) {
  
  int j, N, nlines, nbytes, Nbytes, LOADHEAD, status, bitpix, unsign;
  char *p, filename[128];
  FILE *f;
  double bscale, bzero;
  Buffer *buf;

  LOADHEAD = FALSE;
  if ((N = get_argument (argc, argv, "-w"))) {
    remove_argument (N, &argc, argv);
    strcpy (filename, argv[N]);
    remove_argument (N, &argc, argv);
    LOADHEAD = TRUE;
  }
  if ((N = get_argument (argc, argv, "-file"))) {
    remove_argument (N, &argc, argv);
    strcpy (filename, argv[N]);
    remove_argument (N, &argc, argv);
    LOADHEAD = TRUE;
  }

  char *COPYHEAD = NULL;
  if ((N = get_argument (argc, argv, "-copy"))) {
    if (LOADHEAD) {
      gprint (GP_ERR, "-copy and -w are incompatible\n");
      return (FALSE);
    }
    remove_argument (N, &argc, argv);
    COPYHEAD = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }

  if (argc != 2) {
    gprint (GP_ERR, "USAGE: header <buffer> [-w filename] [-file filename] [-copy buffer]\n");
    gprint (GP_ERR, "  -file / -w : replace buffer header with contents from file (PHU only)\n");
    gprint (GP_ERR, "  -copy : replace buffer header with contents of buffer\n");
    return (FALSE);
  }

  if ((buf = SelectBuffer (argv[1], OLDBUFFER, TRUE)) == NULL) return (FALSE);

  if (COPYHEAD) {
    Buffer *src = NULL;
    if ((src = SelectBuffer (COPYHEAD, OLDBUFFER, TRUE)) == NULL) return (FALSE);

    // loop over all lines in the header and copy one-by-one
    // skip the special words:
    
    for (int i = 0; TRUE; i++) {
      char *line = gfits_header_lineno (&src[0].header, i);
      if (!line) break;

      // skip special keywords
      SKIP_HEADER ("SIMPLE");
      SKIP_HEADER ("SIMPLE");
      SKIP_HEADER ("BITPIX");
      SKIP_HEADER ("NAXIS");
      SKIP_HEADER ("EXTEND");
      SKIP_HEADER ("UNSIGN");
      SKIP_HEADER ("BSCALE");
      SKIP_HEADER ("BZERO");
      SKIP_HEADER ("NAXIS1");
      SKIP_HEADER ("NAXIS2");
      SKIP_HEADER ("NAXIS3");
      SKIP_HEADER ("NAXIS4");
      SKIP_HEADER ("NAXIS5");
      SKIP_HEADER ("NAXIS6");
      SKIP_HEADER ("NAXIS7");
      SKIP_HEADER ("NAXIS8");
      SKIP_HEADER ("NAXIS9");
      SKIP_HEADER ("NAXIS10");
      SKIP_HEADER ("PCOUNT");
      SKIP_HEADER ("GCOUNT");

      gfits_header_append_line_raw (&buf[0].header, line);
    }
    return TRUE;

  }

  if (LOADHEAD) {
    f = fopen (filename, "r");
    if (f == (FILE *) NULL) {
      gprint (GP_ERR, "file %s not found\n", filename);
      return (FALSE);
    }
    fclose (f);
    
    bitpix = buf[0].header.bitpix;
    bzero  = buf[0].header.bzero;
    bscale = buf[0].header.bscale;
    unsign = buf[0].header.unsign;
    gfits_free_header (&buf[0].header);
    
    strcpy (filename, buf[0].file);
    strcpy (buf[0].file, "*");
    strcat (buf[0].file, filename);

    status = gfits_read_header (argv[2], &buf[0].header);
    if (!status) {
      gprint (GP_ERR, "failed to read header for %s\n", argv[2]);
      return FALSE;
    }
    buf[0].header.bitpix = bitpix;     
    buf[0].header.bzero  = bzero;      
    buf[0].header.bscale = bscale;     
    buf[0].header.unsign = unsign;     
    gfits_modify (&buf[0].header, "BITPIX", "%d",  1, bitpix);
    gfits_modify (&buf[0].header, "BSCALE", "%lf", 1, bscale);
    gfits_modify (&buf[0].header, "BZERO",  "%lf", 1, bzero);
    gfits_modify_alt (&buf[0].header, "UNSIGN", "%t",  1, unsign);
    
  } else {

    f = popen ("more", "w");
    
    p = gfits_header_field (&buf[0].header, "END", 1);
    nlines = (p - buf[0].header.buffer) / 80;
    nbytes = 81*nlines;

    /* duplicate the header, add in the <return> chars, send to more */
    ALLOCATE (p, char, nbytes);
    for (j = 0; j < nlines; j++) {
      memcpy (&p[81*j], &buf[0].header.buffer[80*j], 80);
      p[81*j+80] = 10;
    }
    Nbytes = fwrite (p, sizeof(char), nbytes, f);
    if (Nbytes != nbytes) {
      gprint (GP_ERR, "warning: not all printed...\n");
    }
    free (p);

    pclose (f);

  }

  return (TRUE);

}

/* XXX this function is not written in the context of the output file/buffer */

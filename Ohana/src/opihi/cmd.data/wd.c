# include "data.h"
FILE *gfits_open_and_extend (char *filename);

int wd (int argc, char **argv) {
  
  int N, Extend;
  int newUnsign, newBitpix, newScale, newZero;
  int outUnsign, outBitpix;
  double outScale, outZero;
  Header temp_header;
  Matrix temp_matrix;
  Buffer *buf;

  /* XXX I must have dropped the old 'newplane' option */
  Extend  = FALSE;
  if ((N = get_argument (argc, argv, "-extend"))) {
    remove_argument (N, &argc, argv);
    Extend  = TRUE;
  }

  char *CompressMode = NULL;
  if ((N = get_argument (argc, argv, "-compress-mode"))) {
    remove_argument (N, &argc, argv);
    CompressMode = strcreate(argv[N]);
    remove_argument (N, &argc, argv);
  } else if ((N = get_argument (argc, argv, "-compress"))) {
    remove_argument (N, &argc, argv);
    CompressMode = strcreate("GZIP_1");
  }

  outZero = 0;
  newZero = FALSE;
  if ((N = get_argument (argc, argv, "-bzero"))) {
    remove_argument (N, &argc, argv);
    outZero  = atof(argv[N]);
    newZero  = TRUE;
    remove_argument (N, &argc, argv);
  }

  outScale = 1;
  newScale = FALSE;
  if ((N = get_argument (argc, argv, "-bscale"))) {
    remove_argument (N, &argc, argv);
    outScale = atof(argv[N]);
    newScale = TRUE;
    remove_argument (N, &argc, argv);
  }

  outBitpix = 16;
  newBitpix = FALSE;
  if ((N = get_argument (argc, argv, "-bitpix"))) {
    remove_argument (N, &argc, argv);
    outBitpix = atof(argv[N]);
    newBitpix = TRUE;
    remove_argument (N, &argc, argv);
  }

  outUnsign = FALSE;
  newUnsign = FALSE;
  if ((N = get_argument (argc, argv, "-unsign"))) {
    remove_argument (N, &argc, argv);
    outUnsign = -1;
    if (!strcasecmp (argv[N], "t") || !strcasecmp (argv[N], "true")) outUnsign = TRUE;
    if (!strcasecmp (argv[N], "f") || !strcasecmp (argv[N], "false")) outUnsign = FALSE;
    if (outUnsign == -1) {
      gprint (GP_ERR, "-unsign options: t, f, true, false\n");
      FREE (CompressMode);
      return (FALSE);
    }
    newUnsign = TRUE;
    remove_argument (N, &argc, argv);
  }

  if (argc != 3) {
    gprint (GP_ERR, "USAGE: wd <buffer> <filename> [-bitpix N] [-bscale X] [-bzero X] [-extend] [-newplane]\n");
    FREE (CompressMode);
    return (FALSE);
  }

  if ((buf = SelectBuffer (argv[1], OLDBUFFER, TRUE)) == NULL) { FREE (CompressMode); return (FALSE); }

  if (!newBitpix) outBitpix = buf[0].bitpix;
  if (!newScale) outScale = buf[0].bscale;
  if (!newZero) outZero = buf[0].bzero;
  if (!newUnsign) outUnsign = buf[0].unsign;

  /* Convert the buffer from (float) to correct format */
  /* save the (float) version, write out a temporary buffer */
  temp_matrix = buf[0].matrix;
  ALLOCATE (temp_matrix.buffer, char, MAX(1, temp_matrix.datasize));
  memcpy (temp_matrix.buffer, buf[0].matrix.buffer, temp_matrix.datasize);
  temp_header = buf[0].header;
  ALLOCATE (temp_header.buffer, char, MAX(1, temp_header.datasize));
  memcpy (temp_header.buffer, buf[0].header.buffer, temp_header.datasize);

  gfits_convert_format (&temp_header, &temp_matrix, outBitpix, outScale, outZero, 0xffff, outUnsign);

  // Extend puts the output matrix in the first available non-PHU slot (ie, the last one)
  // it updates NEXTEND and set EXTEND to TRUE, and modifies the PHU header (neither should happen)
  // if those keywords do not exist...
  if (Extend && !CompressMode) {
    int status = TRUE;

    FILE *f = gfits_open_and_extend (argv[2]);
    if (!f) {
      gprint (GP_ERR, "failed to extend file %s\n", argv[2]);
      status = FALSE;
      goto done1;
    }

    /* fix up header */
    {
      static char simple[] = "XTENSION= 'IMAGE  '            / Image extension";
      int Ns, No;
      Ns = strlen (simple);
      No = 80 - Ns;
      strncpy_nowarn (temp_header.buffer, simple, Ns);
      memset (&temp_header.buffer[Ns], ' ', No);
    }

    /* position to end of file to write new extend */
    fseeko (f, 0LL, SEEK_END);
    off_t nbytes = fwrite (temp_header.buffer, 1, temp_header.datasize, f);
    if (nbytes != temp_header.datasize) {
      fclose (f);
      gprint (GP_ERR, "failed to write file\n");
      status = FALSE;
      goto done1;
    }

    /* write the matrix buffer (automatically goes to end of file */
    if (!gfits_fwrite_matrix (f, &temp_matrix)) {
      fclose (f);
      gprint (GP_ERR, "failed to write file\n");
      status = FALSE;
      goto done1;
    }
    fclose (f);
    status = TRUE;

  done1:
    gfits_free_header (&temp_header);
    gfits_free_matrix (&temp_matrix);
    FREE (CompressMode);
    return (status);
  }
  
  // not compatible with extend
  if (CompressMode) {
    Header myHeader;
    Matrix myMatrix;

    FILE *f;

    if (Extend) {
      // keeps an existing file
      f = gfits_open_and_extend (argv[2]);
      fseeko (f, 0LL, SEEK_END);
    } else {
      // replaces an existing file
      f = fopen (argv[2], "w");
      if (!f) {
	fprintf (stderr, "ERROR: cannot open image subset file for output %s\n", argv[2]);
	FREE (CompressMode);
	return FALSE;
      }
   
      gfits_init_header (&myHeader);
      myHeader.extend = TRUE;
      gfits_create_header (&myHeader);
      gfits_create_matrix (&myHeader, &myMatrix);
      gfits_fwrite_header  (f, &myHeader);
      gfits_fwrite_matrix  (f, &myMatrix);
      gfits_free_header (&myHeader);
      gfits_free_matrix (&myMatrix);
    }

    FTable ftable;
    Header theader;

    ftable.header = &theader;

    gfits_compress_image (&temp_header, &temp_matrix, &ftable, NULL, CompressMode);
    gfits_byteswap_varlength_column (&ftable, 1);
    gfits_fwrite_Theader (f, &theader);
    gfits_fwrite_table  (f, &ftable);
    fclose (f);

    gfits_free_header (&theader);
    gfits_free_table (&ftable);
    FREE (CompressMode);
    return (TRUE);
  }

  /* the actual write-to-disk goes here */
  if (!gfits_write_header (argv[2], &temp_header)) {
    gprint (GP_ERR, "failed to write header\n");
    gfits_free_header (&temp_header);
    gfits_free_matrix (&temp_matrix);
    FREE (CompressMode);
    return (FALSE);
  }
  
  if (!gfits_write_matrix (argv[2], &temp_matrix)) {
    gprint (GP_ERR, "failed to write matrix\n");
    gfits_free_header (&temp_header);
    gfits_free_matrix (&temp_matrix);
    FREE (CompressMode);
    return (FALSE);
  }

  gfits_free_header (&temp_header);
  gfits_free_matrix (&temp_matrix);
  
  FREE (CompressMode);
  return (TRUE);
}

// prepare the file to add an extension:
// read the PHU, update NEXTEND, write back header
// return the file pointer open and ready for write
FILE *gfits_open_and_extend (char *filename) {

  Header Xhead;
  FILE *f = NULL;
  struct stat filestat;

  // does the file already exist?
  int status = stat (filename, &filestat);
  if (status == 0) { /* file exists, are permissions OK? */
    f = fopen (filename, "r+");
    if (f == NULL) {
      gprint (GP_ERR, "failed to open file %s for write\n", filename);
      return NULL;
    }

    status = gfits_fread_header (f, &Xhead);
    if (!status) {
      gprint (GP_ERR, "failed to read header for existing file %s\n", filename);
      return NULL;
    }
  } else {
    f = fopen (filename, "w");
    if (f == NULL) {
      gprint (GP_ERR, "failed to create file %s for write\n", filename);
      return NULL;
    }

    // create an empty header
    gfits_init_header (&Xhead);
    Xhead.bitpix = 16;
    Xhead.extend = TRUE;
    gfits_create_header (&Xhead);

    gfits_modify (&Xhead, "NEXTEND", "%d", 1, 0);
  }

  gfits_modify_alt (&Xhead, "EXTEND", "%t", 1, TRUE);

  int Nextend = 0;
  gfits_scan (&Xhead, "NEXTEND", "%d", 1, &Nextend);
  Nextend ++;
  gfits_modify (&Xhead, "NEXTEND", "%d", 1, Nextend);

  /* position to begining of file to write header */
  fseeko (f, 0LL, SEEK_SET);
  off_t nbytes = fwrite (Xhead.buffer, 1, Xhead.datasize, f);
  if (nbytes != Xhead.datasize) {
    gfits_free_header (&Xhead);
    gprint (GP_ERR, "ERROR: failed writing data to image header\n");
    return NULL;
  }
  gfits_free_header (&Xhead);
    
  return (f);
}

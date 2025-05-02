# include "data.h"

/* there is some confusion in this function with the several possible options */
int rd (int argc, char **argv) {
  
  int i, N, Nskip, blank;
  int done, Nword;
  char region[512];
  Buffer *buf;

  int VERBOSE = FALSE;
  if ((N = get_argument (argc, argv, "-v"))) {
    remove_argument (N, &argc, argv);
    VERBOSE = TRUE;
  }

  int JustHead = FALSE;
  if ((N = get_argument (argc, argv, "-head"))) {
    remove_argument (N, &argc, argv);
    JustHead = TRUE;
  }

  int SwapRaw = FALSE;
  if ((N = get_argument (argc, argv, "-swap-raw"))) {
    remove_argument (N, &argc, argv);
    SwapRaw = TRUE;
  }

  int plane = -1;
  if ((N = get_argument (argc, argv, "-plane"))) {
    remove_argument (N, &argc, argv);
    plane  = atof(argv[N]);
    remove_argument (N, &argc, argv);
  }

  int extend = FALSE;
  int Nextend = -1;
  if ((N = get_argument (argc, argv, "-x"))) {
    remove_argument (N, &argc, argv);
    Nextend  = atof(argv[N]);
    remove_argument (N, &argc, argv);
    extend = TRUE;
  }

  int ccdsel = FALSE;
  char *ccdid = NULL;
  if ((N = get_argument (argc, argv, "-n"))) {
    remove_argument (N, &argc, argv);
    ccdid  = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
    ccdsel = TRUE;
  }

  if (extend && ccdsel) {
    gprint (GP_ERR, "only specify one of -n and -x\n");
    return (FALSE);
  }

  if (argc != 3) {
    gprint (GP_ERR, "USAGE: rd <buffer> <filename> [-head] [-plane N] [-n ccdid] [-x extnum]\n");
    return (FALSE);
  }

  /* test if file exists */
  FILE *f = fopen (argv[2], "r");
  if (f == (FILE *) NULL) {
    gprint (GP_ERR, "file %s not found\n", argv[2]);
    return (FALSE);
  }

  /* find matrix, free old data */
  if ((buf = SelectBuffer (argv[1], ANYBUFFER, TRUE)) == NULL) {
      fclose (f);
      return (FALSE);
  }
  gfits_free_matrix (&buf[0].matrix);
  gfits_free_header (&buf[0].header);

  /* save file name */
  char *filename = filebasename (argv[2]);
  strcpy (buf[0].file, filename);
  free (filename);

  int status = FALSE;
  int IsCompressed = FALSE;

  /*** advance to the correct FITS extension ***/

  /* FITS extension by number */
  if (extend) {
    /* load in appropriate header */
    Nskip = gfits_fread_Xheader (f, &buf[0].header, Nextend);
    if (!Nskip) {
      gprint (GP_ERR, "entry in %s not found\n", argv[2]);
      DeleteBuffer (buf);
      fclose (f);
      return (FALSE);
    }
    if (gfits_extension_is_compressed_image (&buf[0].header)) {
	IsCompressed = TRUE;
    }
  } 

  /* FITS extension by name */
  if (ccdsel) {
    char *CCDKeyword, ID[64];

    CCDKeyword = get_variable ("CCDKEYWORD");
    if (CCDKeyword == (char *) NULL) {
      // gprint (GP_ERR, "CCDKEYWORD variable is not set; ");
      // gprint (GP_ERR, "using EXTNAME as default\n");
      CCDKeyword = strcreate ("EXTNAME");
    }
    done = FALSE;
    for (i = 0; !done; i++) {
      Nskip = gfits_fread_Xheader (f, &buf[0].header, i);
      if (!Nskip) {
	gprint (GP_ERR, "extension %s in %s not found\n", ccdid, argv[2]);
	DeleteBuffer (buf);
	free (CCDKeyword);
	fclose (f);
	return (FALSE);
      }

      // for compressed data tables, EXTNAME may be duplicated, with the first one containing the
      // word 'COMPRESSED_IMAGE'.  in this case, check the second EXTNAME, if CCDKeyword == EXTNAME 
      // this may have to be a more obscure test specifically for 'imcopy' data...
      // need to check each header, since file may contain a mix
      
      Nword = 1;
      IsCompressed = FALSE;
      if (gfits_extension_is_compressed_image (&buf[0].header)) {
	if (!strcmp (CCDKeyword, "EXTNAME")) Nword = 1;
	IsCompressed = TRUE;
      }
      if (!gfits_scan (&buf[0].header, CCDKeyword, "%s", Nword, ID)) {
	gprint (GP_ERR, "%s not in header\n", CCDKeyword);
	DeleteBuffer (buf);
	free (CCDKeyword);
	fclose (f);
	return (FALSE);
      }

      /* compare as numbers if both are pure numbers, else as strings */
      done = strnumcmp (ccdid, ID);
      if (!done) gfits_free_header (&buf[0].header);
    }
    free (CCDKeyword);
  }

  /* fix up header, if needed */
  if (extend || ccdsel) {
    if (!IsCompressed) {
      gfits_extended_to_primary (&buf[0].header, TRUE, "Standard FITS");
    }
  } else {
      gfits_fread_header (f, &buf[0].header);
  }

  /* for JustHead, we skip reading the data segment */ 
  // XXX for compressed data, we need to convert the header to the equivalent uncompressed version
  if (JustHead) {
    // set the number of pixels to zero:
    gfits_init_matrix (&buf[0].matrix);
    ALLOCATE (buf[0].matrix.buffer, char, 1);
    buf[0].bitpix = 16;
    buf[0].bzero = 0;
    buf[0].bscale = 1;
    buf[0].header.bitpix = 16;
    buf[0].header.bzero = 0;
    buf[0].header.bscale = 1;
    buf[0].header.Naxes = 0;
    fclose (f);
    return (TRUE);
  }

  /* check for valid plane */
  int Nz = buf[0].header.Naxis[2];
  if (plane >= 0) {
    // we are requesting a specific plane (-1 : all data)
    int tooFar = (Nz > 0) ? (plane >= Nz) : (plane > 0);
    if (tooFar) {
      gprint (GP_ERR, "-plane is too large: %d total planes\n", Nz);
      DeleteBuffer (buf);
      fclose (f);
      return (FALSE);
    }
  }

  /* load matrix data */
  if (IsCompressed) {
    if (plane > -1) {
      gprint (GP_ERR, "-plane incompatible with compressed image\n");
      DeleteBuffer (buf);
      fclose (f);
      return (FALSE);
    }
    FTable ftable;
    Header theader;
    ftable.header = &theader;
    ftable.header[0].buffer = NULL;
    gfits_copy_header (&buf[0].header, ftable.header);

    if (!gfits_fread_ftable_data (f, &ftable, FALSE)) { gprint (GP_ERR, "problem reading compressed image table data\n"); fclose (f); return FALSE; }
    if (!gfits_byteswap_varlength_column (&ftable, 1)) { gprint (GP_ERR, "problem doing byteswap on compressed image metadata column\n"); fclose (f); gfits_free_table (&ftable); return FALSE; }
    if (!gfits_uncompress_image (&buf[0].header, &buf[0].matrix, &ftable)) { gprint (GP_ERR, "problem uncompressing the image data\n"); fclose (f); gfits_free_table (&ftable); return FALSE; }

    // uncompressing the image leaves the format as an extension
    gfits_extended_to_primary (&buf[0].header, TRUE, "Standard FITS");
    gfits_free_table (&ftable);

    status = TRUE;
    // XXX this currently does not work for a cube (we get a cube back, not a specific plane)
  } else {
    if (plane > -1) {
      // read a single plane into a 2D image
      sprintf (region, "-1 -1 -1 -1 %d %d", (plane - 1), plane);
      status = gfits_fread_matrix_segment (f, &buf[0].matrix, &buf[0].header, region);
      buf[0].header.Naxis[2] = 0;
      buf[0].header.Naxes = 2;
      gfits_modify (&buf[0].header, "NAXIS", "%d", 1, 2);
      gfits_modify (&buf[0].header, "NAXIS3", "%d", 1, 0);
    } else {
      status = gfits_fread_matrix (f, &buf[0].matrix, &buf[0].header);
    }
  }
  fclose (f);

  if (!status) {
    gprint (GP_ERR, "problem reading file, buffer not opened\n");
    DeleteBuffer (buf);
    return (FALSE);
  }

  /* we need to return a 2D array, convert 1D images to 2D (Naxis[1] = 1) */
  if (buf[0].header.Naxes == 1) {
    buf[0].header.Naxes = 2;
    buf[0].header.Naxis[1] = 1;
    buf[0].matrix.Naxis[1] = 1;
    gfits_modify (&buf[0].header, "NAXIS", "%d", 1, 2);
    gfits_modify (&buf[0].header, "NAXIS2", "%d", 1, 1);
  }    

  buf[0].bitpix = buf[0].header.bitpix;    /* store the original values */
  buf[0].bscale = buf[0].header.bscale;    /* store the original values */
  buf[0].bzero  = buf[0].header.bzero;     /* store the original values */
  buf[0].unsign = buf[0].header.unsign;

  if (VERBOSE) gprint (GP_LOG, "read "OFF_T_FMT" bytes from %s into buffer %s\n", buf[0].header.datasize + buf[0].matrix.datasize, argv[2], argv[1]);

  blank = 0xffff;
  gfits_scan (&buf[0].header, "BLANK", "%d", 1, &blank);

  if (SwapRaw) {
    gfits_swap_raw (&buf[0].matrix);
  }

  /** now - convert the matrix values to floats for internal use **/
  gfits_convert_format (&buf[0].header, &buf[0].matrix, -32, 1.0, 0.0, blank, gfits_get_unsign_mode());

  return (TRUE);
}

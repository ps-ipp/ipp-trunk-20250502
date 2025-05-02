# include <dvo.h>

/* the old Image.dat files used a fake FITS header defining a finite data block
 * this function reads in the header and data, and creates an ftable and theader to match
 */

int dvo_image_load_raw (FITS_DB *db, int VERBOSE, int FORCE_READ) {

  off_t Nimage, Ndata, size, ImageSize, nbytes, Nbytes;
  struct stat filestatus;
  char format[80], telescope[80];

  /* read fits header from file - return FALSE on error */
  if (!gfits_fread_header (db[0].f, &db[0].header)) {
    fprintf (stderr, "can't read primary header\n"); 
    return (FALSE);
  }

  /* determine image table format */
  db[0].format = DVO_FORMAT_UNDEF;
  if (gfits_scan (&db[0].header, "FORMAT",  "%s", 1, format)) {
    db[0].format = dvo_catalog_catformat (format);
    if (db[0].format != DVO_FORMAT_UNDEF) goto got_format;
  }
  if (gfits_scan (&db[0].header, "TELESCOP",  "%s", 1, telescope)) {
    if (!strncmp (telescope, "LONEOS", strlen("LONEOS"))) {
      db[0].format = DVO_FORMAT_LONEOS; // special case for LONEOS
      goto got_format;
    }
    if (!strncmp (telescope, "1.3m McGraw-Hill", strlen("1.3m McGraw-Hill"))) {
      db[0].format = DVO_FORMAT_ELIXIR; // special case for ELIXIR
      goto got_format;
    }
  }
  if (VERBOSE) fprintf (stderr, "cannot determine image table format\n");
  return (FALSE);

got_format:
  /* find number of images */
  Nimage = 0;
  gfits_scan (&db[0].header, "NIMAGES", OFF_T_FMT, 1,  &Nimage);
  if (stat (db[0].filename, &filestatus) == -1) {
    if (VERBOSE) fprintf (stderr, "ERROR: failed to get status of image catalog\n");
    exit (1);
  }

  /* get datatype size */
  ImageSize = 0;
  if (db[0].format == DVO_FORMAT_INTERNAL)  	  ImageSize = sizeof(Image);
  if (db[0].format == DVO_FORMAT_LONEOS)    	  ImageSize = sizeof(Image_Loneos);
  if (db[0].format == DVO_FORMAT_ELIXIR)    	  ImageSize = sizeof(Image_Elixir);
  if (db[0].format == DVO_FORMAT_PANSTARRS_DEV_0) ImageSize = sizeof(Image_Panstarrs_DEV_0);
  if (db[0].format == DVO_FORMAT_PANSTARRS_DEV_1) ImageSize = sizeof(Image_Panstarrs_DEV_1);
  if (db[0].format == DVO_FORMAT_PS1_DEV_1)       ImageSize = sizeof(Image_PS1_DEV_1);
  if (db[0].format == DVO_FORMAT_PS1_DEV_2)       ImageSize = sizeof(Image_PS1_DEV_2);
  if (db[0].format == DVO_FORMAT_PS1_DEV_3)       ImageSize = sizeof(Image_PS1_DEV_3);
  if (db[0].format == DVO_FORMAT_PS1_V1)          ImageSize = sizeof(Image_PS1_V1);
  if (db[0].format == DVO_FORMAT_PS1_V2)          ImageSize = sizeof(Image_PS1_V2);
  if (db[0].format == DVO_FORMAT_PS1_V3)          ImageSize = sizeof(Image_PS1_V3);
  if (db[0].format == DVO_FORMAT_PS1_V4)          ImageSize = sizeof(Image_PS1_V4);
  if (db[0].format == DVO_FORMAT_PS1_V5)          ImageSize = sizeof(Image_PS1_V5);
  if (db[0].format == DVO_FORMAT_PS1_V6)          ImageSize = sizeof(Image_PS1_V6);
  if (db[0].format == DVO_FORMAT_PS1_V5_LOAD)     ImageSize = sizeof(Image_PS1_V5_LOAD);
  if (db[0].format == DVO_FORMAT_PS1_REF)         ImageSize = sizeof(Image_PS1_REF);
  if (db[0].format == DVO_FORMAT_PS1_REF_V2)      ImageSize = sizeof(Image_PS1_REF_V2);
  if (db[0].format == DVO_FORMAT_PS1_REF_V3)      ImageSize = sizeof(Image_PS1_REF_V3);
  if (db[0].format == DVO_FORMAT_PS1_SIM)         ImageSize = sizeof(Image_PS1_SIM);

  /* check that filesize makes sense */
  size = Nimage*ImageSize + db[0].header.datasize;
  if (size != filestatus.st_size) {
    Ndata = (filestatus.st_size - db[0].header.datasize) / ImageSize;
    if (VERBOSE) fprintf (stderr, "ERROR: image catalog has inconsistent size\n");
    if (VERBOSE) fprintf (stderr, "header: "OFF_T_FMT", data: "OFF_T_FMT"\n",  Nimage,  Ndata);
    if (!FORCE_READ) exit (1);
    Nimage = Ndata;
  } 

  /* create a dummy ftable, theader set for this data */
  /* (original table has NAXIS = 2, change to 0) */
  /* gfits_modify (&db[0].header, "NAXIS", "%d", 1, 0); */
  gfits_create_matrix (&db[0].header, &db[0].matrix);
  db[0].ftable.header = &db[0].theader;

  if (db[0].format == DVO_FORMAT_INTERNAL)  	  gfits_table_mkheader_Image (&db[0].theader);
  if (db[0].format == DVO_FORMAT_LONEOS)    	  gfits_table_mkheader_Image_Loneos (&db[0].theader);
  if (db[0].format == DVO_FORMAT_ELIXIR)    	  gfits_table_mkheader_Image_Elixir (&db[0].theader);
  if (db[0].format == DVO_FORMAT_PANSTARRS_DEV_0) gfits_table_mkheader_Image_Panstarrs_DEV_0 (&db[0].theader);
  if (db[0].format == DVO_FORMAT_PANSTARRS_DEV_1) gfits_table_mkheader_Image_Panstarrs_DEV_1 (&db[0].theader);
  if (db[0].format == DVO_FORMAT_PS1_DEV_1)       gfits_table_mkheader_Image_PS1_DEV_1 (&db[0].theader);
  if (db[0].format == DVO_FORMAT_PS1_DEV_2)       gfits_table_mkheader_Image_PS1_DEV_2 (&db[0].theader);
  if (db[0].format == DVO_FORMAT_PS1_DEV_3)       gfits_table_mkheader_Image_PS1_DEV_3 (&db[0].theader);
  if (db[0].format == DVO_FORMAT_PS1_V1)          gfits_table_mkheader_Image_PS1_V1 (&db[0].theader);
  if (db[0].format == DVO_FORMAT_PS1_V2)          gfits_table_mkheader_Image_PS1_V2 (&db[0].theader);
  if (db[0].format == DVO_FORMAT_PS1_V3)          gfits_table_mkheader_Image_PS1_V3 (&db[0].theader);
  if (db[0].format == DVO_FORMAT_PS1_V4)          gfits_table_mkheader_Image_PS1_V4 (&db[0].theader);
  if (db[0].format == DVO_FORMAT_PS1_V5)          gfits_table_mkheader_Image_PS1_V5 (&db[0].theader);
  if (db[0].format == DVO_FORMAT_PS1_V6)          gfits_table_mkheader_Image_PS1_V6 (&db[0].theader);
  if (db[0].format == DVO_FORMAT_PS1_V5_LOAD)     gfits_table_mkheader_Image_PS1_V5_LOAD (&db[0].theader);
  if (db[0].format == DVO_FORMAT_PS1_REF)         gfits_table_mkheader_Image_PS1_REF (&db[0].theader);
  if (db[0].format == DVO_FORMAT_PS1_REF_V2)      gfits_table_mkheader_Image_PS1_REF_V2 (&db[0].theader);
  if (db[0].format == DVO_FORMAT_PS1_REF_V3)      gfits_table_mkheader_Image_PS1_REF_V3 (&db[0].theader);
  if (db[0].format == DVO_FORMAT_PS1_SIM)         gfits_table_mkheader_Image_PS1_SIM (&db[0].theader);
    
  /* read data from file */
  Nbytes = ImageSize*Nimage;
  ALLOCATE (db[0].ftable.buffer, char, Nbytes);
  nbytes = fread (db[0].ftable.buffer, 1, Nbytes, db[0].f);
  if (nbytes != Nbytes) {
    if (VERBOSE) fprintf (stderr, "ERROR: problem loading image catalog\n");
    exit (1);
  } 

  gfits_modify (&db[0].theader, "NAXIS2", OFF_T_FMT, 1,  Nimage);
  db[0].theader.Naxis[1] = Nimage;
  db[0].ftable.datasize = gfits_data_size (&db[0].theader);

  db[0].nativeOrder = FALSE;  /* table does not have internal byte-order */
  db[0].scaledValue = FALSE;  /* table does not have internal byte-order */
  return (TRUE);
}

/* write out image db elements from vtable */
int dvo_image_update_raw (FITS_DB *db, int VERBOSE) {

  off_t i, Nx, Ny, Nrow, Nbytes, start, offset;
  off_t status;
  off_t *row;

  if (VERBOSE) fprintf (stderr, "writing out "OFF_T_FMT" images\n",  db[0].vtable.Nrow);

  /* position to start of file */
  fseeko (db[0].f, 0, SEEK_SET);
  
  /* write out complete header (no check on disk size?) */
  status = fwrite (db[0].header.buffer, 1, db[0].header.datasize, db[0].f);
  if (status != db[0].header.datasize) {
    fprintf (stderr, "ERROR: failed writing data to image header\n");
    exit (1);
  }

  /** this code is identical to gfits_fwrite_vtable, except without padding */
  Nrow = db[0].vtable.Nrow;
  row = db[0].vtable.row;
  gfits_scan (db[0].vtable.header, "NAXIS1", OFF_T_FMT, 1,  &Nx);
  gfits_scan (db[0].vtable.header, "NAXIS2", OFF_T_FMT, 1,  &Ny);

  /* file pointer is at beginning of desired table data */
  start = ftell (db[0].f);
  
  for (i = 0; i < Nrow; i++) {
    offset = start + Nx*row[i];
    fseeko (db[0].f, offset, SEEK_SET);
    Nbytes = fwrite (db[0].vtable.buffer[i], sizeof (char), Nx, db[0].f);
    if (Nbytes != Nx) { return (FALSE); }
  }
  return (TRUE);
}

/* write out complete image db table from ftable */
int dvo_image_save_raw (FITS_DB *db, int VERBOSE) {

  off_t Nx, Ny, size, Nbytes;
  int status;

  if (VERBOSE) fprintf (stderr, "writing out "OFF_T_FMT" images\n",  db[0].theader.Naxis[1]);

  /* position to start of file */
  Fseek (db[0].f, 0, SEEK_SET);
  
  /* write out complete header (no check on disk size?) */
  status = fwrite (db[0].header.buffer, 1, db[0].header.datasize, db[0].f);
  if (status != db[0].header.datasize) {
    fprintf (stderr, "ERROR: failed writing data to image header\n");
    exit (1);
  }

  gfits_scan (db[0].ftable.header, "NAXIS1", OFF_T_FMT, 1,  &Nx);
  gfits_scan (db[0].ftable.header, "NAXIS2", OFF_T_FMT, 1,  &Ny);
  size = Nx * Ny;
  Nbytes = fwrite (db[0].ftable.buffer, sizeof(char), size, db[0].f);
  if (Nbytes != size) return (FALSE);
  return (TRUE);
}

# include "setphot.h"

ZptTable *load_zpt_table (char *filename, int *nzpts) {

  int Nzpts, NZPTS;
  ZptTable *zpts;
  double zpt, mjd, zpt_err;

  FILE *f;

  f = fopen (filename, "r");
  if (!f) {
    fprintf (stderr, "ERROR: cannot open zpt table file %s\n", filename);
    exit (1);
  }

  Nzpts = 0;
  NZPTS = 100;
  ALLOCATE (zpts, ZptTable, NZPTS);

  // format is fixed: (time) (zpt) (zpt_err)
  int status;
  while ((status = fscanf (f, "%lf %lf %lf", &mjd, &zpt, &zpt_err)) == 3) {
    zpts[Nzpts].zpt = zpt;
    zpts[Nzpts].zpt_err = zpt_err;
    zpts[Nzpts].time = ohana_mjd_to_sec (mjd);
    zpts[Nzpts].found = FALSE;

    Nzpts ++;
    CHECK_REALLOCATE (zpts, ZptTable, NZPTS, Nzpts, 100);
  }

  if (status != EOF) {
    fprintf (stderr, "unexpected formatting on line %d\n", Nzpts);
    exit (2);
  }

  fprintf (stderr, "loaded %d zero points\n", Nzpts);

  *nzpts = Nzpts;
  return zpts;
}

/* Slightly more generic loader than the ubercal version.  Still assumes Nfilters x Nseasons
   the input file must contain the following:
   PHU Header : NFILTER, NSEASON, NCHIP_X, NCHIP_Y, NCELL_X, NCELL_Y, TS0_nnnn (season nnnn start mjd), TS1_nnnn (season nnnn end mjd)
   
   NSEASON * 2 extensions with
   TABLE : mjd, zpt; Header: FILTER
   IMAGE : 1D array of flat offsets
*/

ZptTable *load_zpt_ubercal(char *filename, int *nzpts, FlatCorrectionTable *flatcorrTable) {

  int i, nfilter, nseason, ix, iy, ixc, iyc, Ncol;
  off_t Nrow;
  char type[16], filter[80];
  int Nzpts, NZPTS;
  ZptTable *zpts;
  Header header;
  Header theader;
  Matrix matrix;
  FTable ftable;

  // parameters describing the flat-field correction
  int NSEASON;
  int NFILTER;
  int NCHIP_X;
  int NCHIP_Y;
  int NCELL_X;
  int NCELL_Y;
  int CHIP_DX;
  int CHIP_DY;

  *nzpts = 0;

  FILE *f = fopen (filename, "r");
  if (!f) {
    fprintf (stderr, "ERROR: cannot open zpt table file %s\n", filename);
    exit (1);
  }

  /* load in PHU segment (ignore) */
  if (!gfits_fread_header (f, &header)) {
    if (VERBOSE) fprintf (stderr, "can't read Flat Correction header\n");
    fclose (f);
    return (NULL);
  }
  if (!gfits_fread_matrix (f, &matrix, &header)) {
    if (VERBOSE) fprintf (stderr, "can't read Flat Correction matrix\n");
    gfits_free_header (&header);
    fclose (f);
    return (NULL);
  }

  Nzpts = 0;
  NZPTS = 0;
  ALLOCATE (zpts, ZptTable, NZPTS);

  // this function would be better if we read the list of filters, seasons, and the dimensions from the header
  // for current testing, make fake smfs that correspond to specific chips, filter, and mjd ranges?

  // the simple files from Eddie have no internal metadata describing the corrections,
  // so they must be manually encoded

  // hard-wired values which describe the ubercal analysis
  if (NO_METADATA) {
    NFILTER = 5;
    NSEASON = 5;
    NCHIP_X = 8;
    NCHIP_Y = 8;
    NCELL_X = 2;
    NCELL_Y = 2;
    CHIP_DX = 2*2424;  // != 4880
    CHIP_DY = 2*2430;  // != 4864
    // Note that Eddie has identified the center of the boundary between cells xy3n and
    // xy4n and used that to split the chip.  I am setting CHIP_DX,DY to force the same split
  } else {
    if (!gfits_scan (&header, "NSEASON", "%d", 1, &NSEASON)) { 
      fprintf (stderr, "cannot find NSEASON in header of %s\n", filename);
      fclose (f);
      return NULL;
    }
    if (!gfits_scan (&header, "NFILTER", "%d", 1, &NFILTER)) { 
      fprintf (stderr, "cannot find NFILTER in header of %s\n", filename);
      fclose (f);
      return NULL;
    }
    if (!gfits_scan (&header, "NCHIP_X", "%d", 1, &NCHIP_X)) { 
      fprintf (stderr, "cannot find NCHIP_X in header of %s\n", filename);
      fclose (f);
      return NULL;
    }
    if (!gfits_scan (&header, "NCHIP_Y", "%d", 1, &NCHIP_Y)) { 
      fprintf (stderr, "cannot find NCHIP_Y in header of %s\n", filename);
      fclose (f);
      return NULL;
    }
    if (!gfits_scan (&header, "NCELL_X", "%d", 1, &NCELL_X)) { 
      fprintf (stderr, "cannot find NCELL_X in header of %s\n", filename);
      fclose (f);
      return NULL;
    }
    if (!gfits_scan (&header, "NCELL_Y", "%d", 1, &NCELL_Y)) { 
      fprintf (stderr, "cannot find NCELL_Y in header of %s\n", filename);
      fclose (f);
      return NULL;
    }
    if (!gfits_scan (&header, "CHIP_DX", "%d", 1, &CHIP_DX)) { 
      fprintf (stderr, "cannot find CHIP_DX in header of %s\n", filename);
      fclose (f);
      return NULL;
    }
    if (!gfits_scan (&header, "CHIP_DY", "%d", 1, &CHIP_DY)) { 
      fprintf (stderr, "cannot find CHIP_DY in header of %s\n", filename);
      fclose (f);
      return NULL;
    }
  }

  flatcorrTable->Nseason = NSEASON;
  ALLOCATE (flatcorrTable->tstart, e_time, NSEASON);
  ALLOCATE (flatcorrTable->tstop,  e_time, NSEASON);

  // ubercal hard-coded values
  char filters_uc[5][3] = {"g", "r", "i", "z", "y"};

  // double tstart_uc[] = {55000.0, 55296.0, 55327.0, 55662.0};
  // double tstop_uc[]  = {55296.0, 55327.0, 55662.0, 60000.0};

  double tstart_uc[] = {50000.0, 55296.0, 55327.0, 55662.0, 56110.0};
  double tstop_uc[]  = {55296.0, 55327.0, 55662.0, 56110.0, 65000.0};

  double mjdstart, mjdstop;
  for (i = 0; i < NSEASON; i++) {
    if (NO_METADATA) {
      mjdstart = tstart_uc[i];
      mjdstop  = tstop_uc[i];
    } else {
      char name[9];
      // snprintf (name, 9, "TS0_%04d", i);
      snprintf_nowarn (name, 9, "S0_MJD%d", i);
      if (!gfits_scan (&header, name, "%lf", 1, &mjdstart)) { 
	fprintf (stderr, "cannot find %s in header of %s\n", name, filename);
	fclose (f);
	return NULL;
      }

      // snprintf (name, 9, "TS1_%04d", i);
      snprintf_nowarn (name, 9, "S1_MJD%d", i);
      if (!gfits_scan (&header, name, "%lf", 1, &mjdstop)) { 
	fprintf (stderr, "cannot find %s in header of %s\n", name, filename);
	fclose (f);
	return NULL;
      }
    }
    flatcorrTable->tstart[i] = (mjdstart > 0) ? ohana_mjd_to_sec(mjdstart) : ohana_mjd_to_sec(50000.0); // 1995/10/10 (ancient history)
    flatcorrTable->tstop[i]  = (mjdstop  > 0) ? ohana_mjd_to_sec(mjdstop)  : ohana_mjd_to_sec(60000.0); // 2023/02/25 (infinite future)
  }

  // we have 5 filters, and 4 flat-field correction sets for each
  flatcorrTable->Ncorr = NFILTER*NSEASON*NCHIP_X*NCHIP_Y*NCELL_X*NCELL_Y;
  flatcorrTable->Nimage = NFILTER*NSEASON*NCHIP_X*NCHIP_Y;

  ALLOCATE (flatcorrTable->corr, FlatCorrection, flatcorrTable->Ncorr);
  ALLOCATE (flatcorrTable->image, FlatCorrectionImage, flatcorrTable->Nimage);
  memset (flatcorrTable->corr, 0, flatcorrTable->Ncorr*sizeof(FlatCorrection));

  int corrID = 1;
  int Nimage = 0;
  ftable.header = &theader;
  for (nfilter = 0; nfilter < NFILTER; nfilter++) {
    // *** load the ZERO POINT table ***

    // load data for this header 
    if (!gfits_load_header (f, &theader)) return (NULL);

    // read the fits table bytes
    if (!gfits_fread_ftable_data (f, &ftable, FALSE)) return (NULL);
  
    // skip over remaining bytes in data segment
    fseeko (f, ftable.datasize - ftable.validsize, SEEK_CUR);

    // need to create and assign to flat-field correction
    double *mjd = gfits_get_bintable_column_data (&theader, &ftable, "mjd_obs", type, &Nrow, &Ncol);
    assert (!strcmp(type, "double"));

    double *zp = gfits_get_bintable_column_data (&theader, &ftable, "zp", type, &Nrow, &Ncol);
    assert (!strcmp(type, "double"));
      
    // float *zperr = gfits_get_bintable_column_data (&theader, &ftable, "resid", type, &Nrow, &Ncol);
    // assert (!strcmp(type, "float"));
      
    NZPTS += Nrow;
    REALLOCATE (zpts, ZptTable, NZPTS);
    for (i = 0; i < Nrow; i++) {
      zpts[i+Nzpts].time = ohana_mjd_to_sec(mjd[i]);
      zpts[i+Nzpts].zpt = zp[i];
      zpts[i+Nzpts].zpt_err = 0.0;
      zpts[i+Nzpts].found = FALSE;
    }
    Nzpts += Nrow;

    // *** load the flat-field correction image ***

    // the image contains the flat-field corrections for a specific filter

    // load data for this header 
    if (!gfits_load_header (f, &header)) return (NULL);

    if (NO_METADATA) {
      strcpy (filter, filters_uc[nfilter]);
    } else {
      if (!gfits_scan (&header, "FILTER", "%s", 1, filter)) { 
	fprintf (stderr, "warning: cannot find FILTER in header of %s, using %s\n", filename, filters_uc[nfilter]);
	strcpy (filter, filters_uc[nfilter]);
	// fclose (f);
	// return NULL;
      }
    }
    
    // read the fits table bytes
    double *offset64 = NULL;
    float *offset32 = NULL;
    int use32 = TRUE;
    
    if (!gfits_fread_matrix (f, &matrix, &header)) return (NULL);
    switch (header.bitpix) {
      case -32: // float
	offset32 = (float *) matrix.buffer;
	use32 = TRUE;
	break;
      case -64: // double
	offset64 = (double *) matrix.buffer;
	use32 = FALSE;
	break;
      default:
	fprintf (stderr, "invalid bitpix for flat-field correction image: %d\n", header.bitpix);
	exit (2);
    }

    // in some versions of ucal, we have extra FITS extensions (for masked chips)
    // since Eddie gives no (or almost no) metadata, we have to be told if these exist:

    if (SKIP_EXTRA_EXTENSIONS) {
      // ***** skip the extra table segment *****
      // load data for this header 
      if (!gfits_load_header (f, &theader)) return (NULL);

      // read the fits table bytes
      if (!gfits_fread_ftable_data (f, &ftable, FALSE)) return (NULL);
  
      // skip over remaining bytes in data segment
      fseeko (f, ftable.datasize - ftable.validsize, SEEK_CUR);
    }

    // XXX the initial hacked-together table from Eddie is missing the last 2 elements.  they should be zero
    if (NO_METADATA) {
      matrix.Naxis[0] += 2;
      REALLOCATE (offset64, double, matrix.Naxis[0]);
      matrix.buffer = (char *) offset64;
    }
    if (matrix.Naxes == 1) {
      int Ntotal = NSEASON*NCHIP_X*NCHIP_Y*NCELL_X*NCELL_Y;
      if (matrix.Naxis[0] != Ntotal) {
	int Nextra = Ntotal - matrix.Naxis[0];
	assert (Nextra >= 0);
	fprintf (stderr, "warning: images are not fully populated, extending by %d\n", Nextra);
	
	REALLOCATE (offset64, double, Ntotal);
	int npix;
	for (npix = matrix.Naxis[0]; npix < Ntotal; npix ++) offset64[npix] = 0.0;

	matrix.Naxis[0] = Ntotal;
	matrix.buffer = (char *) offset64;
      }
    }

    for (nseason = 0; nseason < NSEASON; nseason++) { // seasons
      for (iy = 0; iy < NCHIP_Y; iy++) { // y-chip
	for (ix = 0; ix < NCHIP_X; ix++) { // x-chip

	  // photcode name
	  char photname[64];
	  snprintf_nowarn (photname, 64, "GPC1.%s.XY%d%d", filter, ix, iy);
	  // note that the XY00, XY07, etc, chips will have photcode values of 0

	  flatcorrTable->image[Nimage].photcode = GetPhotcodeCodebyName(photname);
	  flatcorrTable->image[Nimage].Nx = NCELL_X;
	  flatcorrTable->image[Nimage].Ny = NCELL_Y;
	  flatcorrTable->image[Nimage].ID = corrID;
	  flatcorrTable->image[Nimage].DX = CHIP_DX;
	  flatcorrTable->image[Nimage].DY = CHIP_DY;
	  flatcorrTable->image[Nimage].tstart = flatcorrTable->tstart[nseason];
	  flatcorrTable->image[Nimage].tstop  = flatcorrTable->tstop[nseason];
	  
	  int seq_full = -1; // sequence number within the full table (all filters concatenated together)
	  int seq_filt = -1; // sequence number for just this filter

	  // XXX we should have a NCHIP_X * NCHIP_Y array of x and y parity values

	  // This enforces a 180 chip rotation for XY3n - XY7n & is only known to be valid for GPC1 (the XYnn names as well)
	  for (iyc = 0; iyc < NCELL_Y; iyc++) {
	    for (ixc = 0; ixc < NCELL_X; ixc++) {
	      if (ix < 4) {
		// chips ix < 4 should be flipped
		seq_full = nfilter*NSEASON*NCHIP_X*NCHIP_Y*NCELL_X*NCELL_Y + nseason*NCHIP_X*NCHIP_Y*NCELL_X*NCELL_Y + (iy*NCELL_Y + NCELL_Y - 1 - iyc)*NCHIP_X*NCELL_X + (ix*NCELL_X + NCELL_X - 1 - ixc);
		seq_filt =                                                   nseason*NCHIP_X*NCHIP_Y*NCELL_X*NCELL_Y + (iy*NCELL_Y + NCELL_Y - 1 - iyc)*NCHIP_X*NCELL_X + (ix*NCELL_X + NCELL_X - 1 - ixc);
		// = .... + y_parity*iyc - (y_parity - 1)*(NCELL_Y - 1) / 2
	      } else {
		seq_full = nfilter*NSEASON*NCHIP_X*NCHIP_Y*NCELL_X*NCELL_Y + nseason*NCHIP_X*NCHIP_Y*NCELL_X*NCELL_Y + (iy*NCELL_Y + iyc)*NCHIP_X*NCELL_X + (ix*NCELL_X + ixc);
		seq_filt =                                                   nseason*NCHIP_X*NCHIP_Y*NCELL_X*NCELL_Y + (iy*NCELL_Y + iyc)*NCHIP_X*NCELL_X + (ix*NCELL_X + ixc);
	      }
	      assert (seq_full > -1);
	      assert (seq_filt > -1);
	      assert (seq_full < flatcorrTable->Ncorr);
	      if (matrix.Naxes == 1) {
		assert (seq_filt < matrix.Naxis[0]);
	      } 
	      if (matrix.Naxes == 2) {
		assert (seq_filt < matrix.Naxis[0]*matrix.Naxis[1]);
	      }
	      assert (!flatcorrTable->corr[seq_full].ID);
	      flatcorrTable->corr[seq_full].x = ixc;
	      flatcorrTable->corr[seq_full].y = iyc;
	      if (use32) {
		flatcorrTable->corr[seq_full].offset = offset32[seq_filt];
	      } else {
		flatcorrTable->corr[seq_full].offset = offset64[seq_filt];
	      }
	      flatcorrTable->corr[seq_full].ID = corrID;
	    }
	  }
	  corrID ++;
	  Nimage ++;
	}
      }
    }	      
  }

  /*** convert from corr,image format to offsets ***/
  FlatCorrectionInternal (flatcorrTable);

  fprintf (stderr, "loaded %d zero points\n", Nzpts);

  *nzpts = Nzpts;
  return zpts;
}

static short maxCode = 0;
static short *zpt_codeval = NULL;
static float *zpt_offsets = NULL;
static short *zpt_index = NULL;

int parse_zpt_offsets (char *ZPT_OFFSET_FILTERS, char *ZPT_OFFSET_VALUES) {

  if (!ZPT_OFFSET_FILTERS) return TRUE;
  assert (ZPT_OFFSET_FILTERS);
  assert (ZPT_OFFSET_VALUES);

  char *filters = strcreate(ZPT_OFFSET_FILTERS);
  char *values = strcreate(ZPT_OFFSET_VALUES);

  char *p1 = filters;
  char *p2 = values;

  char *filter = NULL;
  char *value = NULL;

  char *s1 = NULL;
  char *s2 = NULL;

  // save an array of the zero point offsets and their photcodes
  int Ncode = 0;
  int NCODE = 8;
  ALLOCATE (zpt_codeval, short, NCODE);
  ALLOCATE (zpt_offsets, float, NCODE);

  while (1) {
    filter = strtok_r (p1, ",", &s1);
    if (!filter) break;

    value  = strtok_r (p2, ",", &s2);
    if (!value) {
      fprintf (stderr, "ERROR: mismatch between list of photcodes and list of offsets\n");
      exit (1);
    }

    // do something here
    PhotCode *code = GetPhotcodebyName (filter);
    if (!code) {
      fprintf (stderr, "ERROR: unknown photcode %s\n", filter);
      exit (1);
    }
    if (code->type != PHOT_SEC) {
      fprintf (stderr, "ERROR: photcode %s is not an average (SEC) type\n", filter);
      exit (1);
    }

    zpt_codeval[Ncode] = code->code;
    zpt_offsets[Ncode] = atof(value);
    Ncode ++;

    if (Ncode >= NCODE) {
      NCODE += 8;
      REALLOCATE (zpt_codeval, short, NCODE);
      REALLOCATE (zpt_offsets, float, NCODE);
    }
    
    maxCode = MAX(code->code, maxCode);

    p1 = NULL;
    p2 = NULL;
  }
    
  // generate an index to the zpt offsets by the photcode
  ALLOCATE (zpt_index, short, maxCode + 1);

  int i;
  for (i = 0; i < maxCode + 1; i++) {
    zpt_index[i] = -1;
  }

  for (i = 0; i < Ncode; i++) {
    short seq = zpt_codeval[i];
    if (zpt_index[seq] != -1) {
      fprintf (stderr, "duplicate photcode in -zpt-offset list\n");
      exit (2);
    }
    zpt_index[seq] = i;
  }
  return TRUE;
}

float apply_zpt_offset (short code) {

  if (!zpt_offsets) return 0.0;

  // code is a primary photcode (but may be out of range, in which case no correction is supplied)
  if (code <= 0) return 0.0;
  if (code > maxCode) return 0.0;

  short index = zpt_index[code];
  if (index == -1) return 0.0;

  float offset = zpt_offsets[index];
  return offset;
}

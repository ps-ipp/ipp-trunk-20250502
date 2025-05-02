# include "dvoshell.h"

int coordmosaic (int argc, char **argv) {

  int N, ix, iy;
  off_t i, j, nt, Nimage, *subset, Nsubset;
  time_t tzero;
  double trange;
  Buffer *bufX, *bufY;
  SkyRegionSelection *selection;
  Image *image;

  if (!InitPhotcodes ()) return (FALSE);

  // parse skyregion options
  if ((selection = SetRegionSelection (&argc, argv)) == NULL) {
    gprint (GP_ERR, "invalid sky region selection\n");
    return FALSE;
  }

  int TimeSelect = FALSE;
  if ((N = get_argument (argc, argv, "-time"))) {
    remove_argument (N, &argc, argv);
    if (!ohana_str_to_time (argv[N], &tzero)) { 
      gprint (GP_ERR, "syntax error\n");
      return (FALSE);
    }
    remove_argument (N, &argc, argv);
    if (!ohana_str_to_dtime (argv[N], &trange)) { 
      gprint (GP_ERR, "syntax error\n");
      return (FALSE);
    }
    remove_argument (N, &argc, argv);
    TimeSelect = TRUE;
    gprint (GP_ERR, "plotting in range %ds - %ds (%f seconds)\n", (int)tzero, (int)(tzero + trange), trange);
  }

  /* 
  PhotCode *Photcode = NULL;
  if ((N = get_argument (argc, argv, "-photcode"))) {
    remove_argument (N, &argc, argv);
    Photcode = GetPhotcodebyName (argv[N]);
    if (Photcode == NULL) {
      gprint (GP_ERR, "photcode not found in photcode table\n");
      return (FALSE);
    }
    remove_argument (N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "-Nphotcode"))) {
    remove_argument (N, &argc, argv);
    Photcode = GetPhotcodebyCode (atoi(argv[N]));
    if (Photcode == NULL) {
      gprint (GP_ERR, "photcode not found in photcode table\n");
      return (FALSE);
    }
    remove_argument (N, &argc, argv);
  }
  */

  char *Name = NULL;
  if ((N = get_argument (argc, argv, "-name"))) {
    remove_argument (N, &argc, argv);
    Name = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }

  /* 
  int chipID = -1;
  if ((N = get_argument (argc, argv, "-chip"))) {
    remove_argument (N, &argc, argv);
    chipID = atoi (argv[N]);
    remove_argument (N, &argc, argv);
  }
  */

  // save the image plate scales
  Vector *cd1v = NULL, *cd2v = NULL;
  if ((cd1v = SelectVector ("cd1", ANYVECTOR, TRUE)) == NULL) return (FALSE);
  if ((cd2v = SelectVector ("cd2", ANYVECTOR, TRUE)) == NULL) return (FALSE);

  if (argc != 5) goto syntax;

  if ((bufX = SelectBuffer (argv[1], ANYBUFFER, TRUE)) == NULL) goto escape;
  if ((bufY = SelectBuffer (argv[2], ANYBUFFER, TRUE)) == NULL) goto escape;
  int Nx = atoi (argv[3]);
  int Ny = atoi (argv[4]);

  gfits_free_matrix (&bufX[0].matrix);
  gfits_free_header (&bufX[0].header);
  if (!CreateBuffer (bufX, Nx, Ny, -32, 0.0, 1.0)) return FALSE; // initialized to 0.0 here
  strcpy (bufX[0].file, "(empty)");

  gfits_free_matrix (&bufY[0].matrix);
  gfits_free_header (&bufY[0].header);
  if (!CreateBuffer (bufY, Nx, Ny, -32, 0.0, 1.0)) return FALSE; // initialized to 0.0 here
  strcpy (bufY[0].file, "(empty)");

  if ((image = LoadImagesDVO (&Nimage)) == NULL) return (FALSE);
  image_subset (image, Nimage, &subset, &Nsubset, selection, tzero, trange, TimeSelect);

  float *vX = (float *) bufX[0].matrix.buffer;
  float *vY = (float *) bufY[0].matrix.buffer;

  int Nsum = 0;
  int NSUM = 1000;
  opihi_flt *cd1 = NULL;
  opihi_flt *cd2 = NULL;
  ALLOCATE (cd1, opihi_flt, NSUM);
  ALLOCATE (cd2, opihi_flt, NSUM);

  int NX = 0;
  int NY = 0;
  float dX = NAN;
  float dY = NAN;

  // only select the DIS (mosaic) images
  for (j = 0; j < Nsubset; j++) {
    i = subset[j];
    if (Name && (strstr (image[i].name, Name) == (char *) NULL)) continue;

    if (strcmp (&image[i].coords.ctype[4], "-DIS")) continue;

    if (!isfinite(dX)) {
      NX = image[i].NX;
      NY = image[i].NY;
      dX = NX / (float) Nx;
      dY = NY / (float) Ny;
    } else {
      if ((NX != image[i].NX) || (NY != image[i].NY)) {
	// fprintf (stderr, "mosaic size mismatch? %d,%d vs %d,%d\n", NX, NY, image[i].NX, image[i].NY);
      }
    }

    // DIS image pixels range fom -0.5*NX to +0.5*NX
    // output pixels range from 0 to Nx
    // dX = NX / Nx
    // fx = ix * dX - 0.5*NX

    Coords coords = image[i].coords;
    // coords.Npolyterms = 0;

    // the coords have an arbitrary rotation.  I want derotate:
    double T = 1.0 / (image[i].coords.pc1_1*image[i].coords.pc2_2 - image[i].coords.pc2_1*image[i].coords.pc1_2);
    double A10 = +image[i].coords.pc2_2 * T;
    double A01 = -image[i].coords.pc1_2 * T;
    double B10 = -image[i].coords.pc2_1 * T;
    double B01 = +image[i].coords.pc1_1 * T;

    coords.pc1_1 = image[i].coords.pc1_1 * A10 + image[i].coords.pc2_1 * A01;
    coords.pc1_2 = image[i].coords.pc1_2 * A10 + image[i].coords.pc2_2 * A01;
    coords.pc2_1 = image[i].coords.pc1_1 * B10 + image[i].coords.pc2_1 * B01;
    coords.pc2_2 = image[i].coords.pc1_2 * B10 + image[i].coords.pc2_2 * B01;

    if (coords.Npolyterms > 1) {
      for (nt = 0; nt < 3; nt++) {
	coords.polyterms[nt][0] = image[i].coords.polyterms[nt][0] * A10 + image[i].coords.polyterms[nt][1] * A01;
	coords.polyterms[nt][1] = image[i].coords.polyterms[nt][0] * B10 + image[i].coords.polyterms[nt][1] * B01;
      }
    }

    if (coords.Npolyterms > 2) {
      for (nt = 3; nt < 7; nt++) {
	coords.polyterms[nt][0] = image[i].coords.polyterms[nt][0] * A10 + image[i].coords.polyterms[nt][1] * A01;
	coords.polyterms[nt][1] = image[i].coords.polyterms[nt][0] * B10 + image[i].coords.polyterms[nt][1] * B01;
      }
    }

    Coords linear = coords;
    linear.Npolyterms = 0;

    for (iy = 0; iy < Ny; iy++) {
      double fy = iy * dY - 0.5*NY; // coordinate in full image
      for (ix = 0; ix < Nx; ix++) {
	double fx = ix * dX - 0.5*NX; // coordinate in full image

	double Lo, Lm; //, dL;
	double Mo, Mm; //, dM;

	XY_to_LM (&Lo, &Mo, fx, fy, &coords);
	XY_to_LM (&Lm, &Mm, fx, fy, &linear);
	// dL = Lo - Lm;
	// dM = Mo - Mm;

	// vX[ix + iy*Nx] += dL;
	// vY[ix + iy*Nx] += dM;
	vX[ix + iy*Nx] += Lo;
	vY[ix + iy*Nx] += Mo;
      }
    }

    cd1[Nsum] = image[i].coords.cdelt1;
    cd2[Nsum] = image[i].coords.cdelt2;
    Nsum ++;
    if (Nsum == NSUM) {
      NSUM += 1000;
      REALLOCATE (cd1, opihi_flt, NSUM);
      REALLOCATE (cd2, opihi_flt, NSUM);
    }
  }

  free (cd1v->elements.Flt); cd1v->elements.Flt = cd1; cd1v->Nelements = Nsum;
  free (cd2v->elements.Flt); cd2v->elements.Flt = cd2; cd2v->Nelements = Nsum;

  for (iy = 0; iy < Ny; iy++) {
    for (ix = 0; ix < Nx; ix++) {
      vX[ix + iy*Nx] /= (float) Nsum;
      vY[ix + iy*Nx] /= (float) Nsum;
    }
  }

  gfits_modify (&bufX->header, "MOS_NX", "%d", 1, NX);
  gfits_modify (&bufX->header, "MOS_NY", "%d", 1, NY);
  gfits_modify (&bufX->header, "MOS_DX", "%f", 1, dX);
  gfits_modify (&bufX->header, "MOS_DY", "%f", 1, dY);
  gfits_modify (&bufX->header, "MOS_NSUM", "%d", 1, Nsum);

  fprintf (stderr, "Nsum: %d\n", Nsum);
  return TRUE;

 syntax:
  gprint (GP_ERR, "USAGE: coordmosaic buffX buffY Nx Ny\n");
 escape:
  return (FALSE);
}

# undef ESCAPE
# define ESCAPE(MSG) { \
    if (src) fclose (src); \
    if (tgt) fclose (tgt); \
    fprintf (stderr, "%s\n", MSG); return FALSE; }

# define GET_COLUMN(OUT,NAME,TYPE) \
  TYPE *OUT = gfits_get_bintable_column_data (&header, &table, NAME, type, &Nrow, &Ncol); \
  myAssert (!strcmp(type, #TYPE), "wrong column type");

int psastro_model (int argc, char **argv) {

  // I need to write out the full psastro input model based on my new fits

  // astrometry model consists of an empty PHU + 4 extensions:
  // CHIPS : 
  // FP
  // TP
  // SKY

  int Ncol;
  off_t Nrow, Nout;
  char type[16];

  FTable table;
  Header header;
  table.header = &header;

  FILE *src = NULL, *tgt = NULL;

  if (argc != 3) {
    fprintf (stderr, "USAGE: psastro_model (src) (tgt)\n");
    return FALSE;
  }

  // I am going to read in the tables from one file and save them (with updates) in another
  src = fopen (argv[1], "r");
  if (!src) ESCAPE ("failed to open source file");

  tgt = fopen (argv[2], "w");
  if (!tgt) ESCAPE ("failed to open output file");

  // read the PHU 
  if (!gfits_load_header (src, &header)) ESCAPE ("error loading PHU");
  int Nbytes = gfits_data_size (&header);
  if (Nbytes) ESCAPE ("non-zero PHU matrix??");

  // save the PHU
  if (!gfits_save_header (tgt, &header)) ESCAPE ("error saving PHU");
  gfits_free_header (&header);

  { 
    // read the CHIPS 
    if (!gfits_load_header (src, &header)) ESCAPE ("error loading CHIPS header");
    if (!gfits_fread_ftable_data (src, &table, FALSE)) ESCAPE ("error reading table for CHIPS");
  
    // these set the value of Nrow
    GET_COLUMN (SEGMENT , "SEGMENT", char ); int Nsegment = Ncol;
    GET_COLUMN (PARENT  , "PARENT" , char ); 
    GET_COLUMN (MINX    , "MINX"   , float);
    GET_COLUMN (MAXX    , "MAXX"   , float);
    GET_COLUMN (MINY    , "MINY"   , float);
    GET_COLUMN (MAXY    , "MAXY"   , float);
    GET_COLUMN (XORDER  , "XORDER" , int  );
    GET_COLUMN (YORDER  , "YORDER" , int  );
    GET_COLUMN (NXORDER , "NXORDER", int  );
    GET_COLUMN (NYORDER , "NYORDER", int  );
    GET_COLUMN (POLY_X  , "POLY_X" , float);
    GET_COLUMN (POLY_Y  , "POLY_Y" , float);
    GET_COLUMN (ERROR_X , "ERROR_X", float);
    GET_COLUMN (ERROR_Y , "ERROR_Y", float);
    GET_COLUMN (MASK_X  , "MASK_X" , char  );
    GET_COLUMN (MASK_Y  , "MASK_Y" , char  );

    // I am keeping the same number of entries, but changing the values of
    // POLY_X, POLY_Y
    int i;
    for (i = 0; i < Nrow; i++) {
      // validate that NX, NY orders are 1
      assert (NXORDER[i] == 1);
      assert (NYORDER[i] == 1);

      char name[16], segment[16];

      int found, xmask, ymask;
      
      memset (segment, 0, 16); memcpy  (segment, &SEGMENT[i*Nsegment], 4);
      snprintf_nowarn (name, 16, "%s_L_CX%dY%d", segment, XORDER[i], YORDER[i]);
      double xvalue = get_double_variable (name, &found);
      xmask = !found;
      
      memset (segment, 0, 16); memcpy  (segment, &SEGMENT[i*Nsegment], 4);
      snprintf_nowarn (name, 16, "%s_M_CX%dY%d", segment, XORDER[i], YORDER[i]);
      double yvalue = get_double_variable (name, &found);
      ymask = !found;

      POLY_X[i] = xvalue;
      POLY_Y[i] = yvalue;
      ERROR_X[i] = 0.0;
      ERROR_Y[i] = 0.0;
      MASK_X[i] = xmask;
      MASK_Y[i] = ymask;
    }

    // reset the table size to 0 rows -- this forces the data array to be reallocate by gfits_set_bintable_column
    gfits_modify (&header, "NAXIS2", "%d", 1, 0); 

    gfits_set_bintable_column (&header, &table, "SEGMENT",  SEGMENT , Nrow);
    gfits_set_bintable_column (&header, &table, "PARENT" ,  PARENT  , Nrow);
    gfits_set_bintable_column (&header, &table, "MINX"   ,  MINX    , Nrow);
    gfits_set_bintable_column (&header, &table, "MAXX"   ,  MAXX    , Nrow);
    gfits_set_bintable_column (&header, &table, "MINY"   ,  MINY    , Nrow);
    gfits_set_bintable_column (&header, &table, "MAXY"   ,  MAXY    , Nrow);
    gfits_set_bintable_column (&header, &table, "XORDER" ,  XORDER  , Nrow);
    gfits_set_bintable_column (&header, &table, "YORDER" ,  YORDER  , Nrow);
    gfits_set_bintable_column (&header, &table, "NXORDER",  NXORDER , Nrow);
    gfits_set_bintable_column (&header, &table, "NYORDER",  NYORDER , Nrow);
    gfits_set_bintable_column (&header, &table, "POLY_X" ,  POLY_X  , Nrow);
    gfits_set_bintable_column (&header, &table, "POLY_Y" ,  POLY_Y  , Nrow);
    gfits_set_bintable_column (&header, &table, "ERROR_X",  ERROR_X , Nrow);
    gfits_set_bintable_column (&header, &table, "ERROR_Y",  ERROR_Y , Nrow);
    gfits_set_bintable_column (&header, &table, "MASK_X" ,  MASK_X  , Nrow);
    gfits_set_bintable_column (&header, &table, "MASK_Y" ,  MASK_Y  , Nrow);

    free (SEGMENT );
    free (PARENT  );
    free (MINX    );
    free (MAXX    );
    free (MINY    );
    free (MAXY    );
    free (XORDER  );
    free (YORDER  );
    free (NXORDER );
    free (NYORDER );
    free (POLY_X  );
    free (POLY_Y  );
    free (ERROR_X );
    free (ERROR_Y );
    free (MASK_X  );
    free (MASK_Y  );

    // save the CHIPS 
    gfits_fwrite_Theader (tgt, &header);
    gfits_fwrite_table  (tgt, &table);
    gfits_free_header (&header);
    gfits_free_table (&table);
  }  

  { 
    // read the FP
    if (!gfits_load_header (src, &header)) ESCAPE ("error loading FP header");
    if (!gfits_fread_ftable_data (src, &table, FALSE)) ESCAPE ("error reading table for FP");
  
    // these set the value of Nrow
    GET_COLUMN (SEGMENT , "SEGMENT", char ); int Nsegment = Ncol;
    GET_COLUMN (PARENT  , "PARENT" , char ); int Nparent = Ncol;
    GET_COLUMN (MINX    , "MINX"   , float);
    GET_COLUMN (MAXX    , "MAXX"   , float);
    GET_COLUMN (MINY    , "MINY"   , float);
    GET_COLUMN (MAXY    , "MAXY"   , float);
    GET_COLUMN (XORDER  , "XORDER" , int  );
    GET_COLUMN (YORDER  , "YORDER" , int  );
    GET_COLUMN (NXORDER , "NXORDER", int  );
    GET_COLUMN (NYORDER , "NYORDER", int  );
    GET_COLUMN (POLY_X  , "POLY_X" , float);
    GET_COLUMN (POLY_Y  , "POLY_Y" , float);
    GET_COLUMN (ERROR_X , "ERROR_X", float);
    GET_COLUMN (ERROR_Y , "ERROR_Y", float);
    GET_COLUMN (MASK_X  , "MASK_X" , char  );
    GET_COLUMN (MASK_Y  , "MASK_Y" , char  );

    Nout = 16;

    REALLOCATE (SEGMENT , char , Nout*Nsegment);
    REALLOCATE (PARENT  , char , Nout*Nparent);
    REALLOCATE (MINX    , float, Nout);
    REALLOCATE (MAXX    , float, Nout);
    REALLOCATE (MINY    , float, Nout);
    REALLOCATE (MAXY    , float, Nout);
    REALLOCATE (XORDER  , int  , Nout);
    REALLOCATE (YORDER  , int  , Nout);
    REALLOCATE (NXORDER , int  , Nout);
    REALLOCATE (NYORDER , int  , Nout);
    REALLOCATE (POLY_X  , float, Nout);
    REALLOCATE (POLY_Y  , float, Nout);
    REALLOCATE (ERROR_X , float, Nout);
    REALLOCATE (ERROR_Y , float, Nout);
    REALLOCATE (MASK_X  , char , Nout);
    REALLOCATE (MASK_Y  , char , Nout);

    float minX = MINX[0];
    float maxX = MAXX[0];
    float minY = MINY[0];
    float maxY = MAXY[0];

    int ix, iy, nrow;
    nrow = 0;
    for (ix = 0; ix < 4; ix++) {
      for (iy = 0; iy < 4; iy++) {
	memcpy (&SEGMENT[nrow*Nsegment], "FOCAL_PLANE", strlen("FOCAL_PLANE"));
	memcpy (&PARENT[nrow*Nparent], "TANGENT_PLANE", strlen("TANGENT_PLANE"));
	MINX[nrow] = minX;
	MAXX[nrow] = maxX;
	MINY[nrow] = minY;
	MAXY[nrow] = maxY;
	XORDER[nrow] = ix;
	YORDER[nrow] = iy;
	NXORDER[nrow] = 3;
	NYORDER[nrow] = 3;

	char name[16];

	int found, xmask, ymask;
	snprintf_nowarn (name, 16, "L_X%dY%d", ix, iy);
	double xvalue = get_double_variable (name, &found);
	xmask = !found;

	snprintf_nowarn (name, 16, "M_X%dY%d", ix, iy);
	double yvalue = get_double_variable (name, &found);
	ymask = !found;

	POLY_X[nrow] = xvalue;
	POLY_Y[nrow] = yvalue;
	ERROR_X[nrow] = 0.0;
	ERROR_Y[nrow] = 0.0;
	MASK_X[nrow] = xmask;
	MASK_Y[nrow] = ymask;
	nrow ++;
      }
    }

    // reset the table size to 0 rows -- this forces the data array to be reallocate by gfits_set_bintable_column
    gfits_modify (&header, "NAXIS2", "%d", 1, 0); 

    gfits_set_bintable_column (&header, &table, "SEGMENT",  SEGMENT , Nout);
    gfits_set_bintable_column (&header, &table, "PARENT" ,  PARENT  , Nout);
    gfits_set_bintable_column (&header, &table, "MINX"   ,  MINX    , Nout);
    gfits_set_bintable_column (&header, &table, "MAXX"   ,  MAXX    , Nout);
    gfits_set_bintable_column (&header, &table, "MINY"   ,  MINY    , Nout);
    gfits_set_bintable_column (&header, &table, "MAXY"   ,  MAXY    , Nout);
    gfits_set_bintable_column (&header, &table, "XORDER" ,  XORDER  , Nout);
    gfits_set_bintable_column (&header, &table, "YORDER" ,  YORDER  , Nout);
    gfits_set_bintable_column (&header, &table, "NXORDER",  NXORDER , Nout);
    gfits_set_bintable_column (&header, &table, "NYORDER",  NYORDER , Nout);
    gfits_set_bintable_column (&header, &table, "POLY_X" ,  POLY_X  , Nout);
    gfits_set_bintable_column (&header, &table, "POLY_Y" ,  POLY_Y  , Nout);
    gfits_set_bintable_column (&header, &table, "ERROR_X",  ERROR_X , Nout);
    gfits_set_bintable_column (&header, &table, "ERROR_Y",  ERROR_Y , Nout);
    gfits_set_bintable_column (&header, &table, "MASK_X" ,  MASK_X  , Nout);
    gfits_set_bintable_column (&header, &table, "MASK_Y" ,  MASK_Y  , Nout);

    free (SEGMENT );
    free (PARENT  );
    free (MINX    );
    free (MAXX    );
    free (MINY    );
    free (MAXY    );
    free (XORDER  );
    free (YORDER  );
    free (NXORDER );
    free (NYORDER );
    free (POLY_X  );
    free (POLY_Y  );
    free (ERROR_X );
    free (ERROR_Y );
    free (MASK_X  );
    free (MASK_Y  );

    // save the FP 
    gfits_fwrite_Theader (tgt, &header);
    gfits_fwrite_table  (tgt, &table);
    gfits_free_header (&header);
    gfits_free_table (&table);
  }
  
  // read the TP
  if (!gfits_load_header (src, &header)) ESCAPE ("error loading TP header");
  if (!gfits_fread_ftable_data (src, &table, FALSE)) ESCAPE ("error reading table for TP");
  
  // save the TP 
  gfits_fwrite_Theader (tgt, &header);
  gfits_fwrite_table  (tgt, &table);
  gfits_free_header (&header);
  gfits_free_table (&table);
  
  // read the SKY
  if (!gfits_load_header (src, &header)) ESCAPE ("error loading SKY header");
  if (!gfits_fread_ftable_data (src, &table, FALSE)) ESCAPE ("error reading table for SKY");
  
  // save the SKY 
  gfits_fwrite_Theader (tgt, &header);
  gfits_fwrite_table  (tgt, &table);
  gfits_free_header (&header);
  gfits_free_table (&table);
  
  fclose (tgt);
  fclose (src);

  return TRUE;
}

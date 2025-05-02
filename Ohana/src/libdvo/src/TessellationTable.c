# include "dvo.h"

# define GET_COLUMN_NEW(OUT,NAME,TYPE)					\
  TYPE *OUT = gfits_get_bintable_column_data (&theader, &ftable, NAME, type, &Nrow, &Ncol); \
  myAssert (!strcmp(type, #TYPE), "wrong column type");

# define GET_COLUMN_RAW(OUT,NAME,TYPE)					\
  OUT = gfits_get_bintable_column_data (&theader, &ftable, NAME, type, &Nrow, &Ncol); \
  myAssert (!strcmp(type, #TYPE), "wrong column type");

# define DEBUG 0

void TessellationTableInit (TessellationTable *tess, int Ntess) {

  int i;

  for (i = 0; i < Ntess; i++) {
    tess[i].Rmin = NAN;
    tess[i].Rmax = NAN;
    tess[i].Dmin = NAN;
    tess[i].Dmax = NAN;

    tess[i].Xo = NAN;
    tess[i].Yo = NAN;
    tess[i].Ro = NAN;
    tess[i].Do = NAN;

    tess[i].dPix = NAN;

    tess[i].dX = NAN;
    tess[i].dY = NAN;

    tess[i].NX_SUB = NAN;
    tess[i].NY_SUB = NAN;

    tess[i].basename = NULL;
    tess[i].Nbasename = 0;

    tess[i].projectIDoff = 0;
    tess[i].skycellIDoff = 0;

    tess[i].type = TESS_NONE;
    tess[i].tree = NULL;
  }
  return;
}

void TessellationTableFree (TessellationTable *tess, int Ntess) {
  if (!tess) return;

  for (int i = 0; i < Ntess; i++) {
    FREE (tess[i].basename);
    BoundaryTreeFree(tess[i].tree);
    FREE (tess[i].tree);
  }
}

// backwards compatible load : if the first table is a BoundaryTree, not a TessellationTable, just load that.
TessellationTable *TessellationTableLoad(char *filename, int *Ntess) {

  int i, Ncol;
  off_t Nrow;
  char type[16];
  Header header;
  Header theader;
  Matrix matrix;
  FTable ftable;

  header.buffer = NULL;
  matrix.buffer = NULL;
  ftable.buffer = NULL;
  theader.buffer = NULL;
  TessellationTable *tess = NULL;
  *Ntess = 0;

  FILE *f = fopen (filename, "r");
  if (!f) {
    fprintf (stderr, "ERROR: cannot open image subset file %s\n", filename);
    return NULL;
  }

  /* load in PHU segment (ignore) */
  if (!gfits_fread_header (f, &header)) {
    if (DEBUG) fprintf (stderr, "can't read image subset header\n");
    goto escape;
  }
  if (!gfits_fread_matrix (f, &matrix, &header)) {
    if (DEBUG) fprintf (stderr, "can't read image subset matrix\n");
    goto escape;
  }

  ftable.header = &theader;

  // load data for this header 
  if (!gfits_load_header (f, &theader)) goto escape;

  // what kind of table have I just read (if any!)
  char extname[64];
  gfits_scan (&theader, "EXTNAME", "%s", 1, extname);

  // is this a BoundaryTree?  if so, it must be stand-alone: load just that and generate a
  // containing TessellationTable.
  if (!strcmp(extname, "ZONE_DATA")) {
    BoundaryTree *tree = BoundaryTreeRead (&header, &theader, f);
    gfits_free_header (&theader);
    gfits_free_header (&header);
    gfits_free_matrix (&matrix);
    fclose (f);
    
    ALLOCATE (tess, TessellationTable, 1);
    TessellationTableInit (tess, 1);
    tess[0].tree = tree;
    tess[0].type = TESS_RINGS;
    tess[0].Rmin =   0;
    tess[0].Rmax = 360;
    tess[0].Dmin = -90;
    tess[0].Dmax = +90;
    *Ntess = 1;
    return tess;
  }
  
  if (strcmp(extname, "TESS_DATA")) {
    fprintf (stderr, "header is neither ZONE_DATA nor TESS_DATA, problem with file\n");
    goto escape;
  }

  // read the fits table bytes
  if (!gfits_fread_ftable_data (f, &ftable, FALSE)) goto escape;
 
  // need to create and assign to flat-field correction
  GET_COLUMN_NEW(Ro,     "RA",     double);
  GET_COLUMN_NEW(Do,     "DEC",    double);
  GET_COLUMN_NEW(Xo,     "X_CENT", double);
  GET_COLUMN_NEW(Yo,     "Y_CENT", double);
  GET_COLUMN_NEW(dX,     "X_GRID", float);
  GET_COLUMN_NEW(dY,     "Y_GRID", float);
  GET_COLUMN_NEW(Rmin,   "R_MIN",  double);
  GET_COLUMN_NEW(Rmax,   "R_MAX",  double);
  GET_COLUMN_NEW(Dmin,   "D_MIN",  double);
  GET_COLUMN_NEW(Dmax,   "D_MAX",  double);
  GET_COLUMN_NEW(dPix,   "SCALE",  double);
  GET_COLUMN_NEW(NX_SUB, "NX_SUB", float);
  GET_COLUMN_NEW(NY_SUB, "NY_SUB", float);
  GET_COLUMN_NEW(TYPE,   "TYPE",   int);

  GET_COLUMN_NEW(PROJECT_ID_OFF, "PROJECT_ID_OFF", int);
  GET_COLUMN_NEW(SKYCELL_ID_OFF, "SKYCELL_ID_OFF", int);

  GET_COLUMN_NEW(BASENAME, "BASENAME", char);

  *Ntess = Nrow;
  ALLOCATE (tess, TessellationTable, *Ntess);
  for (i = 0; i < *Ntess; i++) {
    tess[i].Ro     = Ro[i]    ;
    tess[i].Do     = Do[i]    ;
    tess[i].Xo     = Xo[i]    ;
    tess[i].Yo     = Yo[i]    ;
    tess[i].dX     = dX[i]    ;
    tess[i].dY     = dY[i]    ;
    tess[i].Rmin   = Rmin[i]  ;
    tess[i].Rmax   = Rmax[i]  ;
    tess[i].Dmin   = Dmin[i]  ;
    tess[i].Dmax   = Dmax[i]  ;
    tess[i].dPix   = dPix[i]  ;
    tess[i].NX_SUB = NX_SUB[i];
    tess[i].NY_SUB = NY_SUB[i];
    tess[i].type   = TYPE[i];

    tess[i].projectIDoff = PROJECT_ID_OFF[i];
    tess[i].skycellIDoff = SKYCELL_ID_OFF[i];

    ALLOCATE (tess[i].basename, char, BOUNDARY_TREE_NAME_LENGTH);
    memcpy (tess[i].basename, &BASENAME[i*BOUNDARY_TREE_NAME_LENGTH], BOUNDARY_TREE_NAME_LENGTH);
    tess[i].Nbasename = strlen (tess[i].basename);

    tess[i].tree   = NULL;
  }

  free(Ro    );
  free(Do    );
  free(Xo    );
  free(Yo    );
  free(dX    );
  free(dY    );
  free(Rmin  );
  free(Rmax  );
  free(Dmin  );
  free(Dmax  );
  free(dPix  );
  free(NX_SUB);
  free(NY_SUB);
  free(TYPE);

  free(PROJECT_ID_OFF);
  free(SKYCELL_ID_OFF);

  free(BASENAME);

  gfits_free_header (&theader);
  gfits_free_table  (&ftable);

  // fprintf (stderr, "loaded data for %lld projection cell\n", (long long) *Ntess);

  /*** check for a RINGS entry ***/

  // find the RINGS tessellation (if present)
  TessellationTable *rings = NULL;
  for (i = 0; i < *Ntess; i++) {
    if (tess[i].type == TESS_LOCAL) continue;
    if (tess[i].type != TESS_RINGS) continue;
    rings = &tess[i];
  }

  if (!rings) {
    fclose (f);
    gfits_free_header (&header);
    gfits_free_matrix (&matrix);
    return (tess);
  }

  // load data for this header, or exit
  if (!gfits_load_header (f, &theader))  {
    fprintf (stderr, "warning : RINGS tessellation listed in table but not present\n");
    gfits_free_header (&header);
    gfits_free_matrix (&matrix);
    return tess;
  }

  // what kind of table have I just read (if any!)
  gfits_scan (&theader, "EXTNAME", "%s", 1, extname);

  if (strcmp(extname, "ZONE_DATA")) {
    fprintf (stderr, "warning : RINGS tessellation listed in table but not present\n");
    gfits_free_header (&theader);
    gfits_free_header (&header);
    gfits_free_matrix (&matrix);
    return tess;
  }

  // is this a BoundaryTree?  if so, it must be stand-alone: load just that and generate a
  // containing TessellationTable.
  BoundaryTree *tree = BoundaryTreeRead (&header, &theader, f);
  gfits_free_header (&theader);
  gfits_free_header (&header);
  gfits_free_matrix (&matrix);
  fclose (f);
    
  rings->tree = tree;
  return tess;

escape:
  gfits_free_header (&header);
  gfits_free_matrix (&matrix);
  gfits_free_header (&theader);
  gfits_free_table  (&ftable);
  if (tess) free (tess);

  fclose (f);
  return NULL;
}

// we are passed a TessellationTable structure, write it to a FITS table.
// if one of the tess entries is a RINGS tessellation, write it as a boundary tree
int TessellationTableSave(char *filename, TessellationTable *tess, int Ntess) {

  int i;
  Header header;
  Matrix matrix;

  gfits_init_header (&header);
  header.extend = TRUE;
  gfits_create_header (&header);
  gfits_create_matrix (&header, &matrix);

  FILE *f = fopen (filename, "w");
  if (!f) {
    fprintf (stderr, "ERROR: cannot open boundary tess file for output %s\n", filename);
    return FALSE;
  }

  // find the RINGS tessellation (if present)
  BoundaryTree *rings = NULL;
  for (i = 0; i < Ntess; i++) {
    if (tess[i].type == TESS_LOCAL) continue;
    if (tess[i].type != TESS_RINGS) continue;
    rings = tess[i].tree;
  }

  // if we have a RINGS entry, we need to save some information in the header to define the layout
  if (rings) {
    gfits_modify (&header, "DEC_ORI", "%lf", 1, rings->DEC_origin);
    gfits_modify (&header, "DEC_OFF", "%lf", 1, rings->DEC_offset);

    gfits_modify (&header, "NX_SUB", "%f", 1, rings->NX_SUB);
    gfits_modify (&header, "NY_SUB", "%f", 1, rings->NY_SUB);
    gfits_modify (&header, "PIXSCALE", "%lf", 1, rings->dPix);
  }

  gfits_fwrite_header  (f, &header);
  gfits_fwrite_matrix  (f, &matrix);
  gfits_free_header (&header);
  gfits_free_matrix (&matrix);

  /** LOCAL tessellation table info **/
  Header theader;
  FTable ftable;

  gfits_create_table_header (&theader, "BINTABLE", "TESS_DATA");

  gfits_define_bintable_column (&theader, "D", "RA",     "ra (J2000) of cell center", "degree", 1.0, 0.0);
  gfits_define_bintable_column (&theader, "D", "DEC",    "dec (J2000) of cell center", "degree", 1.0, 0.0);
  gfits_define_bintable_column (&theader, "D", "X_CENT", "projection cell center pixel", "none", 1.0, 0.0);
  gfits_define_bintable_column (&theader, "D", "Y_CENT", "projection cell center pixel", "none", 1.0, 0.0);
  gfits_define_bintable_column (&theader, "E", "X_GRID", "skycell grid spacing", "none", 1.0, 0.0);
  gfits_define_bintable_column (&theader, "E", "Y_GRID", "skycell grid spacing", "none", 1.0, 0.0);
  gfits_define_bintable_column (&theader, "D", "R_MIN",  "RA limit (lower)", "none", 1.0, 0.0);
  gfits_define_bintable_column (&theader, "D", "R_MAX",  "RA limit (upper)", "none", 1.0, 0.0);
  gfits_define_bintable_column (&theader, "D", "D_MIN",  "DEC limit (lower)", "none", 1.0, 0.0);
  gfits_define_bintable_column (&theader, "D", "D_MAX",  "DEC limit (upper)", "none", 1.0, 0.0);
  gfits_define_bintable_column (&theader, "D", "SCALE",  "pixel scale for projection cell", "none", 1.0, 0.0);
  gfits_define_bintable_column (&theader, "E", "NX_SUB", "skycell subdivision in x", "none", 1.0, 0.0);
  gfits_define_bintable_column (&theader, "E", "NY_SUB", "skycell subdivision in y", "none", 1.0, 0.0);
  gfits_define_bintable_column (&theader, "J", "TYPE",   "type of tessellation", "none", 1.0, 0.0);

  gfits_define_bintable_column (&theader, "J", "PROJECT_ID_OFF", "offset in image name to projection cell ID", "none", 1.0, 0.0);
  gfits_define_bintable_column (&theader, "J", "SKYCELL_ID_OFF", "offset in image name to skycell ID", "none", 1.0, 0.0);

  char fmt[16];
  snprintf (fmt, 16, "%dA", BOUNDARY_TREE_NAME_LENGTH);
  gfits_define_bintable_column (&theader, fmt, "BASENAME", "image base name", "none", 1.0, 0.0);

  // generate the output array that carries the data
  gfits_create_table (&theader, &ftable);
  
  // int Nout = (rings == NULL) ? Ntess : Ntess - 1;
  int Nout = Ntess;

  // create intermediate storage arrays
  // NOTE: we have to unroll the 2D arrays in tess into 1D arrays
  double *Ro            ; ALLOCATE (Ro    ,    double, Nout);
  double *Do            ; ALLOCATE (Do    ,    double, Nout);
  double *Xo            ; ALLOCATE (Xo    ,    double, Nout);
  double *Yo            ; ALLOCATE (Yo    ,    double, Nout);
  float  *dX            ; ALLOCATE (dX    ,    float,  Nout);
  float  *dY            ; ALLOCATE (dY    ,    float,  Nout);
  double *Rmin          ; ALLOCATE (Rmin  ,    double, Nout);
  double *Rmax          ; ALLOCATE (Rmax  ,    double, Nout);
  double *Dmin          ; ALLOCATE (Dmin  ,    double, Nout);
  double *Dmax          ; ALLOCATE (Dmax  ,    double, Nout);
  double *dPix          ; ALLOCATE (dPix  ,    double, Nout);
  float  *NX_SUB        ; ALLOCATE (NX_SUB,    float,  Nout);
  float  *NY_SUB        ; ALLOCATE (NY_SUB,    float,  Nout);
  int    *TYPE          ; ALLOCATE (TYPE,      int,    Nout);

  int    *PROJECT_ID_OFF; ALLOCATE (PROJECT_ID_OFF, int, Nout);
  int    *SKYCELL_ID_OFF; ALLOCATE (SKYCELL_ID_OFF, int, Nout);

  char   *BASENAME      ; ALLOCATE (BASENAME,  char,   Nout*BOUNDARY_TREE_NAME_LENGTH);
  memset (BASENAME, 0, Nout*BOUNDARY_TREE_NAME_LENGTH);

  // assign the storage arrays
  // XXX I was not writing out the RINGS tess entry, but that probably is not useful (drop
  // N vs i in future?)
  int N = 0;
  for (i = 0; i < Ntess; i++) {
    // if (tess->type != TESS_LOCAL) continue;
    Ro[N]     = tess[i].Ro;
    Do[N]     = tess[i].Do;
    Xo[N]     = tess[i].Xo;
    Yo[N]     = tess[i].Yo;
    dX[N]     = tess[i].dX;
    dY[N]     = tess[i].dY;
    Rmin[N]   = tess[i].Rmin;
    Rmax[N]   = tess[i].Rmax;
    Dmin[N]   = tess[i].Dmin;
    Dmax[N]   = tess[i].Dmax;
    dPix[N]   = tess[i].dPix;
    NX_SUB[N] = tess[i].NX_SUB;
    NY_SUB[N] = tess[i].NY_SUB;
    TYPE[N]   = tess[i].type;

    PROJECT_ID_OFF[N] = tess[i].projectIDoff;
    SKYCELL_ID_OFF[N] = tess[i].skycellIDoff;

    if (tess[i].basename) {
      memcpy(&BASENAME[N*BOUNDARY_TREE_NAME_LENGTH], tess[i].basename, BOUNDARY_TREE_NAME_LENGTH);
    }

    N++; 
  }

  // add the columns to the output array
  gfits_set_bintable_column (&theader, &ftable, "RA",     Ro,     Nout);
  gfits_set_bintable_column (&theader, &ftable, "DEC",    Do,     Nout);
  gfits_set_bintable_column (&theader, &ftable, "X_CENT", Xo,     Nout);
  gfits_set_bintable_column (&theader, &ftable, "Y_CENT", Yo,     Nout);
  gfits_set_bintable_column (&theader, &ftable, "X_GRID", dX,     Nout);
  gfits_set_bintable_column (&theader, &ftable, "Y_GRID", dY,     Nout);
  gfits_set_bintable_column (&theader, &ftable, "R_MIN",  Rmin,   Nout);
  gfits_set_bintable_column (&theader, &ftable, "R_MAX",  Rmax,   Nout);
  gfits_set_bintable_column (&theader, &ftable, "D_MIN",  Dmin,   Nout);
  gfits_set_bintable_column (&theader, &ftable, "D_MAX",  Dmax,   Nout);
  gfits_set_bintable_column (&theader, &ftable, "SCALE",  dPix,   Nout);
  gfits_set_bintable_column (&theader, &ftable, "NX_SUB", NX_SUB, Nout);
  gfits_set_bintable_column (&theader, &ftable, "NY_SUB", NY_SUB, Nout);
  gfits_set_bintable_column (&theader, &ftable, "TYPE",   TYPE,   Nout);

  gfits_set_bintable_column (&theader, &ftable, "PROJECT_ID_OFF", PROJECT_ID_OFF, Nout);
  gfits_set_bintable_column (&theader, &ftable, "SKYCELL_ID_OFF", SKYCELL_ID_OFF, Nout);
  gfits_set_bintable_column (&theader, &ftable, "BASENAME", BASENAME, Nout);

  free (Ro    );
  free (Do    );
  free (Xo    );
  free (Yo    );
  free (dX    );
  free (dY    );
  free (Rmin  );
  free (Rmax  );
  free (Dmin  );
  free (Dmax  );
  free (dPix  );
  free (NX_SUB);
  free (NY_SUB);
  free (TYPE);

  free (PROJECT_ID_OFF);
  free (SKYCELL_ID_OFF);
  free (BASENAME);

  gfits_fwrite_Theader (f, &theader);
  gfits_fwrite_table  (f, &ftable);
  gfits_free_header (&theader);
  gfits_free_table (&ftable);

  if (rings) {
    BoundaryTreeWrite(f, rings);
  }

  fclose (f);

  return TRUE;
}

int TessellationLocalProjection (double *x, double *y, double r, double d, TessellationTable *tess) {

    double Xo = tess->Xo;
    double Yo = tess->Yo;
    double Ro = tess->Ro;
    double Do = tess->Do;
    double dPix = tess->dPix;

    // this block only depends on Ro, Do

    double sdp  = sin(RAD_DEG*Do);
    double cdp  = cos(RAD_DEG*Do);
    double salp = sin(RAD_DEG*(r - Ro));
    double calp = cos(RAD_DEG*(r - Ro));
    double sdel = sin(RAD_DEG*d);
    double cdel = cos(RAD_DEG*d);
    
    double stht = sdel*sdp + cdel*cdp*calp;    /* sin(theta) */
    double sphi = cdel*salp;                   /* = cos(theta)*sin(phi) */
    double cphi = cdel*sdp*calp - sdel*cdp;    /* = cos(theta)*cos(phi) */

    // defines the TAN projection (one of zenithal projections available, libdvo/src/coordops.c
    // R = cot (theta) = cos(theta) / sin(theta)
    double L, M;
    if (stht == 0) {
	double Rc = hypot(sphi, cphi);
	L = 180.0 * sphi / Rc;
	M = 180.0 * cphi / Rc;
    } else {
	L = +DEG_RAD * sphi / stht;
	M = -DEG_RAD * cphi / stht;
    }

    // scale, rotation, parity:
    // rotation == 0.0 (pc1_1 == pc2_2 == 1.0, pc1_2 = pc2_1 = 0.0)

    // if there were rotation or parity:
    // Ro = (coords[0].pc1_1*coords[0].pc2_2 - coords[0].pc1_2*coords[0].pc2_1);
    // Xo = (coords[0].pc2_2*L - coords[0].pc1_2*M) / Ro;
    // Yo = (coords[0].pc1_1*M - coords[0].pc2_1*L) / Ro;

    double X = L;
    double Y = M;

    // scale is dPix

    *x = Xo - X / dPix;
    *y = Yo + Y / dPix;
    
    return TRUE;
}

static int Nfail = 0;

// for the given ra,dec : find the valid tessellation and the containing projection / skycell IDs
int TessellationPrimaryCellIDs (TessellationTable *tess, int Ntess, int *tessID, int *projID, int *skycellID, double ra, double dec) {

  int i, zone, band;

  *tessID = -1;
  *projID = -1;
  *skycellID = -1;

  if (!tess) return FALSE;

  // find the tessellation which includes this location
  int myTess = -1;
  for (i = 0; i < Ntess; i++) {
    if (ra  <  tess[i].Rmin) continue;
    if (ra  >= tess[i].Rmax) continue;
    if (dec <  tess[i].Dmin) continue;
    if (dec >= tess[i].Dmax) continue;
    myTess = i;
    break;
  }
  if (myTess < 0) {
    if (Nfail < 100) {
      fprintf (stderr, "no matching tessellation @ %f,%f\n", ra, dec);
    }
    Nfail ++;
    return FALSE;
  }

  if (tess[myTess].type == TESS_NONE) {
    if (Nfail < 100) {
      fprintf (stderr, "tess has invalid type\n");
      Nfail ++;
    }
    return FALSE;
  }

  if (tess[myTess].type == TESS_LOCAL) {
    // I have ra, dec, and the primary projection cell.  In order to choose the primary skycell,
    // I just need to project to ra,dec to X,Y based on the center of the cell and then get the subdivision right.
    
    double x = 0.0;
    double y = 0.0;
    TessellationLocalProjection (&x, &y, ra, dec, &tess[myTess]);
  
    int xi = x / tess[myTess].dX;
    int yi = y / tess[myTess].dY;
    int N = xi + tess[myTess].NX_SUB * yi;
  
    *tessID = myTess;
    *projID = 0;
    *skycellID = N;

    // *projID = tess[myTess].projID :: NOTE : The projection cell may be non-zero like eg
    // STS.  However, I'm not certain we are going to implement it like this.

    return TRUE;
  }

  if (tess[myTess].type == TESS_RINGS) {
    if (!BoundaryTreeCellCoords (tess[myTess].tree, &zone, &band, ra, dec)) {
      fprintf (stderr, "mismatch!\n");
      return FALSE;
    }
  
    // I have ra, dec, and the primary projection cell.  In order to choose the primary skycell,
    // I just need to project to ra,dec to X,Y based on the center of the cell and then get the subdivision right.
    
    double x = 0.0;
    double y = 0.0;
    BoundaryTreeProjection (&x, &y, ra, dec, tess[myTess].tree, zone, band);
  
    int xi = x / tess[myTess].tree->dX[zone][band];
    int yi = y / tess[myTess].tree->dY[zone][band];

    xi = MAX(MIN(xi, tess[myTess].tree->NX_SUB - 1.0), 0);
    yi = MAX(MIN(yi, tess[myTess].tree->NY_SUB - 1.0), 0);

    int N = xi + tess[myTess].tree->NX_SUB * yi;
  
    *tessID = myTess;
    *projID = tess[myTess].tree->projID[zone][band];
    if (tess[myTess].tree->skycellID[zone][band] == -1) {
      *skycellID = N;
    } else {
      *skycellID = tess[myTess].tree->skycellID[zone][band] + N;
    }      

    return TRUE;
  }

  fprintf (stderr, "invalid tessellation type!\n");
  return FALSE;
}


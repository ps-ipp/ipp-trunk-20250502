# include "dvo.h"

# define GET_COLUMN_NEW(HEADER,FTABLE,OUT,NAME,TYPE)				\
  TYPE *OUT = gfits_get_bintable_column_data (HEADER, FTABLE, NAME, type, &Nrow, &Ncol); \
  myAssert (!strcmp(type, #TYPE), "wrong column type");

# define GET_COLUMN_RAW(HEADER,FTABLE,OUT,NAME,TYPE)			\
  OUT = gfits_get_bintable_column_data (HEADER, FTABLE, NAME, type, &Nrow, &Ncol); \
  myAssert (!strcmp(type, #TYPE), "wrong column type");

# define DEBUG 0

void BoundaryTreeFree(BoundaryTree *tree) {

  if (!tree) return;

  for (int i = 0; i < tree->Nzone; i++) {
    for (int j = 0; j < tree->Nband[i]; j++) {
      FREE (tree->name[i][j]);
    }
    FREE (tree->ra[i]);
    FREE (tree->dec[i]);
    FREE (tree->Xo[i]);
    FREE (tree->Yo[i]);
    FREE (tree->dX[i]);
    FREE (tree->dY[i]);
    FREE (tree->cell[i]);
    FREE (tree->projID[i]);
    FREE (tree->skycellID[i]);
    FREE (tree->name[i]);
  }

  FREE (tree->ra);
  FREE (tree->dec);
  FREE (tree->Xo);
  FREE (tree->Yo);
  FREE (tree->dX);
  FREE (tree->dY);
  FREE (tree->cell);
  FREE (tree->projID);
  FREE (tree->skycellID);
  FREE (tree->name);

  FREE (tree->Nband);
  FREE (tree->RA_origin);
  FREE (tree->RA_offset);
  FREE (tree->DEC_min);
  FREE (tree->DEC_max);
  FREE (tree->DEC_min_raw);
  FREE (tree->DEC_max_raw);

  return;
}

BoundaryTree *BoundaryTreeLoad(char *filename) {

  Header header;
  Header theader;
  Matrix matrix;

  header.buffer = NULL;
  matrix.buffer = NULL;
  theader.buffer = NULL;
  BoundaryTree *tree = NULL;

  FILE *f = fopen (filename, "r");
  if (!f) {
    fprintf (stderr, "ERROR: cannot open boundary tree file %s\n", filename);
    return NULL;
  }

  /* load in PHU segment (ignore) */
  if (!gfits_fread_header (f, &header)) {
    if (DEBUG) fprintf (stderr, "can't read boundary tree header\n");
    goto escape;
  }
  if (!gfits_fread_matrix (f, &matrix, &header)) {
    if (DEBUG) fprintf (stderr, "can't read boundary tree matrix\n");
    goto escape;
  }

  // load data for this header 
  if (!gfits_load_header (f, &theader)) {
    if (DEBUG) fprintf (stderr, "can't read boundary tree zone table header\n");
    goto escape;
  }

  tree = BoundaryTreeRead (&header, &theader, f);

escape:

  gfits_free_header (&theader);
  gfits_free_header (&header);
  gfits_free_matrix (&matrix);
  fclose (f);

  return tree;
}

// assume we are pointing at the relevant table portion
BoundaryTree *BoundaryTreeRead(Header *headerPHU, Header *headerZone, FILE *f) {

  int i, j, nz, nb, Ncol;
  off_t Nrow;
  char type[16];

  FTable ftableZone;

  Header headerCell;
  FTable ftableCell;

  ftableZone.buffer = NULL;
  ftableCell.buffer = NULL;
  headerCell.buffer = NULL;

  // we must have already read in the Zone table header section
  ftableZone.header =  headerZone;
  ftableCell.header = &headerCell;

  BoundaryTree *tree = NULL;

  ALLOCATE (tree, BoundaryTree, 1);

  // we need to read the boundary tree parameters from the correct header
  // put them in the PHU header in any case?
  gfits_scan (headerPHU, "DEC_ORI",  "%lf", 1, &tree->DEC_origin);
  gfits_scan (headerPHU, "DEC_OFF",  "%lf", 1, &tree->DEC_offset);
  gfits_scan (headerPHU, "NX_SUB",   "%f",  1, &tree->NX_SUB);
  gfits_scan (headerPHU, "NY_SUB",   "%f",  1, &tree->NY_SUB);
  gfits_scan (headerPHU, "PIXSCALE", "%lf", 1, &tree->dPix);

  /*** zone information table ***/

  // read the fits table bytes
  if (!gfits_fread_ftable_data (f, &ftableZone, FALSE)) goto escape;
 
  // need to create and assign to flat-field correction
  GET_COLUMN_RAW(headerZone, &ftableZone, tree->Nband,       "NBAND",  	    int);
  GET_COLUMN_RAW(headerZone, &ftableZone, tree->RA_origin,   "RA_ORIGIN",   double);
  GET_COLUMN_RAW(headerZone, &ftableZone, tree->RA_offset,   "RA_OFFSET",   double);
  GET_COLUMN_RAW(headerZone, &ftableZone, tree->DEC_min  ,   "DEC_MIN",     double);
  GET_COLUMN_RAW(headerZone, &ftableZone, tree->DEC_max  ,   "DEC_MAX",     double);
  GET_COLUMN_RAW(headerZone, &ftableZone, tree->DEC_min_raw, "DEC_MIN_RAW", double);
  GET_COLUMN_RAW(headerZone, &ftableZone, tree->DEC_max_raw, "DEC_MAX_RAW", double);
  gfits_free_table  (&ftableZone);

  fprintf (stderr, "loaded data for %lld zones\n", (long long) Nrow);
  tree->Nzone = Nrow;

  // allocate the storage arrays
  ALLOCATE (tree->ra,   double *, tree->Nzone);
  ALLOCATE (tree->dec,  double *, tree->Nzone);
  ALLOCATE (tree->Xo,   double *, tree->Nzone);
  ALLOCATE (tree->Yo,   double *, tree->Nzone);
  ALLOCATE (tree->dX,    float *, tree->Nzone);
  ALLOCATE (tree->dY,    float *, tree->Nzone);
  ALLOCATE (tree->cell,    int *, tree->Nzone);
  ALLOCATE (tree->projID,  int *, tree->Nzone);
  ALLOCATE (tree->skycellID,  int *, tree->Nzone);
  ALLOCATE (tree->name,  char **, tree->Nzone);
  for (i = 0; i < tree->Nzone; i++) {
    ALLOCATE (tree->ra[i],   double, tree->Nband[i]);
    ALLOCATE (tree->dec[i],  double, tree->Nband[i]);
    ALLOCATE (tree->Xo[i],   double, tree->Nband[i]);
    ALLOCATE (tree->Yo[i],   double, tree->Nband[i]);
    ALLOCATE (tree->dX[i],    float, tree->Nband[i]);
    ALLOCATE (tree->dY[i],    float, tree->Nband[i]);
    ALLOCATE (tree->cell[i],    int, tree->Nband[i]);
    ALLOCATE (tree->projID[i],  int, tree->Nband[i]);
    ALLOCATE (tree->skycellID[i],  int, tree->Nband[i]);
    ALLOCATE (tree->name[i], char *, tree->Nband[i]);
    for (j = 0; j < tree->Nband[i]; j++) {
      ALLOCATE (tree->name[i][j], char, BOUNDARY_TREE_NAME_LENGTH);
    }
  }

  /*** cell information table ***/

  // load data for this header 
  if (!gfits_load_header (f, &headerCell)) goto escape;

  // read the fits table bytes
  if (!gfits_fread_ftable_data (f, &ftableCell, FALSE)) goto escape;
 
  // need to create and assign to flat-field correction
  GET_COLUMN_NEW(&headerCell, &ftableCell, R,     "RA",   	 double);
  GET_COLUMN_NEW(&headerCell, &ftableCell, D,     "DEC",  	 double);
  GET_COLUMN_NEW(&headerCell, &ftableCell, zone,  "ZONE",        int);
  GET_COLUMN_NEW(&headerCell, &ftableCell, band,  "BAND",        int);
  GET_COLUMN_NEW(&headerCell, &ftableCell, index, "INDEX",       int);
  GET_COLUMN_NEW(&headerCell, &ftableCell, Xo,    "X_CENT",      double);
  GET_COLUMN_NEW(&headerCell, &ftableCell, Yo,    "Y_CENT",      double);
  GET_COLUMN_NEW(&headerCell, &ftableCell, dX,    "X_GRID",      float);
  GET_COLUMN_NEW(&headerCell, &ftableCell, dY,    "Y_GRID",      float);
  GET_COLUMN_NEW(&headerCell, &ftableCell, name,  "NAME",        char); // XXX how is this done?
  gfits_free_header (&headerCell);
  gfits_free_table  (&ftableCell);

  fprintf (stderr, "loaded data for %lld cells\n", (long long) Nrow);

  // assign the storage arrays
  for (i = 0; i < Nrow; i++) {
    nz = zone[i];
    nb = band[i];
    tree->ra[nz][nb] = R[i];
    tree->dec[nz][nb] = D[i];
    tree->Xo[nz][nb] = Xo[i];
    tree->Yo[nz][nb] = Yo[i];
    tree->dX[nz][nb] = dX[i];
    tree->dY[nz][nb] = dY[i];
    tree->cell[nz][nb] = i; // XXX ?
    memcpy(tree->name[nz][nb], &name[i*BOUNDARY_TREE_NAME_LENGTH], BOUNDARY_TREE_NAME_LENGTH);
    // XXX parse out the ID from the name (skycell.NNNN)
    char *endptr = NULL;
    tree->projID[nz][nb] = strtol(&tree->name[nz][nb][8], &endptr, 10);
    if (endptr[0] == '.') {
      // we have elements after the projID, which should be the skycell ID
      endptr ++;
      tree->skycellID[nz][nb] = strtol(endptr, NULL, 10);
    } else {
      tree->skycellID[nz][nb] = -1;
    }
  }

  free (R     );
  free (D     );
  free (zone  );
  free (band  );
  free (Xo    );
  free (Yo    );
  free (dX    );
  free (dY    );
  free (index );
  free (name  );

  return tree;

escape:
  gfits_free_header (&headerCell);
  gfits_free_table  (&ftableCell);
  gfits_free_table  (&ftableZone);
  if (tree) free (tree);

  return NULL;
}

// we are passed a BoundaryTree structure, write it to a FITS table (3 ext)
int BoundaryTreeSave(char *filename, BoundaryTree *tree) {

  Header header;
  Matrix matrix;

  gfits_init_header (&header);
  header.extend = TRUE;
  gfits_create_header (&header);
  gfits_create_matrix (&header, &matrix);

  FILE *f = fopen (filename, "w");
  if (!f) {
    fprintf (stderr, "ERROR: cannot open boundary tree file for output %s\n", filename);
    return FALSE;
  }

  // we need some information in the header to define the layout
  gfits_modify (&header, "DEC_ORI", "%lf", 1, tree->DEC_origin);
  gfits_modify (&header, "DEC_OFF", "%lf", 1, tree->DEC_offset);

  gfits_modify (&header, "NX_SUB", "%f", 1, tree->NX_SUB);
  gfits_modify (&header, "NY_SUB", "%f", 1, tree->NY_SUB);
  gfits_modify (&header, "PIXSCALE", "%lf", 1, tree->dPix);

  gfits_fwrite_header  (f, &header);
  gfits_fwrite_matrix  (f, &matrix);
  gfits_free_header (&header);
  gfits_free_matrix (&matrix);

  BoundaryTreeWrite (f, tree);
  fclose (f);

  return TRUE;
}

int BoundaryTreeWrite(FILE *f, BoundaryTree *tree) {

  int i, nz, nb;
  Header theader;
  FTable ftable;

  /*** zone information table ***/
  {
    gfits_create_table_header (&theader, "BINTABLE", "ZONE_DATA");

    gfits_define_bintable_column (&theader, "J", "ZONE",      	"zone sequence number", "none", 1.0, 0.0);
    gfits_define_bintable_column (&theader, "J", "NBAND",     	"number of cells in each zone", "none", 1.0, 0.0);
    gfits_define_bintable_column (&theader, "D", "RA_ORIGIN", 	"origin of ra cell sequence", "degree", 1.0, 0.0);
    gfits_define_bintable_column (&theader, "D", "RA_OFFSET", 	"offset per cell of ra cell sequence", "degree/cell", 1.0, 0.0);
    gfits_define_bintable_column (&theader, "D", "DEC_MIN",   	"min dec for zone", "degree", 1.0, 0.0);
    gfits_define_bintable_column (&theader, "D", "DEC_MAX",   	"max dec for zone", "degree", 1.0, 0.0);
    gfits_define_bintable_column (&theader, "D", "DEC_MIN_RAW", "min dec for zone", "degree", 1.0, 0.0);
    gfits_define_bintable_column (&theader, "D", "DEC_MAX_RAW", "max dec for zone", "degree", 1.0, 0.0);

    // generate the output array that carries the data
    gfits_create_table (&theader, &ftable);

    // create intermediate storage arrays
    int *zone = NULL; ALLOCATE (zone,  int, tree->Nzone);

    // assign the storage arrays
    for (i = 0; i < tree->Nzone; i++) {
      zone[i] = i;
    }

    // add the columns to the output array
    gfits_set_bintable_column (&theader, &ftable, "ZONE",   	 zone,              tree->Nzone);
    gfits_set_bintable_column (&theader, &ftable, "NBAND",   	 tree->Nband,       tree->Nzone);
    gfits_set_bintable_column (&theader, &ftable, "RA_ORIGIN", 	 tree->RA_origin,   tree->Nzone);
    gfits_set_bintable_column (&theader, &ftable, "RA_OFFSET", 	 tree->RA_offset,   tree->Nzone);
    gfits_set_bintable_column (&theader, &ftable, "DEC_MIN", 	 tree->DEC_min,     tree->Nzone);
    gfits_set_bintable_column (&theader, &ftable, "DEC_MAX", 	 tree->DEC_max,     tree->Nzone);
    gfits_set_bintable_column (&theader, &ftable, "DEC_MIN_RAW", tree->DEC_min_raw, tree->Nzone);
    gfits_set_bintable_column (&theader, &ftable, "DEC_MAX_RAW", tree->DEC_max_raw, tree->Nzone);
    free (zone);

    gfits_fwrite_Theader (f, &theader);
    gfits_fwrite_table (f, &ftable);
    gfits_free_header (&theader);
    gfits_free_table (&ftable);
  }

  /*** cell information table ***/
  {
    gfits_create_table_header (&theader, "BINTABLE", "CELL_DATA");

    char fmt[16];
    snprintf (fmt, 16, "%dA", BOUNDARY_TREE_NAME_LENGTH);
    gfits_define_bintable_column (&theader, "D", "RA",   "ra (J2000) of cell center", "degree", 1.0, 0.0);
    gfits_define_bintable_column (&theader, "D", "DEC",  "dec (J2000) of cell center", "degree", 1.0, 0.0);
    gfits_define_bintable_column (&theader, "J", "ZONE", "zone sequence number", "none", 1.0, 0.0);
    gfits_define_bintable_column (&theader, "J", "BAND", "band sequence number", "none", 1.0, 0.0);
    gfits_define_bintable_column (&theader, "J", "INDEX","cell index", "none", 1.0, 0.0);
    gfits_define_bintable_column (&theader, "D", "X_CENT", "projection cell center pixel", "none", 1.0, 0.0);
    gfits_define_bintable_column (&theader, "D", "Y_CENT", "projection cell center pixel", "none", 1.0, 0.0);
    gfits_define_bintable_column (&theader, "E", "X_GRID", "skycell grid spacing", "none", 1.0, 0.0);
    gfits_define_bintable_column (&theader, "E", "Y_GRID", "skycell grid spacing", "none", 1.0, 0.0);
    gfits_define_bintable_column (&theader, fmt, "NAME", "cell name", "none", 1.0, 0.0);

    // generate the output array that carries the data
    gfits_create_table (&theader, &ftable);

    int Ncell = 0;
    for (i = 0; i < tree->Nzone; i++) {
      Ncell += tree->Nband[i];
    }

    // create intermediate storage arrays
    // NOTE: we have to unroll the 2D arrays in tree into 1D arrays
    double *R             ; ALLOCATE (R,     double, Ncell);
    double *D             ; ALLOCATE (D,     double, Ncell);
    int    *zone          ; ALLOCATE (zone,  int,    Ncell);
    int    *band          ; ALLOCATE (band,  int,    Ncell);
    int    *index         ; ALLOCATE (index, int,    Ncell);
    double *Xo            ; ALLOCATE (Xo,    double, Ncell);
    double *Yo            ; ALLOCATE (Yo,    double, Ncell);
    float  *dX            ; ALLOCATE (dX,    float,  Ncell);
    float  *dY            ; ALLOCATE (dY,    float,  Ncell);
    char   *name          ; ALLOCATE (name,  char,   Ncell*BOUNDARY_TREE_NAME_LENGTH);

    // NOTE: a table column of characters must be fixed width, and is passed as a
    // contiguous array of Nchar * Nrow values

    // assign the storage arrays
    i = 0;
    for (nz = 0; nz < tree->Nzone; nz++) {
      for (nb = 0; nb < tree->Nband[nz]; nb++) {
	R[i]     = tree->ra[nz][nb];
	D[i]     = tree->dec[nz][nb];
	Xo[i]    = tree->Xo[nz][nb];
	Yo[i]    = tree->Yo[nz][nb];
	dX[i]    = tree->dX[nz][nb];
	dY[i]    = tree->dY[nz][nb];
	zone[i]  = nz;
	band[i]  = nb;
	index[i] = i; // or tree->cells[nz][nb] ?
	memcpy(&name[i*BOUNDARY_TREE_NAME_LENGTH], tree->name[nz][nb], BOUNDARY_TREE_NAME_LENGTH);
	i++; 
      }
    }

    // add the columns to the output array
    gfits_set_bintable_column (&theader, &ftable, "RA",    R,     Ncell);
    gfits_set_bintable_column (&theader, &ftable, "DEC",   D,     Ncell);
    gfits_set_bintable_column (&theader, &ftable, "ZONE",  zone,  Ncell);
    gfits_set_bintable_column (&theader, &ftable, "BAND",  band,  Ncell);
    gfits_set_bintable_column (&theader, &ftable, "INDEX", index, Ncell);
    gfits_set_bintable_column (&theader, &ftable, "X_CENT", Xo,   Ncell);
    gfits_set_bintable_column (&theader, &ftable, "Y_CENT", Yo,   Ncell);
    gfits_set_bintable_column (&theader, &ftable, "X_GRID", dX,   Ncell);
    gfits_set_bintable_column (&theader, &ftable, "Y_GRID", dY,   Ncell);
    gfits_set_bintable_column (&theader, &ftable, "NAME",  name,  Ncell);

    free (R     );
    free (D     );
    free (zone  );
    free (band  );
    free (index );
    free (Xo    );
    free (Yo    );
    free (dX    );
    free (dY    );
    free (name  );

    gfits_fwrite_Theader (f, &theader);
    gfits_fwrite_table  (f, &ftable);
    gfits_free_header (&theader);
    gfits_free_table (&ftable);
  }
  return TRUE;
}

// the boundary tree...
// given an (ra,dec) pair, find the containing projection cell.

int BoundaryTreeCellCoords (BoundaryTree *tree, int *zone, int *band, double ra, double dec) {

  ra = ohana_normalize_angle (ra);

  // first, find the containing zone

  // if we know dDEC, we can get the bin instantly:
  *zone = (dec - tree->DEC_origin) / tree->DEC_offset;
  
  if (*zone < 0) return FALSE;
  if (*zone >= tree->Nzone) return FALSE;

  // test if:
  // (a) DEC_min,DEC_max of *zone are defined (not NAN)
  // (b) dec for *zone falls in range DEC_min[*zone] <= dec < DEC_max[*zone]
  //     migrate up or down

  // TEST int zone_raw = *zone;
  if (isfinite(tree->DEC_min[*zone])) {
    if (dec >= tree->DEC_max[*zone]) (*zone) ++;
    if (dec <  tree->DEC_min[*zone]) (*zone) --;
    // TEST assert (dec <  tree->DEC_max[*zone]);
    // TEST assert (dec >= tree->DEC_min[*zone]);
  }
  // TEST assert (zone_raw > -1);

  // now select the RA bin for that zone
  *band = (ra - tree->RA_origin[*zone]) / tree->RA_offset[*zone];
  
  if (*band < 0) return FALSE;
  if (*band >= tree->Nband[*zone]) *band = 0;

  return TRUE;
}


// projection = TAN
// need Ro, Do, Xo, Yo, dPix

int BoundaryTreeProjection (double *x, double *y, double r, double d, BoundaryTree *tree, int zone, int band) {

    double Xo = tree->Xo[zone][band];
    double Yo = tree->Yo[zone][band];
    double Ro = tree->ra[zone][band];
    double Do = tree->dec[zone][band];
    double dPix = tree->dPix;

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


# include "addstar.h"

/** this file is deprecated (functions moved to libdvo) ***/

# define GET_COLUMN_NEW(OUT,NAME,TYPE)					\
  TYPE *OUT = gfits_get_bintable_column_data (&theader, &ftable, NAME, type, &Nrow, &Ncol); \
  myAssert (!strcmp(type, #TYPE), "wrong column type");

# define GET_COLUMN_RAW(OUT,NAME,TYPE)					\
  OUT = gfits_get_bintable_column_data (&theader, &ftable, NAME, type, &Nrow, &Ncol); \
  myAssert (!strcmp(type, #TYPE), "wrong column type");

BoundaryTree *BoundaryTreeLoad(char *filename) {

  int i, j, nz, nb, Ncol;
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
  BoundaryTree *tree = NULL;

  FILE *f = fopen (filename, "r");
  if (!f) {
    fprintf (stderr, "ERROR: cannot open image subset file %s\n", filename);
    return NULL;
  }

  /* load in PHU segment (ignore) */
  if (!gfits_fread_header (f, &header)) {
    if (VERBOSE) fprintf (stderr, "can't read image subset header\n");
    goto escape;
  }
  if (!gfits_fread_matrix (f, &matrix, &header)) {
    if (VERBOSE) fprintf (stderr, "can't read image subset matrix\n");
    goto escape;
  }

  ALLOCATE (tree, BoundaryTree, 1);

  gfits_scan (&header, "DEC_ORI", "%lf", 1, &tree->DEC_origin);
  gfits_scan (&header, "DEC_OFF", "%lf", 1, &tree->DEC_offset);

  ftable.header = &theader;

  /*** zone information table ***/
  { 
    // load data for this header 
    if (!gfits_load_header (f, &theader)) goto escape;

    // read the fits table bytes
    if (!gfits_fread_ftable_data (f, &ftable, FALSE)) goto escape;
 
    // need to create and assign to flat-field correction
    GET_COLUMN_RAW(tree->Nband,     "NBAND",  	 int);
    GET_COLUMN_RAW(tree->RA_origin, "RA_ORIGIN", double);
    GET_COLUMN_RAW(tree->RA_offset, "RA_OFFSET", double);
    gfits_free_header (&theader);
    gfits_free_table  (&ftable);

    fprintf (stderr, "loaded data for %lld zones\n", (long long) Nrow);
    tree->Nzone = Nrow;

    // allocate the storage arrays
    ALLOCATE (tree->ra,   double *, tree->Nzone);
    ALLOCATE (tree->dec,  double *, tree->Nzone);
    ALLOCATE (tree->cell, int *, tree->Nzone);
    ALLOCATE (tree->name, char **, tree->Nzone);
    for (i = 0; i < tree->Nzone; i++) {
      ALLOCATE (tree->ra[i],   double, tree->Nband[i]);
      ALLOCATE (tree->dec[i],  double, tree->Nband[i]);
      ALLOCATE (tree->cell[i], int,    tree->Nband[i]);
      ALLOCATE (tree->name[i], char *, tree->Nband[i]);
      for (j = 0; j < tree->Nband[i]; j++) {
	ALLOCATE (tree->name[i][j], char, BOUNDARY_TREE_NAME_LENGTH);
      }
    }
  }

  /*** cell information table ***/
  { 
    // load data for this header 
    if (!gfits_load_header (f, &theader)) goto escape;

    // read the fits table bytes
    if (!gfits_fread_ftable_data (f, &ftable, FALSE)) goto escape;
 
    // need to create and assign to flat-field correction
    GET_COLUMN_NEW(R,     "RA",   	 double);
    GET_COLUMN_NEW(D,     "DEC",  	 double);
    GET_COLUMN_NEW(zone,  "ZONE",        int);
    GET_COLUMN_NEW(band,  "BAND",        int);
    GET_COLUMN_NEW(index, "INDEX",       int);
    GET_COLUMN_NEW(name,  "NAME",        char); // XXX how is this done?
    gfits_free_header (&theader);
    gfits_free_table  (&ftable);
    fprintf (stderr, "loaded data for %lld cells\n", (long long) Nrow);

    // assign the storage arrays
    for (i = 0; i < Nrow; i++) {
      nz = zone[i];
      nb = band[i];
      tree->ra[nz][nb] = R[i];
      tree->dec[nz][nb] = D[i];
      tree->cell[nz][nb] = i; // XXX ?
      memcpy(tree->name[nz][nb], &name[i*BOUNDARY_TREE_NAME_LENGTH], BOUNDARY_TREE_NAME_LENGTH);
    }

    free (R     );
    free (D     );
    free (zone  );
    free (band  );
    free (index );
    free (name  );
  }

  gfits_free_header (&header);
  gfits_free_matrix (&matrix);
  fclose (f);

  return tree;

escape:
  gfits_free_header (&header);
  gfits_free_matrix (&matrix);
  gfits_free_header (&theader);
  gfits_free_table  (&ftable);
  if (tree) free (tree);

  fclose (f);
  return NULL;
}

// we are passed a BoundaryTree structure, write it to a FITS table (3 ext)
int BoundaryTreeSave(char *filename, BoundaryTree *tree) {

  int i, nz, nb;
  Header header;
  Header theader;
  Matrix matrix;
  FTable ftable;

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

  gfits_fwrite_header  (f, &header);
  gfits_fwrite_matrix  (f, &matrix);
  gfits_free_header (&header);
  gfits_free_matrix (&matrix);

  /*** zone information table ***/
  {
    gfits_create_table_header (&theader, "BINTABLE", "ZONE_DATA");

    gfits_define_bintable_column (&theader, "J", "ZONE",      "zone sequence number", "none", 1.0, 0.0);
    gfits_define_bintable_column (&theader, "J", "NBAND",     "number of cells in each zone", "none", 1.0, 0.0);
    gfits_define_bintable_column (&theader, "D", "RA_ORIGIN", "origin of ra cell sequence", "degree", 1.0, 0.0);
    gfits_define_bintable_column (&theader, "D", "RA_OFFSET", "offset per cell of ra cell sequence", "degree/cell", 1.0, 0.0);

    // generate the output array that carries the data
    gfits_create_table (&theader, &ftable);

    // create intermediate storage arrays
    int *zone = NULL; ALLOCATE (zone,  int, tree->Nzone);

    // assign the storage arrays
    for (i = 0; i < tree->Nzone; i++) {
      zone[i] = i;
    }

    // add the columns to the output array
    gfits_set_bintable_column (&theader, &ftable, "ZONE",   	zone,            tree->Nzone);
    gfits_set_bintable_column (&theader, &ftable, "NBAND",   	tree->Nband,     tree->Nzone);
    gfits_set_bintable_column (&theader, &ftable, "RA_ORIGIN", 	tree->RA_origin, tree->Nzone);
    gfits_set_bintable_column (&theader, &ftable, "RA_OFFSET", 	tree->RA_offset, tree->Nzone);
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
    char   *name          ; ALLOCATE (name,  char,   Ncell*BOUNDARY_TREE_NAME_LENGTH);

    // NOTE: a table column of characters must be fixed width, and is passed as a
    // contiguous array of Nchar * Nrow values

    // assign the storage arrays
    i = 0;
    for (nz = 0; nz < tree->Nzone; nz++) {
      for (nb = 0; nb < tree->Nband[nz]; nb++) {
	R[i]     = tree->ra[nz][nb];
	D[i]     = tree->dec[nz][nb];
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
    gfits_set_bintable_column (&theader, &ftable, "NAME",  name,  Ncell);

    free (R     );
    free (D     );
    free (zone  );
    free (band  );
    free (index );
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

  // first, find the containing zone

  // if we know dDEC, we can get the bin instantly:
  *zone = (dec - tree->DEC_origin) / tree->DEC_offset;
  
  if (*zone < 0) return FALSE;
  if (*zone >= tree->Nzone) return FALSE;

  // now select the RA bin for that zone
  *band = (ra - tree->RA_origin[*zone]) / tree->RA_offset[*zone];
  
  if (*band < 0) return FALSE;
  if (*band >= tree->Nband[*zone]) *band = 0;

  return TRUE;
}


# include <ohana.h>
# include <gfitsio.h>
# include "inttypes.h"

void table_uncompress (int argc, char **argv);
int print_table_rows (FTable *table, int start, int Nrows);
FILE *load_extension (char *file, int Nextend, char *Extname, Header *header);
void print_column (FTable *table, int Column, char *Colname);
void usage();
void list_extnames (char *file);
void print_layout (Header *header);
int Binary;

int main (int argc, char **argv) {

  off_t Nx, Ny, Nbytes, Nread;
  int N;
  char ttype[80];
  FTable table;
  Header header;
  FILE *f;

  if (get_argument (argc, argv, "-h")) usage ();
  if (get_argument (argc, argv, "--help")) usage ();

  if ((N = get_argument (argc, argv, "-uncompress"))) {
    remove_argument (N, &argc, argv);
    table_uncompress (argc, argv);
  }
  if ((N = get_argument (argc, argv, "-u"))) {
    remove_argument (N, &argc, argv);
    table_uncompress (argc, argv);
  }

  int Nextend = 0;
  if ((N = get_argument (argc, argv, "-x"))) {
    remove_argument (N, &argc, argv);
    Nextend = atoi (argv[N]);
    remove_argument (N, &argc, argv);
  }

  char *Extname = NULL;
  if ((N = get_argument (argc, argv, "-n"))) {
    if (Nextend) usage ();
    remove_argument (N, &argc, argv);
    Extname = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }

  off_t Row = -1;
  if ((N = get_argument (argc, argv, "-row"))) {
    remove_argument (N, &argc, argv);
    Row = atoi (argv[N]);
    remove_argument (N, &argc, argv);
  }

  off_t RowStart = 0;
  if ((N = get_argument (argc, argv, "-row-start"))) {
    remove_argument (N, &argc, argv);
    RowStart = atoi (argv[N]);
    remove_argument (N, &argc, argv);
  }

  off_t RowStop = 20;
  if ((N = get_argument (argc, argv, "-row-stop"))) {
    remove_argument (N, &argc, argv);
    RowStop = atoi (argv[N]);
    remove_argument (N, &argc, argv);
  }

  int Column = 0;
  if ((N = get_argument (argc, argv, "-ncolumn"))) {
    remove_argument (N, &argc, argv);
    Column = atoi (argv[N]);
    remove_argument (N, &argc, argv);
  }

  char *Colname = 0;
  if ((N = get_argument (argc, argv, "-column"))) {
    if (Column) usage ();
    remove_argument (N, &argc, argv);
    Colname = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }

  int ListExtname = FALSE;
  if ((N = get_argument (argc, argv, "-list"))) {
    remove_argument (N, &argc, argv);
    ListExtname = TRUE;
  }

  int Layout = FALSE;
  if ((N = get_argument (argc, argv, "-layout"))) {
    remove_argument (N, &argc, argv);
    Layout = TRUE;
  }

  if (argc != 2) usage ();

  if (ListExtname) list_extnames (argv[1]);

  /* load header */
  table.header = &header;
  f = load_extension (argv[1], Nextend, Extname, table.header);

  if (Layout) print_layout (table.header);

  Binary = FALSE;
  gfits_scan (table.header, "XTENSION", "%s", 1, ttype);
  if (!strcmp (ttype, "BINTABLE")) Binary = TRUE;

  /* load table data array */
  Nbytes = gfits_data_size (table.header);
  ALLOCATE (table.buffer, char, Nbytes);
  Nread = fread (table.buffer, sizeof (char), Nbytes, f);
  if (Nread != Nbytes) {
    fprintf (stderr, "failed to read all table data\n");
    exit (1);
  }
  table.datasize = Nbytes;

  gfits_scan (table.header, "NAXIS1",  OFF_T_FMT, 1,  &Nx);
  gfits_scan (table.header, "NAXIS2",  OFF_T_FMT, 1,  &Ny);

  /* print a column */
  if (Column || (Colname != (char *) NULL)) print_column (&table, Column, Colname);

  /* print a row */
  if (Row > -1) {
    if (!print_table_rows (&table, Row, 1)) {
      fprintf (stderr, "failed to print row\n");
    }
    exit (0);
  }

  /* print rows of table */
  if (!print_table_rows (&table, RowStart, RowStop)) {
    fprintf (stderr, "failed to print table\n");
  }    
  fprintf (stdout, "# %d of %d total rows (%d to %d)\n", (int) (RowStop - RowStart), (int) Ny, (int) RowStart, (int) RowStop);
  exit (0);
}

# define SWAP_BYTE(BYTE)					\
  tmp = BYTE[0]; BYTE[0] = BYTE[1]; BYTE[1] = tmp;
# define SWAP_WORD(BYTE) \
  tmp = BYTE[0]; BYTE[0] = BYTE[3]; BYTE[3] = tmp; \
  tmp = BYTE[1]; BYTE[1] = BYTE[2]; BYTE[2] = tmp;
# define SWAP_DBLE(BYTE) \
  tmp = BYTE[0]; BYTE[0] = BYTE[7]; BYTE[7] = tmp; \
  tmp = BYTE[1]; BYTE[1] = BYTE[6]; BYTE[6] = tmp; \
  tmp = BYTE[2]; BYTE[2] = BYTE[5]; BYTE[5] = tmp; \
  tmp = BYTE[3]; BYTE[3] = BYTE[4]; BYTE[4] = tmp;

/* print Nrows of the given table starting at row 'start' */
int print_table_rows (FTable *table, int start, int Nrows) {
  
  off_t Nx, Ny;
  int n, i, j, Nfields, *Nbyte, *Nvals, Oout, Oin, Nv, Nb, byte, status;
  char field[16], **types, format[16], type[80], *line, *row, tmp;
  double *Tzero, *Tscal;

  gfits_scan (table->header, "NAXIS1",  OFF_T_FMT, 1,  &Nx);
  gfits_scan (table->header, "NAXIS2",  OFF_T_FMT, 1,  &Ny);
  gfits_scan (table->header, "TFIELDS", "%d", 1, &Nfields);

  if (start <   0) return FALSE;
  if (start >= Ny) return FALSE;
  if (Nrows <   0) return FALSE;
  if (start + Nrows > Ny) return FALSE;

  /* assume we have one space per byte column */
  ALLOCATE (line, char, MAX(2*Nx+1,512));

  ALLOCATE (types, char *, Nfields);
  ALLOCATE (Nvals, int, Nfields);
  ALLOCATE (Nbyte, int, Nfields);
  ALLOCATE (Tzero, double, Nfields);
  ALLOCATE (Tscal, double, Nfields);

  // determine the layout of the columns
  for (i = 0; i < Nfields; i++) {
    sprintf (field, "TFORM%d", i+1);
    gfits_scan (table->header, field, "%s", 1, format); /* get field format */

    if (Binary)  {
      gfits_bintable_format (format, type, &Nv, &Nb);    /* convert to c-style */

      sprintf (field, "TZERO%d", i+1);
      status = gfits_scan (table[0].header, field, "%lf", 1, &Tzero[i]);       /* get field format */
      if (!status) Tzero[i] = 0.0;
      
      sprintf (field, "TSCAL%d", i+1);
      status = gfits_scan (table[0].header, field, "%lf", 1, &Tscal[i]);       /* get field format */
      if (!status) Tscal[i] = 1.0;
    } else {
      gfits_table_format (format, type, &Nv, &Nb);    /* convert to c-style */
    }
    
    types[i] = strcreate (type);
    Nvals[i] = Nv;
    Nbyte[i] = Nb;
  }

  for (n = start; n < start + Nrows; n++) {

    row = &table->buffer[Nx*n];

    if (Binary) {
      byte = 0;  // counter for byte element of this row
      for (i = 0; i < Nfields; i++) {
	int found = FALSE;
	if (!strcmp (types[i], "char")) {
	  memcpy (line, &row[byte], Nvals[i]*Nbyte[i]);
	  line[Nvals[i]*Nbyte[i]] = 0;
	  fprintf (stdout, "%s ", line);
	  found = TRUE;
	} else {
	  for (j = 0; j < Nvals[i]; j++) {
	    memcpy (line, &row[byte + Nbyte[i]*j], Nbyte[i]);
	    if (!strcmp (types[i], "int")) {
# ifdef BYTE_SWAP
	      SWAP_WORD (line);
# endif
	      fprintf (stdout, "%d ", (int)(*(int *)line * Tscal[i] + Tzero[i]));
	      found = TRUE;
	    }
	    if (!strcmp (types[i], "short")) {
# ifdef BYTE_SWAP
	      SWAP_BYTE (line);
# endif
	      fprintf (stdout, "%d ", (int)(*(short *)line * Tscal[i] + Tzero[i]));
	      found = TRUE;
	    }
	    if (!strcmp (types[i], "int64_t")) {
# ifdef BYTE_SWAP
	      SWAP_DBLE (line);
# endif
	      fprintf (stdout, "%" PRId64" ",  (int64_t)(*(int64_t*)line * Tscal[i] + Tzero[i]));
	      found = TRUE;
	    }
	    if (!strcmp (types[i], "float")) {
# ifdef BYTE_SWAP
	      SWAP_WORD (line);
# endif
	      fprintf (stdout, "%e ", (*(float *)line * Tscal[i] + Tzero[i]));
	      found = TRUE;
	    }
	    if (!strcmp (types[i], "double")) {
# ifdef BYTE_SWAP
	      SWAP_DBLE (line);
# endif
	      fprintf (stdout, "%e ", (*(double *)line * Tscal[i] + Tzero[i]));
	      found = TRUE;
	    }
	  }
	}
	byte += Nvals[i]*Nbyte[i];
	if (!found) {
	  fprintf (stderr, "failed to find format for %d : %s\n", i, types[i]);
	}
      }
    } else {
      Oout = 0;
      Oin = 0;
      for (i = 0; i < Nfields; i++) {
	memcpy (&line[Oout], &row[Oin], Nvals[i]*Nbyte[i]);
	for (j = 0; j < Nvals[i]*Nbyte[i]; j++) if (line[Oout+j] == 0) line[Oout+j] = ' ';
	line[Oout+Nvals[i]*Nbyte[i]] = ' ';
	Oout += Nvals[i]*Nbyte[i] + 1;
	Oin += Nvals[i]*Nbyte[i];
      }
      fprintf (stdout, "%s ", line);
    }
    fprintf (stdout, "\n");
  }
  return (TRUE);
}

void usage () {
    fprintf (stderr, "USAGE: [-h] [-N N] (table.fits)\n");
    fprintf (stderr, " -x N:         operate on extension N (0 is default)\n");
    fprintf (stderr, " -n EXTNAME:   operate on named extension (incompatible with -x)\n");
    fprintf (stderr, " -row N:       print row number N\n");
    fprintf (stderr, " -row-start N: start at row number N\n");
    fprintf (stderr, " -row-stop N:  stop at row number N\n");
    fprintf (stderr, " -column n:    print column named n\n");
    fprintf (stderr, " -ncolumn N:   print column number N\n");
    fprintf (stderr, " -list:        print extension names\n");
    fprintf (stderr, " -layout:      describe extension\n");
    fprintf (stderr, " -uncompress:  uncompress file (requires output argument)\n");
    exit (2);
}

void list_extnames (char *file) {

  FILE *f;
  off_t Nbytes, Nelem;
  int i, Naxis, Ncomp, extend, status;
  char extname[82], exttype[82], axisname[32];
  Header header;

  f = fopen (file, "r");
  if (f == (FILE *) NULL) {
    fprintf (stderr, "can't open file %s\n", file);
    exit (1);
  }

  fprintf (stdout, "%-30s  %-15s NAXIS NAXIS(i)...\n", "extname", "datatype"); 

  Ncomp = 0;
  while (gfits_fread_header (f, &header)) {
    /* extract the EXTNAME for this component (set to PHU for 0th component) */
    status = gfits_scan (&header, "EXTNAME", "%s", 1, extname);
    if (!status) {
      if (Ncomp == 0) {
	strcpy (extname, "PHU");
      } else {
	strcpy (extname, "UNKNOWN");
      }
    }
    fprintf (stdout, "%-30s ", extname);

    int IsCompressed = gfits_extension_is_compressed_table (&header);

    /* extract the datatype for this component (IMAGE for 0th component) */
    if (Ncomp == 0) {
      strcpy (exttype, "IMAGE");
    } else {
      status = gfits_scan (&header, "XTENSION", "%s", 1, exttype);
      if (!status) {
	strcpy (exttype, "UNKNOWN");
      }
    }
    if (IsCompressed) {
      fprintf (stdout, "*%-15s ", exttype);
    } else {
      fprintf (stdout, " %-15s ", exttype);
    }

    /* extract the rank of the component */
    status = gfits_scan (&header, "NAXIS",  "%d", 1, &Naxis);
    if (!status) {
      fprintf (stderr, "component %d is missing Naxis!\n", Ncomp);
      Ncomp ++;
      continue;
    }
    fprintf (stdout, " %4d", Naxis);

    /* extract the individual axes */
    for (i = 0; i < Naxis; i++) {
      if (IsCompressed) {
	sprintf (axisname, "ZNAXIS%d", i+1);
      } else {
	sprintf (axisname, "NAXIS%d", i+1);
      }
      status = gfits_scan (&header, axisname,  OFF_T_FMT, 1,  &Nelem);
      if (!status) {
	fprintf (stderr, "missing %s\n", axisname);
      }
      fprintf (stdout, " "OFF_T_FMT,  Nelem);
    }
    fprintf (stdout, "\n");

    /* are extensions identified? (we will scan for them anyway) */
    if (Ncomp == 0) {
      extend = FALSE;
      gfits_scan_alt (&header, "EXTEND", "%t", 1, &extend);
      if (!extend) {
	fprintf (stderr, "no extensions listed in file\n");
      }
    }

    Nbytes = gfits_data_size (&header);
    fseeko (f, Nbytes, SEEK_CUR);

    Ncomp ++;
  }
  fclose (f);
  exit (0);
}

FILE *load_extension (char *file, int Nextend, char *Extname, Header *header) {

  off_t Nbytes;
  int i, extend;
  char extname[82];
  FILE *f;

  /* open file */
  f = fopen (file, "r");
  if (f == (FILE *) NULL) {
    fprintf (stderr, "can't open file %s\n", file);
    exit (1);
  }

  /* read PHU */
  if (!gfits_fread_header (f, header)) {
    fprintf (stderr, "can't read header from %s\n", file);
    exit (1);
  }

  /* check for existence of extensions */
  extend = FALSE;
  gfits_scan_alt (header, "EXTEND", "%t", 1, &extend);
  if (!extend) {
    fprintf (stderr, "no extensions listed in file\n");
  }

  /* skip first data array */
  Nbytes = gfits_data_size (header);
  fseeko (f, Nbytes, SEEK_CUR);

  /* search for extension of interest */
  for (i = 0; gfits_fread_Theader (f, header); i++) {
    if ((Extname == NULL) && (Nextend == i)) return (f);

    gfits_scan (header, "EXTNAME", "%s", 1, extname);
    if ((Extname != NULL) && (!strcmp (Extname, extname))) return (f);

    Nbytes = gfits_data_size (header);
    fseeko (f, Nbytes, SEEK_CUR);
    gfits_free_header (header);
  }
  fclose (f);

  fprintf (stderr, "failed to load extension of interest\n");
  exit (1);
}

void print_column (FTable *table, int Column, char *Colname) {
  
  off_t Nx, Ny;
  int i, j, Nfields, Nstart, Nv, Nb;
  Header *header;
  char format[16], field[16], type[16], *line, *data;

  header =  table[0].header;
  data   =  table[0].buffer;

  gfits_scan (header, "TFIELDS", "%d", 1, &Nfields);
  gfits_scan (header, "NAXIS1",  OFF_T_FMT, 1,  &Nx);
  gfits_scan (header, "NAXIS2",  OFF_T_FMT, 1,  &Ny);

  if (Colname != (char *) NULL) {
    /* find matching column entry */
    for (i = 1; i <= Nfields; i++) {
      sprintf (field, "TTYPE%d", i);
      gfits_scan (header, field, "%s", 1, type);
      if (!strcmp (type, Colname)) {
	Column = i;
	break;
      }
    }
    if (!Column) {
      fprintf (stderr, "column %s not found\n", Colname);
      exit (1);
    }
  } else {
    /* check if Column is in range */
    if (Column > Nfields) {
      fprintf (stderr, "-ncolumn %d too large, only %d columns\n", Column, Nfields);
      exit (1);
    }
  }

  /* scan columns to find insert point */
  Nstart = 0;
  for (i = 1; i < Column; i++) {
    sprintf (field, "TFORM%d", i);
    gfits_scan (header, field, "%s", 1, format);
    if (Binary)  {
      gfits_bintable_format (format, type, &Nv, &Nb);
    } else {
      gfits_table_format (format, type, &Nv, &Nb);
    }
    Nstart += Nv*Nb;
  }

  sprintf (field, "TFORM%d", Column);
  gfits_scan (header, field, "%s", 1, format);
  if (Binary)  {
    gfits_bintable_format (format, type, &Nv, &Nb);    /* convert to c-style */
  } else {
    gfits_table_format (format, type, &Nv, &Nb);    /* convert to c-style */
  }
  ALLOCATE (line, char, Nv*Nb + 1);

  if (Binary) {
    if (!gfits_get_bintable_column_type (header, Colname, type, &Nv)) return;
    // if (!strcmp (type, "char")) return;
    if (!gfits_get_bintable_column (header, table, Colname, (void **)&data)) return;
    // this results in an array of Ny*Nv entries

    for (i = 0; i < Ny; i++) {
      if (!strcmp (type, "char")) {
	memcpy (line, &data[i*Nv*Nb], Nv*Nb);
	fprintf (stdout, "%s\n", line);
      } else {
	for (j = 0; j < Nv; j++) {
	  if (!strcmp (type, "byte")) {
	    memcpy (line, &data[i*Nv*Nb + Nb*j], Nb);
	    fprintf (stdout, "%d ", *(char *)line);
	  }
	  if (!strcmp (type, "short")) {
	    memcpy (line, &data[i*Nv*Nb + Nb*j], Nb);
	    fprintf (stdout, "%d ", *(short *)line);
	  }
	  if (!strcmp (type, "int")) {
	    memcpy (line, &data[i*Nv*Nb + Nb*j], Nb);
	    fprintf (stdout, "%d ", *(int *)line);
	  }
	  if (!strcmp (type, "int64_t")) {
	    memcpy (line, &data[i*Nv*Nb + Nb*j], Nb);
	    fprintf (stdout, "%" PRId64" ",  *(int64_t*)line);
	  }
	  if (!strcmp (type, "float")) {
	    memcpy (line, &data[i*Nv*Nb + Nb*j], Nb);
	    fprintf (stdout, "%e ", *(float *)line);
	  }
	  if (!strcmp (type, "double")) {
	    memcpy (line, &data[i*Nv*Nb + Nb*j], Nb);
	    fprintf (stdout, "%e ", *(double *)line);
	  }
	}
	fprintf (stdout, "\n");
      }
    }
  } else {
    for (i = 0; i < Ny; i++) {
      memcpy (line, &data[i*Nx + Nstart], Nv*Nb);
      sprintf (format, "%%%ds\n", Nv*Nb);
      fprintf (stdout, format, line);
    }
  }
  exit (0);
}

void print_layout (Header *header) {

  int i, Nfields;
  char field[32], type[80], comment[80], format[80], unit[80], null[80], nval[80];

  gfits_scan (header, "TFIELDS", "%d", 1, &Nfields);

  int isCompressed = gfits_extension_is_compressed_table (header);

  for (i = 1; i <= Nfields; i++) {
    memset (field,   0, 32);
    memset (type,    0, 80);
    memset (comment, 0, 80);
    memset (format,  0, 80);
    memset (unit,    0, 80);
    memset (null,    0, 80);
    memset (nval,    0, 80);

    snprintf (field, 32, "TTYPE%d", i);
    gfits_scan (header, field, "%s", 1, type);
    gfits_scan_alt (header, field, "%C", 1, comment);

    if (!isCompressed) {
      snprintf (field, 32, "TFORM%d", i);
    } else {
      snprintf (field, 32, "ZFORM%d", i);
    }
    gfits_scan (header, field, "%s", 1, format);

    snprintf (field, 32, "TUNIT%d", i);
    gfits_scan (header, field, "%s", 1, unit);
    snprintf (field, 32, "TNULL%d", i);
    if (!gfits_scan (header, field, "%s", 1, null)) strcpy (null, "none");
    snprintf (field, 32, "TNVAL%d", i);
    if (!gfits_scan (header, field, "%s", 1, nval)) strcpy (nval, "none");

    fprintf (stdout, "%-18s %-32s %-18s %-6s %-8s %-8s\n", 
	  type, comment, unit, format, null, nval); 
  }
  exit (0);
}

void myQuit (char *format,...) {

  va_list argp;  
  va_start (argp, format);
  vfprintf (stderr, format, argp);
  va_end (argp);
  
  exit (1);
}

void table_uncompress (int argc, char **argv) {

  if (argc != 3) {
    fprintf (stderr, "USAGE: ftable -u/-uncompress (input) (output)\n");
    exit (2);
  }

  Header header;
  FTable ftable;
  Matrix matrix;
  ftable.header = &header;

  FILE *fi = fopen (argv[1], "r");
  if (!fi) myQuit ("failed to open input file %s\n", argv[1]);
  
  FILE *fo = fopen (argv[2], "w");
  if (!fo) myQuit ("failed to open output file %s\n", argv[2]);
  
  if (!gfits_fread_header (fi, &header)) myQuit ("failed to read PHU header from input file %s\n", argv[1]);
  if (!gfits_fread_matrix (fi, &matrix, &header)) myQuit ("failed to read PHU data array from input file %s\n", argv[1]);
  
  if (!gfits_fwrite_header (fo, &header)) myQuit ("failed to write PHU header to output file %s\n", argv[2]);
  if (!gfits_fwrite_matrix (fo, &matrix)) myQuit ("failed to write PHU data array to output file %s\n", argv[2]);
  
  gfits_free_header (&header);
  gfits_free_matrix (&matrix);
  
  int Nextend = 0;
  while (TRUE) {
    if (!gfits_fread_header (fi, &header)) break;

    if (!gfits_fread_ftable_data (fi, &ftable, FALSE)) {
      myQuit ("failed to read table data for extension #%d from input file %s\n", Nextend, argv[1]);
    }
  
    Header *outheader = &header;
    FTable *outtable  = &ftable;
    
    Header rawheader;
    FTable rawtable;
    rawtable.header = &rawheader;
    
    int IsCompressed = gfits_extension_is_compressed_table (&header);

    if (IsCompressed) {
      int i, Nfields;
      if (!gfits_scan (&header, "TFIELDS", "%d", 1, &Nfields)) 
	myQuit ("cannot find TFIELDS in header for extension #%d from input file %s\n", Nextend, argv[1]);
      for (i = 0; i < Nfields; i++) {
	if (!gfits_byteswap_varlength_column (&ftable, i+1)) 
	  myQuit ("cannot swap varlength column for extension #%d from input file %s\n", Nextend, argv[1]);
      }
      if (!gfits_uncompress_table (&ftable, &rawtable)) 
	myQuit ("failed to uncompress table for extension #%d from input file %s\n", Nextend, argv[1]);
      
      for (i = 0; i < Nfields; i++) {
	if (!gfits_byteswap_bintable_column (&rawtable, i+1)) 
	  myQuit ("cannot swap table column for extension #%d from input file %s\n", Nextend, argv[1]);
      }

      outheader = &rawheader;
      outtable  = &rawtable;
      gfits_free_header (&header);
      gfits_free_table  (&ftable);
    }

    if (!gfits_fwrite_header (fo, outheader)) {
      fprintf (stderr, "failed to write table header for extension #%d to output file %s\n", Nextend, argv[2]);
      exit (1);
    }
    if (!gfits_fwrite_table (fo, outtable)) {
      fprintf (stderr, "failed to write PHU data array for extension #%d to output file %s\n", Nextend, argv[2]);
      exit (1);
    }
  
    gfits_free_header (outheader);
    gfits_free_table (outtable);

    Nextend ++;
  }
  if (fclose (fi)) myQuit ("error closing input file %s\n", argv[1]);
  if (fclose (fo)) myQuit ("error closing output file %s\n", argv[1]);

  fprintf (stderr, "copied %d extensions from %s into %s\n", Nextend, argv[1], argv[2]);

  exit (0);
}

# include <ohana.h>
# include <gfitsio.h>
# include "inttypes.h"
# include "mysql.h"

# define SWAP_BYTE {					\
    char tmp;						\
    tmp = Pin[0]; Pin[0] = Pin[1]; Pin[1] = tmp; }
# define SWAP_WORD {					\
    char tmp;						\
    tmp = Pin[0]; Pin[0] = Pin[3]; Pin[3] = tmp;	\
    tmp = Pin[1]; Pin[1] = Pin[2]; Pin[2] = tmp; }
# define SWAP_DBLE {					\
    char tmp;						\
    tmp = Pin[0]; Pin[0] = Pin[7]; Pin[7] = tmp;	\
    tmp = Pin[1]; Pin[1] = Pin[6]; Pin[6] = tmp;	\
    tmp = Pin[2]; Pin[2] = Pin[5]; Pin[5] = tmp;	\
    tmp = Pin[3]; Pin[3] = Pin[4]; Pin[4] = tmp; }

# define DEBUG 1
# define NMAX    0x100
# define NBUFFER 0x100000
# define NROWS 10000
# define MAX_BUFFER 0x080000

# define TO_BUFFER 1

typedef enum {
  TYPE_NONE,  
  TYPE_CHAR,  
  TYPE_BYTE,  
  TYPE_SHORT, 
  TYPE_INT,   
  TYPE_INT64, 
  TYPE_FLOAT, 
  TYPE_DOUBLE,
  TYPE_SEQUENCE,
} TypeInt;

// generate a structure to describe the table:
typedef struct {
  char *rawname; // name of the column in the fits header
  char *colname; // name of the column to be used in the db
  char *format;
  TypeInt type;
  int Nval;
  int Nbytes;
} ColumnDef;

char        *DATABASE_HOST;
char        *DATABASE_USER;
char        *DATABASE_PASS;
char        *DATABASE_NAME;
char        *TABLENAME;
char        *TABLEPREFIX;
char        *TABLESUFFIX;
char        *SEQUENCE_COLUMN;
int          SEQUENCE_COLUMN_START;

int args (int *argc, char **argv);
void usage();

void FreeColumnDef (ColumnDef *columns, int Ncolumns);
ColumnDef *parse_table (Header *header, int *ncolumn);
int create_table (ColumnDef *columns, int Ncolumns, char *extname, IOBuffer *insert, MYSQL *mysql);
void write_data (ColumnDef *columns, int Ncolumns, int start, FTable *table, IOBuffer *insert, MYSQL *mysql);

void *gfits_get_bintable_column_data_by_Ncol (FTable *table, int Nfield, char *type, off_t *Nrow, int *Ncol, char nativeOrder);
int CopyIOBuffer (IOBuffer *output, IOBuffer *input);

void replace_dot_with_underscore (char *string);

void dump_on_failure (IOBuffer *buffer);
MYSQL *mysql_connect (MYSQL *mysqlBase);
int mysql_commit_buffer (IOBuffer *buffer, MYSQL *mysql);

int main (int argc, char **argv) {

  args (&argc, argv);
  if (argc != 2) usage ();
  char *input = argv[1];

  MYSQL  mysqlBase;
  MYSQL *mysqlReal = mysql_connect (&mysqlBase);
  if (!mysqlReal) {
    fprintf (stderr, "failed to connect to mysql\n");
    exit (1);
  }

  FTable table;
  Header header;
  table.header = &header;
  table.buffer = NULL;

  FILE *f = fopen (input, "r");
  if (f == (FILE *) NULL) {
    fprintf (stderr, "can't open file %s\n", input);
    exit (1);
  }

  int Nsection = 0;
  int Ntable = 0;
  while (gfits_fread_header (f, &header)) {

    /* extract the EXTNAME for this component (set to PHU for 0th component) */
    char extname[80];
    int status = gfits_scan (&header, "EXTNAME", "%s", 1, extname);
    if (!status) {
      if (Nsection == 0) {
	strcpy (extname, "PHU");
      } else {
	strcpy (extname, "UNKNOWN");
      }
    }
    fprintf (stdout, "%-30s ", extname);

    /* extract the datatype for this component (IMAGE for 0th component) */
    char exttype[80];
    if (Nsection == 0) {
      strcpy (exttype, "IMAGE");
    } else {
      status = gfits_scan (&header, "XTENSION", "%s", 1, exttype);
      if (!status) {
	strcpy (exttype, "UNKNOWN");
      }
    }
    fprintf (stdout, "%-15s ", exttype);

    fprintf (stdout, " %4d", header.Naxes);

    /* extract the individual axes */
    int i;
    for (i = 0; i < header.Naxes; i++) {
      fprintf (stdout, " "OFF_T_FMT, header.Naxis[i]);
    }
    fprintf (stdout, "\n");

    if (strcmp(exttype, "BINTABLE")) {
      off_t Nbytes = gfits_data_size (&header);
      gfits_free_header (&header);
      fseeko (f, Nbytes, SEEK_CUR);
      Nsection ++;
      continue;
    }

    int Ncolumns;
    ColumnDef *columns = parse_table (&header, &Ncolumns);

    IOBuffer insert;
    InitIOBuffer (&insert, 1024);

    if ((Ntable > 0) && TABLENAME) {
      fprintf (stderr, "WARNING: table name is supplied but more than one table is read: earlier tables will be replaced\n");
    } 
    create_table (columns, Ncolumns, extname, &insert, mysqlReal);

    // need to reset this on each pass?
    myAssert (!table.buffer, "oops");

    // total number of bytes in the data section (including pad at the end)
    off_t Nbytes = gfits_data_size (&header);
    
    off_t Ntotal = header.Naxis[1];
    int Nrows = NROWS;
    int start = 0;
    while (start < Ntotal) {
      if (!gfits_fread_ftable_range (f, FALSE, TRUE, &table, start, Nrows)) {
	fprintf (stderr, "error reading table for extension %d\n", Nsection);
	exit (1);
      }
      fprintf (stderr, "read "OFF_T_FMT" rows\n", header.Naxis[1]);

      write_data (columns, Ncolumns, start, &table, &insert, mysqlReal);

      start += Nrows;
      Nbytes -= header.Naxis[0]*header.Naxis[1]*abs(header.bitpix / 8);
      header.Naxis[1] = Ntotal;
    }

    gfits_free_table (&table);
    gfits_free_header (&header);

    // move the pointer to the end of this data segment
    fseeko (f, Nbytes, SEEK_CUR);
    Nsection ++;
    Ntable ++;

    FreeColumnDef (columns, Ncolumns);
    FreeIOBuffer (&insert);
  }
  gfits_free_header (&header);

  fclose (f);

  FREE (DATABASE_HOST);
  FREE (DATABASE_USER);
  FREE (DATABASE_PASS);
  FREE (DATABASE_NAME);
  FREE (TABLENAME);
  FREE (TABLEPREFIX);
  FREE (TABLESUFFIX);
  FREE (SEQUENCE_COLUMN);

  ohana_memdump (TRUE);

  exit (0);
}

void FreeColumnDef (ColumnDef *columns, int Ncolumns) {

  int i;

  if (!columns) return;
  for (i = 0; i < Ncolumns; i++) {
    FREE (columns[i].rawname);
    FREE (columns[i].colname);
    FREE (columns[i].format);
  }
  FREE (columns);
  return;
}

ColumnDef *parse_table (Header *header, int *ncolumn) {

  int i, j;

  int Ncolumn = 0;
  int NCOLUMN = 50;
  ColumnDef *column = NULL;
  ALLOCATE (column, ColumnDef, NCOLUMN);

  // print out the columns (need to swap first)

  int Nfields;
  if (!gfits_scan (header, "TFIELDS", "%d", 1, &Nfields)) return NULL;

  // generate the sequence column first so that any actual table columns which match the
  // requested name get a uniquified colname
  if (SEQUENCE_COLUMN) {
    column[Ncolumn].colname = strcreate (SEQUENCE_COLUMN);
    column[Ncolumn].rawname = strcreate (SEQUENCE_COLUMN);
    column[Ncolumn].format = NULL;
    replace_dot_with_underscore (column[Ncolumn].colname);
    column[Ncolumn].Nval = 0;
    column[Ncolumn].Nbytes = 0;
    column[Ncolumn].type = TYPE_SEQUENCE;
    Ncolumn++;
  }

  // first, extract the table data:
  for (i = 0; i < Nfields; i++) {

    char field[256], tlabel[256], format[256];
    snprintf_nowarn (field, 256, "TTYPE%d", i+1);
    gfits_scan (header, field, "%s", 1, tlabel);

    snprintf_nowarn (field, 256, "TFORM%d", i+1);
    gfits_scan (header, field, "%s", 1, format);

    // some special names must be protected:
    if (!strcasecmp (tlabel, "index")) { strcpy (tlabel, "index_"); }
    if (!strcasecmp (tlabel, "table")) { strcpy (tlabel, "table_"); }

    // check for duplicates
    int Ndup = 0;
    for (j = 0; j < Ncolumn; j++) {
      if (strcmp (column[j].rawname, tlabel)) continue;
      Ndup ++;
    }
    if (Ndup) {
      char tmpname[256];
      snprintf_nowarn (tmpname, 256, "%s_%d", tlabel, Ndup);
      column[Ncolumn].colname = strcreate (tmpname);
    } else {
      column[Ncolumn].colname = strcreate (tlabel);
    }
    column[Ncolumn].rawname = strcreate (tlabel);
    column[Ncolumn].format = strcreate (format);
    replace_dot_with_underscore (column[Ncolumn].colname);

    char type[16];
    int Nval, Nbytes;
    if (!gfits_bintable_format (format, type, &Nval, &Nbytes)) return NULL;
    column[Ncolumn].Nval = Nval;
    column[Ncolumn].Nbytes = Nbytes;

    // save the type as well
    if (!strcmp (type, "byte"))    column[Ncolumn].type = TYPE_BYTE;
    if (!strcmp (type, "short"))   column[Ncolumn].type = TYPE_SHORT;
    if (!strcmp (type, "int"))     column[Ncolumn].type = TYPE_INT;
    if (!strcmp (type, "int64_t")) column[Ncolumn].type = TYPE_INT64;
    if (!strcmp (type, "float"))   column[Ncolumn].type = TYPE_FLOAT;
    if (!strcmp (type, "double"))  column[Ncolumn].type = TYPE_DOUBLE;
    if (!strcmp (type, "char"))    column[Ncolumn].type = TYPE_CHAR;

    Ncolumn ++;
    if (Ncolumn == NCOLUMN) {
      NCOLUMN += 100;
      REALLOCATE (column, ColumnDef, NCOLUMN);
    }
  }
  *ncolumn = Ncolumn;
  return column;
}

/* print a double an IOBuffer (sprintf_double) */
int PrintIOBuffer_double (IOBuffer *buffer, double value) {

  /* add the output line to the given IOBuffer */
  
  char output[128]; // I can use a fixed-length buffer since sprintf_double prints no more than 22 bytes;

  output[0] = 0x27;
  int Nbyte = sprintf_double (&output[1], value);
  output[Nbyte+1] = 0x27; // 'asdf' : 012345
  output[Nbyte+2] = 0;
  Nbyte += 2;

  if (buffer[0].Nbuffer + Nbyte + 1 >= buffer[0].Nalloc) {
    buffer[0].Nalloc = buffer[0].Nbuffer + Nbyte + 64;
    REALLOCATE (buffer[0].buffer, char, buffer[0].Nalloc);
  }

  strcpy (&buffer[0].buffer[buffer[0].Nbuffer], output);
  buffer[0].Nbuffer += Nbyte; // buffer[Nbuffer] is the EOL null byte

  return (TRUE);
}

/* float to an IOBuffer (sprintf_float) */
int PrintIOBuffer_float (IOBuffer *buffer, float value) {

  /* add the output line to the given IOBuffer */
  
  char output[128]; // I can use a fixed-length buffer since sprintf_float prints no more than 14 bytes;

  output[0] = 0x27;
  int Nbyte = sprintf_float (&output[1], value);
  output[Nbyte+1] = 0x27; // 'asdf' : 012345
  output[Nbyte+2] = 0;
  Nbyte += 2;

  if (buffer[0].Nbuffer + Nbyte + 1 >= buffer[0].Nalloc) {
    buffer[0].Nalloc = buffer[0].Nbuffer + Nbyte + 64;
    REALLOCATE (buffer[0].buffer, char, buffer[0].Nalloc);
  }

  strcpy (&buffer[0].buffer[buffer[0].Nbuffer], output);
  buffer[0].Nbuffer += Nbyte; // buffer[Nbuffer] is the EOL null byte

  return (TRUE);
}

void PrintColumns (IOBuffer *buffer, char *typename, char *rootname, int Nval) {
  if (Nval > 1) {
    int j;
    char colname[64];
    for (j = 0; j < Nval; j++) {
      snprintf_nowarn (colname, 64, "%s_%d", rootname, j + 1);
      PrintIOBuffer (buffer, "%s %s", colname, typename);
      if (j < Nval - 1) {
	PrintIOBuffer (buffer, ",\n");
      }
    }
  } else {
    PrintIOBuffer (buffer, "%s %s", rootname, typename);
  }
  return;
}

// the 'insert' buffer will be used to generate the insert lines downstream (call InitIOBuffer on it first)
int create_table (ColumnDef *columns, int Ncolumns, char *extname, IOBuffer *insert, MYSQL *mysql) {

  int i;

  IOBuffer buffer;
  InitIOBuffer (&buffer, 1024);

  char *tablename = NULL;
  int length = TABLENAME ? strlen(TABLENAME) : strlen(extname);
  if (TABLEPREFIX) length += strlen(TABLEPREFIX);
  if (TABLESUFFIX) length += strlen(TABLESUFFIX);
  ALLOCATE_ZERO (tablename, char, length + 1);
  if (TABLEPREFIX) strcpy (tablename, TABLEPREFIX);
  if (TABLENAME) {
    strcat (tablename, TABLENAME);
  } else {
    strcat (tablename, extname);
  }
  if (TABLESUFFIX) strcat (tablename, TABLESUFFIX);
  replace_dot_with_underscore (tablename);

  // some special names must be protected:
  if (!strcasecmp (tablename, "index")) {
    free (tablename);
    tablename = strcreate ("index_");
  }
  if (!strcasecmp (tablename, "table")) {
    free (tablename);
    tablename = strcreate ("table_");
  }

  // drop the table if it exists
  PrintIOBuffer (&buffer, "DROP TABLE if exists %s\n", tablename);
  int status = mysql_query(mysql, buffer.buffer); 
  if (status) {
    fprintf (stderr, "failed to drop table:\n");
    fprintf (stderr, "%s\n", mysql_error(mysql));
    dump_on_failure (&buffer);
    return FALSE;
  }
  buffer.Nbuffer = 0;
  bzero (buffer.buffer, buffer.Nalloc);

  // Only send the necessary fields (eg, do not sent parallax and pm)
  PrintIOBuffer (&buffer, "CREATE TABLE %s (\n", tablename);
  PrintIOBuffer (insert, "INSERT INTO %s (\n", tablename);

  // first, extract the table data:
  for (i = 0; i < Ncolumns; i++) {

    // watch out for Nval != 1
    switch (columns[i].type) {
      case TYPE_BYTE:
	PrintColumns (&buffer, "SMALLINT", columns[i].colname, columns[i].Nval);
	break;
      case TYPE_SHORT:
	PrintColumns (&buffer, "SMALLINT", columns[i].colname, columns[i].Nval);
	break;
      case TYPE_INT:
	PrintColumns (&buffer, "INT", columns[i].colname, columns[i].Nval);
	break;
      case TYPE_INT64:
	PrintColumns (&buffer, "BIGINT", columns[i].colname, columns[i].Nval);
	break;
      case TYPE_FLOAT:
	PrintColumns (&buffer, "FLOAT", columns[i].colname, columns[i].Nval);
	break;
      case TYPE_DOUBLE:
	PrintColumns (&buffer, "DOUBLE", columns[i].colname, columns[i].Nval);
	break;
      case TYPE_CHAR:
	PrintIOBuffer (&buffer, "%s VARCHAR(%d)", columns[i].colname, columns[i].Nval);
	break;
      case TYPE_SEQUENCE:
	// SEQUENCE is a (single) special column with a sequence of values from SEQUENCE_COLUMN_START
	PrintIOBuffer (&buffer, "%s INT", columns[i].colname);
	break;
      default:
	myAbort ("oops");
    }

    if ((columns[i].Nval > 1) && (columns[i].type != TYPE_CHAR)) {
      int j;
      char colname[64];
      for (j = 0; j < columns[i].Nval; j++) {
	snprintf_nowarn (colname, 64, "%s_%d", columns[i].colname, j + 1);
	PrintIOBuffer (insert, "%s", colname);
	if (j < columns[i].Nval - 1) {
	  PrintIOBuffer (insert, ",\n");
	}
      }
    } else {
      PrintIOBuffer (insert, "%s", columns[i].colname);
    }

    if (i == Ncolumns - 1)  {
      PrintIOBuffer (&buffer, ")\n");
      PrintIOBuffer (insert, ") VALUES \n");
    } else {
      PrintIOBuffer (&buffer, ",\n");
      PrintIOBuffer (insert, ",\n");
    }
  }

  status = mysql_query(mysql, buffer.buffer); 
  if (status) {
    fprintf (stderr, "failed to create table:\n");
    fprintf (stderr, "%s\n", mysql_error(mysql));
    fprintf (stderr, "Nbuffer: %d\n", buffer.Nbuffer);
    dump_on_failure (&buffer);
  }

  FreeIOBuffer (&buffer);
  free (tablename);

  return TRUE;
}

void write_data (ColumnDef *columns, int Ncolumns, int start, FTable *table, IOBuffer *insert, MYSQL *mysql) {

  int i, j, Nf;

  // copy the 'insert' IOBuffer to the output buffer
  IOBuffer output;
  InitIOBuffer (&output, 1024);
  CopyIOBuffer (&output, insert);

  int Ny = table->header->Naxis[1];

  ALLOCATE_PTR (data, void *, Ncolumns);

  // first, extract the table data: (probably should save this structure and re-use in create_table)
  int Nfield = 1;
  for (i = 0; i < Ncolumns; i++) {
    off_t Nrow;
    int Ncol;
    char type[16];

    // we need to skip past virtual columns that do not correspond to actual table columns
    if (columns[i].type == TYPE_SEQUENCE) { data[i] = NULL; continue; }
    data[i] = gfits_get_bintable_column_data_by_Ncol (table, Nfield, type, &Nrow, &Ncol, FALSE);
    Nfield ++;
    myAssert (Ncol == columns[i].Nval, "oops (1)");
    myAssert (Nrow == Ny, "oops (2)");
  }

  // convert the type characters to an enum for speed here
  // could also generate pointers of all types to the data;

  for (i = 0; i < Ny; i++) {
    PrintIOBuffer (&output, "( ");
    for (Nf = 0; Nf < Ncolumns; Nf++) {
      switch (columns[Nf].type) {
	case TYPE_SEQUENCE:
	  // XXX need to include offset from table block and possible starting offset
	  PrintIOBuffer (&output, "%d", i + start + SEQUENCE_COLUMN_START);
	  break;
	case TYPE_CHAR: {
	  char line[1024];
	  char *value = data[Nf];
	  memset (line, 0, 128);
	  memcpy (line, &value[i*columns[Nf].Nval], columns[Nf].Nval);
	  PrintIOBuffer (&output, "'%s'", line);
	  break;
	} 
	case TYPE_BYTE: {
	  char *value = data[Nf];
	  for (j = 0; j < columns[Nf].Nval; j++) {
	    PrintIOBuffer (&output, "%hhd", value[i*columns[Nf].Nval + j]);
	    if (j < columns[Nf].Nval - 1) {
	      PrintIOBuffer (&output, ",\n");
	    }
	  }
	  break;
	}
	case TYPE_SHORT: {
	  short *value = data[Nf];
	  for (j = 0; j < columns[Nf].Nval; j++) {
	    PrintIOBuffer (&output, "%hd", value[i*columns[Nf].Nval + j]);
	    if (j < columns[Nf].Nval - 1) {
	      PrintIOBuffer (&output, ",\n");
	    }
	  }
	  break;
	}
	case TYPE_INT: {
	  int *value = data[Nf];
	  for (j = 0; j < columns[Nf].Nval; j++) {
	    PrintIOBuffer (&output, "%d", value[i*columns[Nf].Nval + j]);
	    if (j < columns[Nf].Nval - 1) {
	      PrintIOBuffer (&output, ",\n");
	    }
	  }
	  break;
	}
	case TYPE_INT64: {
	  int64_t *value = data[Nf];
	  for (j = 0; j < columns[Nf].Nval; j++) {
	    PrintIOBuffer (&output, "%lld", (long long int) value[i*columns[Nf].Nval + j]);
	    if (j < columns[Nf].Nval - 1) {
	      PrintIOBuffer (&output, ",\n");
	    }
	  }
	  break;
	}
	case TYPE_FLOAT: {
	  float *value = data[Nf];
	  for (j = 0; j < columns[Nf].Nval; j++) {
	    float myValue = value[i*columns[Nf].Nval + j];
	    if (isfinite(myValue)) {
	      PrintIOBuffer_float (&output, myValue);
	    } else {
	      PrintIOBuffer (&output, "NULL");
	    }
	    if (j < columns[Nf].Nval - 1) {
	      PrintIOBuffer (&output, ",\n");
	    }
	  }
	  break;
	}
	case TYPE_DOUBLE: {
	  double *value = data[Nf];
	  for (j = 0; j < columns[Nf].Nval; j++) {
	    double myValue = value[i*columns[Nf].Nval + j];
	    if (isfinite(myValue)) {
	      PrintIOBuffer_double (&output, myValue);
	    } else {
	      PrintIOBuffer (&output, "NULL");
	    }
	    if (j < columns[Nf].Nval - 1) {
	      PrintIOBuffer (&output, ",\n");
	    }
	  }
	  break;
	}
	default:
	  myAbort ("impossible");
      }
      if (Nf < Ncolumns - 1) {
	PrintIOBuffer (&output, ",\n");
      } else {
	PrintIOBuffer (&output, "),\n");
      }
    }
    if (output.Nbuffer > MAX_BUFFER) {
      mysql_commit_buffer (&output, mysql);
      CopyIOBuffer (&output, insert);
    }
  }

  mysql_commit_buffer (&output, mysql);
  FreeIOBuffer (&output);
  
  for (i = 0 ; i < Ncolumns; i++) {
    free (data[i]);
  }
  free (data);
  return;
}

void *gfits_get_bintable_column_data_by_Ncol (FTable *table, int Nfield, char *type, off_t *Nrow, int *Ncol, char nativeOrder) {

  int i;
  char tlabel[256], field[256], format[256], tmpline[64];
  char *Pin, *Pout, *array;
  double Bscale, Bzero;

  Header *header = table->header;

  int Nfields;
  if (!gfits_scan (header, "TFIELDS", "%d", 1, &Nfields)) return NULL;

  if (Nfield < 1) return NULL;
  if (Nfield > Nfields) return NULL;

  Bscale = 1; 
  Bzero  = 0;

  /* get column keywords */
  snprintf_nowarn (field, 256, "TTYPE%d", Nfield);
  gfits_scan (header, field, "%s", 1, tlabel);
  snprintf_nowarn (field, 256, "TSCAL%d", Nfield);
  gfits_scan (header, field, "%lf", 1, &Bscale);
  snprintf_nowarn (field, 256, "TZERO%d", Nfield);
  gfits_scan (header, field, "%lf", 1, &Bzero);
  snprintf_nowarn (field, 256, "TFORM%d", Nfield);
  gfits_scan (header, field, "%s", 1, format);

  int Nval, Nbytes;
  if (!gfits_bintable_format (format, type, &Nval, &Nbytes)) return (NULL);
  
  /* check existing table dimensions */
  off_t Nx, Ny;
  gfits_scan (header, "NAXIS1",  OFF_T_FMT, 1,  &Nx);
  gfits_scan (header, "NAXIS2",  OFF_T_FMT, 1,  &Ny);

  /* scan columns to find insert point */
  int Nstart = 0;
  for (i = 1; i < Nfield; i++) {
    snprintf_nowarn (field, 256, "TFORM%d", i);
    gfits_scan (header, field, "%s", 1, format);
    int Nv, Nb;
    gfits_bintable_format (format, tmpline, &Nv, &Nb);
    Nstart += Nv*Nb;
  }

  /* extract bytes from table into array */
  ALLOCATE (array, char, Nbytes*Nval*Ny);
  Pin  = table[0].buffer + Nstart;
  Pout = array;
  for (i = 0; i < Ny; i++, Pin += Nx, Pout += Nval*Nbytes) {
    memcpy (Pout, Pin, Nval*Nbytes);
  }

  // NOTE: we have already copied the data to 'array', 
  Pin  = array;
  Pout = array;
  int directCopy = (Bzero == 0.0) && (Bscale == 1.0);

  /* convert data in-situ with correct type, byte swap and Bzero/Bscale */
  if (!strcmp (type, "char")) {
    for (i = 0; i < Nval*Ny; i++, Pin+=Nbytes, Pout+=Nbytes) {
      if (!directCopy) { *(char *)Pout = *(char *)Pin*Bscale + Bzero; }
    }
  }
  if (!strcmp (type, "byte")) {
    for (i = 0; i < Nval*Ny; i++, Pin+=Nbytes, Pout+=Nbytes) {
      if (!directCopy) { *(char *)Pout = *(char *)Pin*Bscale + Bzero; }
    }
  }
  if (!strcmp (type, "short")) {
    for (i = 0; i < Nval*Ny; i++, Pin+=Nbytes, Pout+=Nbytes) {
# ifdef BYTE_SWAP
      if (!nativeOrder) { SWAP_BYTE; }
# endif
      if (!directCopy) { *(short *)Pout = *(short *)Pin*Bscale + Bzero; }
    }  
  }
  if (!strcmp (type, "int")) {
    for (i = 0; i < Nval*Ny; i++, Pin+=Nbytes, Pout+=Nbytes) {
# ifdef BYTE_SWAP
      if (!nativeOrder) { SWAP_WORD; }
# endif
      if (!directCopy) { *(int *)Pout = *(int *)Pin*Bscale + Bzero; }
    }
  }
  if (!strcmp (type, "int64_t")) {
    for (i = 0; i < Nval*Ny; i++, Pin+=Nbytes, Pout+=Nbytes) {
# ifdef BYTE_SWAP
      if (!nativeOrder) { SWAP_DBLE; }
# endif
      if (!directCopy) { *(int64_t *)Pout = *(int64_t *)Pin*Bscale + Bzero; }
    }
  }
  if (!strcmp (type, "float")) {
    for (i = 0; i < Nval*Ny; i++, Pin+=Nbytes, Pout+=Nbytes) {
# ifdef BYTE_SWAP
      if (!nativeOrder) { SWAP_WORD; }
# endif
      if (!directCopy) { *(float *)Pout = *(float *)Pin*Bscale + Bzero; }
    }
  }
  if (!strcmp (type, "double")) {
    for (i = 0; i < Nval*Ny; i++, Pin+=Nbytes, Pout+=Nbytes) {
# ifdef BYTE_SWAP
      if (!nativeOrder) { SWAP_DBLE; }
# endif
      if (!directCopy) { *(double *)Pout = *(double *)Pin*Bscale + Bzero; }
    }
  }

  // check that we supplied a valid type

  *Ncol = Nval;
  *Nrow = Ny;
  return (array);
}

int CopyIOBuffer (IOBuffer *output, IOBuffer *input) {

  output[0].Nalloc  = input[0].Nalloc; 
  output[0].Nreset  = input[0].Nreset; 
  output[0].Nblock  = input[0].Nblock; 
  output[0].Nbuffer = input[0].Nbuffer;

  if (output->buffer) {
    REALLOCATE (output[0].buffer, char, output[0].Nalloc);
  } else {
    ALLOCATE (output[0].buffer, char, output[0].Nalloc);
  }

  memcpy (output->buffer, input->buffer, input->Nbuffer);

  return (TRUE);
}

int args (int *argc, char **argv) {

  int N;

  if ((N = get_argument (*argc, argv, "-dbhost"))) {
    remove_argument (N, argc, argv);
    DATABASE_HOST = strcreate (argv[N]);
    remove_argument (N, argc, argv);
  } else usage();

  if ((N = get_argument (*argc, argv, "-dbuser"))) {
    remove_argument (N, argc, argv);
    DATABASE_USER = strcreate (argv[N]);
    remove_argument (N, argc, argv);
  } else usage();

  if ((N = get_argument (*argc, argv, "-dbpass"))) {
    remove_argument (N, argc, argv);
    DATABASE_PASS = strcreate (argv[N]);
    remove_argument (N, argc, argv);
  } else usage();

  if ((N = get_argument (*argc, argv, "-dbname"))) {
    remove_argument (N, argc, argv);
    DATABASE_NAME = strcreate (argv[N]);
    remove_argument (N, argc, argv);
  } else usage();

  TABLENAME = NULL;
  if ((N = get_argument (*argc, argv, "-tablename"))) {
    remove_argument (N, argc, argv);
    TABLENAME = strcreate (argv[N]);
    remove_argument (N, argc, argv);
  }

  TABLEPREFIX = NULL;
  if ((N = get_argument (*argc, argv, "-tableprefix"))) {
    remove_argument (N, argc, argv);
    TABLEPREFIX = strcreate (argv[N]);
    remove_argument (N, argc, argv);
  }
  TABLESUFFIX = NULL;
  if ((N = get_argument (*argc, argv, "-tablesuffix"))) {
    remove_argument (N, argc, argv);
    TABLESUFFIX = strcreate (argv[N]);
    remove_argument (N, argc, argv);
  }

  SEQUENCE_COLUMN = NULL;
  if ((N = get_argument (*argc, argv, "-sequence-column"))) {
    remove_argument (N, argc, argv);
    SEQUENCE_COLUMN = strcreate (argv[N]);
    remove_argument (N, argc, argv);
  }
  SEQUENCE_COLUMN_START = 0;
  if ((N = get_argument (*argc, argv, "-sequence-column-start"))) {
    remove_argument (N, argc, argv);
    SEQUENCE_COLUMN_START = atoi (argv[N]);
    remove_argument (N, argc, argv);
  }

  return TRUE;
}

void usage () {
  fprintf (stderr, "USAGE: fits_to_mysql [dbinfo] (filename.fits)\n");
  fprintf (stderr, "    mysql database info is supplied with these:\n");
  fprintf (stderr, "    -dbhost : database host machine\n");
  fprintf (stderr, "    -dbuser : database username\n");
  fprintf (stderr, "    -dbpass : database password\n");
  fprintf (stderr, "    -dbname : database name\n\n");
  exit (2);
}

int dump_result (MYSQL *connection) {

  MYSQL_RES *result = mysql_store_result (connection);
  if (!result) return FALSE;
  int Nrows = mysql_num_rows(result);
  int Ncols = mysql_num_fields(result);

  int i, j;
  for (j = 0; j < Nrows; j++) {
    MYSQL_ROW row = mysql_fetch_row(result);
    for (i = 0; i < Ncols; i++) {
      fprintf (stderr, "%s ", row[i]);
    }
    fprintf (stderr, "\n");
  }
  mysql_free_result (result);
  return (TRUE);
}

// DATABASE_* are supplied on the command line and are global variables in dvopsps.h
MYSQL *mysql_connect (MYSQL *mysqlBase) {

  char query[256];

  mysql_init (mysqlBase);
  MYSQL *connection = mysql_real_connect (mysqlBase, DATABASE_HOST, DATABASE_USER, DATABASE_PASS, DATABASE_NAME, 0, 0, 0);

  if (connection == NULL) {
    fprintf (stderr, "failed to connect to database\n");
    fprintf (stderr, "%s\n", mysql_error (mysqlBase));
    return NULL;
  }
    
  sprintf (query, "set @@interactive_timeout = 30000;");
  if (mysql_query (connection, query)) {
    fprintf (stderr, "failed to set interactive timout\n");
    fprintf (stderr, "%s\n", mysql_error (connection));
    return NULL;
  }
  dump_result (connection);
    
  sprintf (query, "set @@wait_timeout = 30000;");
  if (mysql_query (connection, query)) {
    fprintf (stderr, "failed to set wait timout\n");
    fprintf (stderr, "%s\n", mysql_error (connection));
    return NULL;
  }
  dump_result (connection);
    
  // sprintf (query, "set max_allowed_packet = %d;", 2*MAX_BUFFER);
  // if (mysql_query (connection, query)) {
  //   fprintf (stderr, "failed to set max_allowed_packet\n");
  //   fprintf (stderr, "%s\n", mysql_error (connection));
  //   return NULL;
  // }
  // dump_result (connection);
    
  // sprintf (query, "show variables like 'max_allowed_packet';");
  // if (mysql_query (connection, query)) {
  //   fprintf (stderr, "failed to set max_allowed_packet\n");
  //   fprintf (stderr, "%s\n", mysql_error (connection));
  //   return NULL;
  // }
  // dump_result (connection);
    
  if (0) {
    sprintf (query, "set autocommit=0;");
    if (mysql_query (connection, query)) {
      fprintf (stderr, "failed to turn off autocommit\n");
      fprintf (stderr, "%s\n", mysql_error (connection));
      return NULL;
    }
    dump_result (connection);
  }
    
  return connection;
}

int mysql_commit_buffer (IOBuffer *buffer, MYSQL *mysql) {

  MYSQL_RES *result;

  // check that the last two chars are ,\n and replace with ;\n
  if (!strcmp(&buffer->buffer[buffer->Nbuffer-2], ",\n")) {
    buffer->buffer[buffer->Nbuffer-2] = ';';
  } else {
    fprintf (stderr, "invalid sql?\n");
    return FALSE;
  }

  // XXX check return status
  if (mysql) {
    int status;
    status = mysql_query(mysql, buffer->buffer); 
    if (status) {
      fprintf (stderr, "error with insert:\n");
      fprintf (stderr, "%s\n", mysql_error(mysql));
      fprintf (stderr, "Nbuffer: %d\n", buffer->Nbuffer);
      dump_on_failure (buffer);
    }
    result = mysql_store_result (mysql);
    if (result) mysql_free_result (result);
  }

  return TRUE;
}

void dump_on_failure (IOBuffer *buffer) {

  if (!DEBUG) return;

  FILE *f = fopen ("dvopsps.dump.sql", "w");
  fwrite (buffer->buffer, 1, buffer->Nbuffer, f);
  fclose (f);
  exit (1);
}

void replace_dot_with_underscore (char *string) {

  // convert "." in string to "_"
  char *p = string;
  while (p && *p) {
    if (*p == '.') *p = '_';
    p++;
  }

  return;
}

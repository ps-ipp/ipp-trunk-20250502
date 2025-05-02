# include <ohana.h>
# include <gfitsio.h>
# include "inttypes.h"

# define NMAX    0x100
# define NBUFFER 0x100000
# define NROWS 10000

# define TO_BUFFER 1

# if (TO_BUFFER) 
# define FPRINT(FILE,...) {						\
  Nout = snprintf (&buffer[Nbuffer], NMAX, __VA_ARGS__);		\
  /* fprintf (stderr, __VA_ARGS__); fprintf (stderr, "\n"); */		\
  /* fprintf (stderr, "Nbuf: %d, Nout: %d\n", Nbuffer, Nout);*/		\
  Nbuffer += Nout; }						
# else
// # define FPRINT(FILE,...) { fprintf (FILE, __VA_ARGS__); }
# define FPRINT(FILE,...) if (0) fprintf (FILE, __VA_ARGS__)
# endif


void usage();

void print_data (FTable *table);
void *gfits_get_bintable_column_data_by_Ncol (FTable *table, int Nfield, char *type, off_t *Nrow, int *Ncol, char nativeOrder);
void FFPRINT (char *buffer, int *nbuffer, double value);

int main (int argc, char **argv) {

  if (0) {

    int Nout;
    int Nbuffer = 0;
    ALLOCATE_PTR (buffer, char, NBUFFER);
    memset (buffer, 0, NBUFFER);
    
    srand48(1);

    FILE *f1 = fopen ("test1.txt", "w");
    FILE *f2 = fopen ("test2.txt", "w");

    int i;
    for (i = 0; i < 20; i++) {
      float v = 1000*(drand48() - 0.5);

      FFPRINT (buffer, &Nbuffer, v);
      FPRINT (stdout, "\n");

      fprintf (f1, "%+.6e\n", v);
    }

    fwrite (buffer, 1, Nbuffer, f2);
    fclose (f1);
    fclose (f2);

    exit (0);
  }

  if (argc != 2) usage ();
  char *input = argv[1];

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

      print_data (&table);

      start += Nrows;
      Nbytes -= header.Naxis[0]*header.Naxis[1]*abs(header.bitpix / 8);
      header.Naxis[1] = Ntotal;
    }

    gfits_free_table (&table);
    gfits_free_header (&header);

    // move the pointer to the end of this data segment
    fseeko (f, Nbytes, SEEK_CUR);
    Nsection ++;
  }
  gfits_free_header (&header);

  fclose (f);

  ohana_memdump (TRUE);

  exit (0);
}

void usage () {
  fprintf (stderr, "USAGE: pspsconvert [-h] (table.fits) (outroot)\n");
  exit (2);
}

typedef enum {
  TYPE_NONE,  
  TYPE_CHAR,  
  TYPE_BYTE,  
  TYPE_SHORT, 
  TYPE_INT,   
  TYPE_INT64, 
  TYPE_FLOAT, 
  TYPE_DOUBLE
} TypeInt;

void print_data (FTable *table) {

  int i, j, Nf;

  // print out the columns (need to swap first)

  Header *header = table->header;
  int Ny = table->header->Naxis[1];

  int Nfields;
  if (!gfits_scan (header, "TFIELDS", "%d", 1, &Nfields)) return;

  ALLOCATE_PTR (Nrow, off_t,  Nfields);
  ALLOCATE_PTR (Ncol, int,    Nfields);
  ALLOCATE_PTR (data, void *, Nfields);
  ALLOCATE_PTR (type, char *, Nfields);
  ALLOCATE_PTR (Type, int,    Nfields);

  // first, extract the table data:
  for (i = 0; i < Nfields; i++) {
    ALLOCATE (type[i], char, 16);
    data[i] = gfits_get_bintable_column_data_by_Ncol (table, i + 1, type[i], &Nrow[i], &Ncol[i], FALSE);
    myAssert (Nrow[i] == Ny, "oops");
    Type[i] = TYPE_NONE;
    if (!strcmp (type[i], "char"))    Type[i] = TYPE_CHAR;
    if (!strcmp (type[i], "byte"))    Type[i] = TYPE_BYTE;
    if (!strcmp (type[i], "short"))   Type[i] = TYPE_SHORT;
    if (!strcmp (type[i], "int"))     Type[i] = TYPE_INT;
    if (!strcmp (type[i], "int64_t")) Type[i] = TYPE_INT64;
    if (!strcmp (type[i], "float"))   Type[i] = TYPE_FLOAT;
    if (!strcmp (type[i], "double"))  Type[i] = TYPE_DOUBLE;
    myAssert (Type[i] != TYPE_NONE, "oops");
  }

  // convert the type characters to an enum for speed here
  // could also generate pointers of all types to the data;

# if (TO_BUFFER)
  int Nout;
# endif
  int Nbuffer = 0;
  ALLOCATE_PTR (buffer, char, NBUFFER);
  memset (buffer, 0, NBUFFER);

  for (i = 0; i < Ny; i++) {
    for (Nf = 0; Nf < Nfields; Nf++) {
      switch (Type[Nf]) {
	case TYPE_CHAR: {
	  char line[1024];
	  char *value = data[Nf];
	  memset (line, 0, 128);
	  memcpy (line, &value[i*Ncol[Nf]], Ncol[Nf]);
	  FPRINT (stdout, "%s", line);
	  break;
	} 
	case TYPE_BYTE: {
	  char *value = data[Nf];
	  for (j = 0; j < Ncol[Nf]; j++) {
	    FPRINT (stdout, "%hhd", value[i*Ncol[Nf] + j]);
	  }
	  break;
	}
	case TYPE_SHORT: {
	  short *value = data[Nf];
	  for (j = 0; j < Ncol[Nf]; j++) {
	    FPRINT (stdout, "%hd", value[i*Ncol[Nf] + j]);
	  }
	  break;
	}
	case TYPE_INT: {
	  int *value = data[Nf];
	  for (j = 0; j < Ncol[Nf]; j++) {
	    FPRINT (stdout, "%d", value[i*Ncol[Nf] + j]);
	  }
	  break;
	}
	case TYPE_INT64: {
	  int64_t *value = data[Nf];
	  for (j = 0; j < Ncol[Nf]; j++) {
	    FPRINT (stdout, "%lld", (long long int) value[i*Ncol[Nf] + j]);
	  }
	  break;
	}
	case TYPE_FLOAT: {
	  float *value = data[Nf];
	  for (j = 0; j < Ncol[Nf]; j++) {
	    FFPRINT (buffer, &Nbuffer, value[i*Ncol[Nf] + j]);
	    // FPRINT (stdout, "%e", value[i*Ncol[Nf] + j]);
	  }
	  break;
	}
	case TYPE_DOUBLE: {
	  double *value = data[Nf];
	  for (j = 0; j < Ncol[Nf]; j++) {
	    FFPRINT (buffer, &Nbuffer, value[i*Ncol[Nf] + j]);
	    // FPRINT (stdout, "%e", value[i*Ncol[Nf] + j]);
	  }
	  break;
	}
	default:
	  myAbort ("impossible");
      }
      if (Nf < Nfields - 1) FPRINT (stdout, ",");
      if (Nbuffer >= NBUFFER - 2*NMAX) {
	fwrite (buffer, 1, Nbuffer, stdout);
	Nbuffer = 0;
	memset (buffer, 0, NMAX);
      }
    }
    FPRINT (stdout, "\n");
  }
# if (TO_BUFFER)
  fwrite (buffer, 1, Nbuffer, stdout);
# endif

  free (buffer);

  for (i = 0 ; i < Nfields; i++) {
    free (type[i]);
    free (data[i]);
  }
  free (Nrow);
  free (Ncol);
  free (data);
  free (type);
  free (Type);

  return;
}

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
  snprintf (field, 256, "TTYPE%d", Nfield);
  gfits_scan (header, field, "%s", 1, tlabel);
  snprintf (field, 256, "TSCAL%d", Nfield);
  gfits_scan (header, field, "%lf", 1, &Bscale);
  snprintf (field, 256, "TZERO%d", Nfield);
  gfits_scan (header, field, "%lf", 1, &Bzero);
  snprintf (field, 256, "TFORM%d", Nfield);
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
    snprintf (field, 256, "TFORM%d", i);
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

/** 
	char *ptr = line;
	while (*ptr) {
	  if (*ptr == '\n') {
	    *ptr = 0;
	    break;
	  }
	  ptr ++;
	}

 **/

void FFPRINT (char *buffer, int *nbuffer, double value) {

  // I want a fast way to print a float in %e format.

  // use a fixed format 
  // +F.ffffffe+NN

  int Nbuffer = *nbuffer;

  if (value == 0.0) {
    strcpy (&buffer[Nbuffer], "0.0+e00");
    Nbuffer += 7; 
    *nbuffer = Nbuffer;
    return;
  }						

  int isPositiveValue = (value > 0.0);

  double pvalue = fabs(value);

  int isPositiveExp = (pvalue >= 1.0);

  int iExponent = (int)(log10(pvalue));
  if (!isPositiveExp) iExponent --;
  
  double power = pow (10.0, (double) iExponent);

  double Mantissa = pvalue / power;

  int iMantissa = Mantissa * 10000000;
  
  char output[14];

  // +1.234567e+01 -> 1234567
  // 0123456789012 == 6543210

  output[0] = isPositiveValue ? '+' : '-';

  int roundUp = (iMantissa % 10 > 4);
  // fprintf (stderr, "%f : %d : %d\n", value, iMantissa, roundUp);

  iMantissa = iMantissa / 10;
  if (roundUp) iMantissa ++;

  int i;
  for (i = 0; i < 8; i++) {
    if (i == 6) {
      output[8-i] = '.';
      continue;
    }
    output[8-i] = '0' + iMantissa % 10;
    iMantissa = iMantissa / 10;
  }
    
  output[9] = 'e';
  output[10] = isPositiveExp ? '+' : '-';

  for (i = 0; i < 2; i++) {
    output[12-i] = '0' + iExponent % 10;
    iExponent = iExponent / 10;
  }
  output[13] = 0;
  
  memcpy (&buffer[Nbuffer], output, 14);
  Nbuffer += 13;
  *nbuffer = Nbuffer;
}

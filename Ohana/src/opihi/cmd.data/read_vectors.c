# include "data.h"
int dump_raw_table (FTable *table, char *message);
int dump_cmp_table (FTable *table, char *message);
void gfits_uncompress_timing ();

FILE *f = (FILE *) NULL;
char filename[2048];

void read_vectors_cleanup ();
int read_table_sizes (Header *header, int isCompressed);
int read_table_fields (Header *header, int isCompressed, int VERBOSE);
void list_extnames (int VERBOSE);

int datafile (int argc, char **argv) {

  if (argc < 2) {
    gprint (GP_ERR, "USAGE: datafile (filename)\n");
    return (FALSE);
  }
  
  if (strlen(argv[1]) >= 2048) {
    gprint (GP_ERR, "filename %s is too long\n", argv[1]);
    return (FALSE);
  }

  strcpy (filename, argv[1]);
  if (f != (FILE *) NULL) { fclose (f); }
  f = fopen (filename, "r");
  if (f == (FILE *) NULL) {
    gprint (GP_ERR, "failed to open file %s\n", filename);
    return (FALSE);
  }
  return (TRUE);
}

// vector types
enum {COLTYPE_NONE, COLTYPE_FLT, COLTYPE_INT, COLTYPE_TIME, COLTYPE_DATE, COLTYPE_CHAR, COLTYPE_STR, COLTYPE_HMS};

static int      Nvec     = 0;
static Vector **vec      = NULL;
static char   **listname = NULL;
static int     *col      = NULL;
static int     *coltype  = NULL;
static char    *buffer   = NULL;

int read_vectors (int argc, char **argv) {
  
  int TimeFormat;
  time_t TimeReference;
  int i, j, Nskip, Narg, IsCSV, VERBOSE;
  int Nbytes, NELEM, nread;
  char *colstr, *c0, *c1, *extname;
  char varname[1024];  // used as a buffer for the names of string fields

  /* auto-sense table type */
  if ((Narg = get_argument (argc, argv, "-fits"))) {
    remove_argument (Narg, &argc, argv);
    extname = strcreate (argv[Narg]);
    if (extname == (char *) NULL) return (FALSE);
    remove_argument (Narg, &argc, argv);
    int status = read_table_vectors (argc, argv, extname);
    free (extname);
    return (status);
  }

  Nskip = 0;
  if ((Narg = get_argument (argc, argv, "-skip"))) {
    remove_argument (Narg, &argc, argv);
    Nskip = atof (argv[Narg]);
    remove_argument (Narg, &argc, argv);
  }

  IsCSV = FALSE;
  if ((Narg = get_argument (argc, argv, "-csv"))) {
    remove_argument (Narg, &argc, argv);
    IsCSV = TRUE;
  }

  VERBOSE = FALSE;
  if ((Narg = get_argument (argc, argv, "-v"))) {
    remove_argument (Narg, &argc, argv);
    VERBOSE = TRUE;
  }

  if ((argc < 3) || !(argc % 2)) {
    gprint (GP_ERR, "USAGE: read name N name N ...\n");
    gprint (GP_ERR, "  options:\n");
    gprint (GP_ERR, "     -fits EXTNAME : read a fits file from the given extension (vector names are column names)\n");
    gprint (GP_ERR, "     -v : verbose mode\n");
    gprint (GP_ERR, "     -csv : comma-separated values (columns may be identified in excel-style: A, AC\n");
    gprint (GP_ERR, "     -skip N : skip N lines before reading\n");
    gprint (GP_ERR, "     column names may include a type: name:type\n");
    gprint (GP_ERR, "       type is int, float, char, str, time, date, hms\n");
    gprint (GP_ERR, "       for char, values are placed into a list $name:0 - $name:n\n");
    gprint (GP_ERR, "       for str, values are placed into a string-typed vector\n");
    gprint (GP_ERR, "       for hms, values are sexigesimal values HH:MM:SS (good for degrees as well)\n");
    gprint (GP_ERR, "       for date, values are human-readable date strings YYYY/MM/DD\n");
    gprint (GP_ERR, "       for time, values are human-readable date/time strings YYYY/MM/DD,hh:mm:ss\n");
    gprint (GP_ERR, "         date & time values are stored as a float using the TIMEFORMAT, TIMEREF values for the conversion\n");

    gprint (GP_ERR, "     -fits options:\n");
    gprint (GP_ERR, "       -transpose : read rows as columns and vice-versa\n");

    gprint (GP_ERR, "       -keyword (FIELD) : use FIELD to identify the extension (default: EXTNAME)\n");
    gprint (GP_ERR, "       -pad-if-short : allow reading of broken files by padding missing data\n");
    gprint (GP_ERR, "       -sizes : return a list of table size information ($table:Nx, $table:Ny, $table:$Nfields)  \n");
    gprint (GP_ERR, "       -list-fields : return a list of table columns ($tfields:0 to $tfields:n) (use -v to see the list)\n");
    gprint (GP_ERR, "       -extnum (N) : use the number to find the Nth extension (ignore EXTNAME)\n");
    gprint (GP_ERR, "       -range (start) (Nrows) : read the specified portion of the file\n");
    gprint (GP_ERR, "        -v : verbose \n");
    gprint (GP_ERR, "        -char-vectors : char fields will be saved as vectors NAME:0 -- NAME:n for n characters \n");

    return (FALSE);
  }
  /* read name N name N  */

  // do this only optionally?
  GetTimeFormat (&TimeReference, &TimeFormat);

  if (f == (FILE *) NULL) {
    gprint (GP_ERR, "no open file for read\n");
    return (FALSE);
  }
  fseeko (f, 0LL, SEEK_SET);

  buffer = NULL;

  Nvec = (argc - 1) / 2;
  ALLOCATE (listname, char *, Nvec);
  ALLOCATE (vec, Vector *, Nvec);
  ALLOCATE (col, int, Nvec);
  ALLOCATE (coltype, int, Nvec);
  for (i = 0; i < Nvec; i++) {
    listname[i] = NULL;
    vec[i] = NULL;
  }

  for (i = 0; i < Nvec; i++) {

    // interpret the column names including type flags
    // XXX review the grammar before releasing this : is foo:type best, or is something else needed?
    // Note the conflict wrt list entries
    // the name may be of the form foo:type, where type may be one of : int, float, time
    
    coltype[i] = COLTYPE_FLT;
    char *colname = argv[2*i + 1];
    char *ptr = strchr (colname, ':');
    if (ptr) {
      // split out colname and type
      *ptr = 0;
      ptr ++;
      if (!ptr) goto bad_colname;
      coltype[i] = COLTYPE_NONE;
      if (!strcasecmp(ptr, "float")) { coltype[i] = COLTYPE_FLT; }
      if (!strcasecmp(ptr, "int"))   { coltype[i] = COLTYPE_INT; }
      if (!strcasecmp(ptr, "char"))  { coltype[i] = COLTYPE_CHAR; }
      if (!strcasecmp(ptr, "str"))   { coltype[i] = COLTYPE_STR; }
      if (!strcasecmp(ptr, "date"))  { coltype[i] = COLTYPE_DATE; }
      if (!strcasecmp(ptr, "time"))  { coltype[i] = COLTYPE_TIME; }
      if (!strcasecmp(ptr, "hms"))   { coltype[i] = COLTYPE_HMS; }
      if (!coltype[i]) goto bad_colname;
    }

    if (coltype[i] == COLTYPE_CHAR) {
      listname[i] = strcreate (argv[2*i + 1]);
    } else {
      if ((vec[i] = SelectVector (argv[2*i + 1], ANYVECTOR, TRUE)) == NULL) {
	gprint (GP_ERR, "USAGE: read name N name N ...\n");
	read_vectors_cleanup();
	return (FALSE);    
      }
    }

    colstr = argv[2*i+2];

    // numerical columns, e.g., 15, must be base 10 and not have other characters (15x)
    if (isdigit(colstr[0])) {
      char *endptr;
      long colnum_tmp = strtol(colstr, &endptr, 10);
      if (endptr[0]) {
	gprint (GP_ERR, "unexpected characters in column number: %s\n", endptr);
	goto bad_colname;
      }
      col[i] = colnum_tmp;
      continue;
    }

    // allow 'excel' columns names of the form A-Z, AA-AZ, BA-BZ, .. ZA-ZZ
    for (j = 0; j < strlen (colstr); j++) {
      if (strlen(colstr) >= 3) goto bad_colname;
      if (colstr[0] < 'A') goto bad_colname;
      if (colstr[0] > 'Z') goto bad_colname;
      if (colstr[1] && colstr[1] < 'A') goto bad_colname;
      if (colstr[1] && colstr[1] > 'Z') goto bad_colname;

      col[i] = colstr[0] - 'A' + 1;
      if (colstr[1]) {
	col[i] *= 26;
	col[i] += colstr[1] - 'A' + 1;
      }
      continue;

    bad_colname:
      gprint (GP_ERR, "USAGE: read name N name N ...\n");
      read_vectors_cleanup();
      return (FALSE);    
    }
  }

  // currently, all read vectors are forced to be type FLT
  NELEM = 1000;
  for (i = 0; i < Nvec; i++) {
    switch (coltype[i]) {
      case COLTYPE_INT:
	ResetVector (vec[i], OPIHI_INT, NELEM);
	break;
      case COLTYPE_FLT:
      case COLTYPE_HMS:
      case COLTYPE_TIME:
      case COLTYPE_DATE:
	ResetVector (vec[i], OPIHI_FLT, NELEM);
	break;
      case COLTYPE_STR:
	ResetVector (vec[i], OPIHI_STR, NELEM);
	break;
      case COLTYPE_CHAR:
      default:
	break;
    }
  }
  
  // we allocate one extra byte into which we never read so there will always be a NULL terminating the string
  ALLOCATE (buffer, char, 0x10001);
  bzero (buffer, 0x10001);

  int Nline_read = 0; // track number of lines read so far (use to skip lines as well)

  // we have a working buffer read from the file. we parse the lines in the working buffer
  // until we reach the last chunk without an EOL char.  at that point, we shift the start
  // of the last (partial) line to the start of the buffer and re-fill.

  // we treat \n\r pair as a single EOL char to handle mac files:

  int Nstart = 0; // location of the last valid byte in the buffer (start filling here)
  int Nelem = 0; // number of valid rows read (vector elements)
  int EndOfFile = FALSE;
  while (!EndOfFile) {
    Nbytes = 0x10000 - Nstart;
    // we have allocated one extra byte into which we never read so there will always be a NULL terminating the string
    bzero (&buffer[Nstart], Nbytes + 1);
    nread = fread (&buffer[Nstart], 1, Nbytes, f);
    if (ferror (f)) {
      perror ("error reading data file");
      break;
    }
    // we still need to parse the rest of the buffer, but there might not be an EOL on the last line
    if (nread == 0) {
      EndOfFile = TRUE;
    }
    
    int bufferStatus = TRUE; 
    c0 = buffer; // c0 always marks the start of a line
    while (bufferStatus) {
      c1 = strchr (c0, '\n'); // find the end of this current line (also valid for a Mac: \r\n)
      if (!c1) {
	c1 = strchr (c0, '\r'); // try \r for Windows files
      }
      if (!c1) {
	Nstart = strlen (c0);
	if (EndOfFile) {
	  // if we have reached EOF, we need to do one last pass in case there is a line without a return
	  c1 = c0 + Nstart;
	  bufferStatus = FALSE;
	  if (Nstart == 0) continue; // if we have reached EOF and c0 points at the last valid character, we are done
	} else {
	  // if we have not reached EOF, we need to shift the buffer to the start of this line and read more data
	  memmove (buffer, c0, Nstart);
	  bufferStatus = FALSE;
	  continue;
	}
      }
      *c1 = 0; // mark the end of the line 
      Nline_read ++;

      // skip to the next line (but if EOF, do not overrun buffer)
      if (Nline_read <= Nskip) { if (!EndOfFile) { c0 = c1 + 1; } continue; }
      if (*c0 == '#')          { if (!EndOfFile) { c0 = c1 + 1; } continue; }
      if (*c0 == '!')          { if (!EndOfFile) { c0 = c1 + 1; } continue; }

      // parse the vectors in this line.  this code is a bit inefficient: each column
      // requires a separate pass through the line.

      int lineStatus = TRUE;
      for (i = 0; i < Nvec; i++) {
	int ivalue;
	double dvalue;
	time_t tvalue;
	int readStatus = FALSE;
	int dataStatus = FALSE;
	// need to make the if cases for coltype[i]
	switch (coltype[i]) {
	  case COLTYPE_INT:
	    readStatus = IsCSV ? iparse_csv (&ivalue, col[i], c0) : iparse (&ivalue, col[i], c0);
	    vec[i][0].elements.Int[Nelem] = readStatus ? ivalue : 0;
	    break;
	  case COLTYPE_CHAR:
	    {
	      // I need to get an isolated word in 'value' with the string value of this field 
	      char *ptr = IsCSV ? ptrparse_csv (col[i], c0) : ptrparse (col[i], c0);
	      char *value = NULL;
	      if (IsCSV) {
		char *end = parse_nextword_csv (ptr);
		if (end) {
		  value = end ? strncreate (ptr, end - ptr) : strcreate (ptr);
		}
	      } else {
		value = thisword(ptr);
	      }
	      set_list_varname (varname, listname[i], Nelem, FALSE);
	      set_str_variable (varname, value);
	      free (value);
	      break;
	    }
	  case COLTYPE_STR:
	    {
	      // I need to get an isolated word in 'value' with the string value of this field 
	      char *ptr = IsCSV ? ptrparse_csv (col[i], c0) : ptrparse (col[i], c0);
	      char *value = NULL;
	      if (IsCSV) {
		char *end = parse_nextword_csv (ptr);
		if (end) {
		  value = end ? strncreate (ptr, end - ptr) : strcreate (ptr);
		}
	      } else {
		value = thisword(ptr);
	      }
	      if (value == NULL) {value = strcreate ("");} // we need at least an empty string not a NULL
	      vec[i][0].elements.Str[Nelem] = value; // needs to be freed later.
	      break;
	    }
	  case COLTYPE_FLT:
	    readStatus = IsCSV ? dparse_csv (&dvalue, col[i], c0) : dparse (&dvalue, col[i], c0);
	    vec[i][0].elements.Flt[Nelem] = readStatus ? dvalue : NAN;
	    break;
	  case COLTYPE_TIME:
	    readStatus = IsCSV ? tparse_csv (&tvalue, col[i], c0) : tparse (&tvalue, col[i], c0);
	    dvalue = TimeValue (tvalue, TimeReference, TimeFormat);
	    vec[i][0].elements.Flt[Nelem] = readStatus ? dvalue : NAN;
	    break;
	  case COLTYPE_DATE: {
	    char *string = NULL;
	    if (IsCSV) {
	      char *ptr = ptrparse_csv (col[i], c0);
	      string = strcreate (ptr); // create separate string (NULL-safe)
	      ptr = (string == NULL) ? NULL : strchr (string, ',');
	      if (ptr != NULL) *ptr = 0; // place an EOL here so parsing does not go past the comma
	    } else {
	      char *ptr = ptrparse (col[i], c0); // NULL-safe
	      string = getword (ptr); // NULL-safe
	    }	      
	    tvalue = ohana_date_to_sec (string); // NULL-safe (returns 0)
	    dvalue = TimeValue (tvalue, TimeReference, TimeFormat);
	    vec[i][0].elements.Flt[Nelem] = string ? dvalue : NAN;
	    FREE (string);
	    break;
	  }
	  case COLTYPE_HMS: {
	    char *ptr = IsCSV ? ptrparse_csv (col[i], c0) : ptrparse (col[i], c0);
	    char *string = strcreate (ptr); // make a copy so we can munge it in ohana_dms_to_ddd
	    dataStatus = ohana_dms_to_ddd (&dvalue, string);
	    vec[i][0].elements.Flt[Nelem] = ((string != NULL) && dataStatus) ? dvalue : NAN;
	    FREE (string);
	    break;
	  }
	}
	if (!readStatus && VERBOSE) {
	  if (IsCSV) {
	    gprint (GP_ERR, "suspect field: %d (%s) in %s\n", col[i], argv[2*i+2], c0);
	  } else {
	    gprint (GP_ERR, "suspect field: %d in %s\n", col[i], c0);
	  }
	}
	lineStatus &= readStatus;
      }
      if (!lineStatus && VERBOSE) {
	char temp[32];
	strncpy_nowarn (temp, c0, 31);
	gprint (GP_ERR, "skip line %s\n\n", temp);
      }
      Nelem ++;
      if (Nelem == NELEM) {
	NELEM += 1000;
	for (i = 0; i < Nvec; i++) {
	  switch (coltype[i]) {
	    case COLTYPE_INT:
	      ResetVector (vec[i], OPIHI_INT, NELEM);
	      break;
	    case COLTYPE_FLT:
	    case COLTYPE_HMS:
	    case COLTYPE_TIME:
	    case COLTYPE_DATE:
	      ResetVector (vec[i], OPIHI_FLT, NELEM);
	      break;
	    case COLTYPE_STR:
	      ResetVector (vec[i], OPIHI_STR, NELEM);
	      break;
	    case COLTYPE_CHAR:
	    default:
	      break;
	  }
	}
      }
      if (!EndOfFile) {
	c0 = c1 + 1;
      }
    }
  }
  // set the final vector / list length
  for (i = 0; i < Nvec; i++) {
    switch (coltype[i]) {
      case COLTYPE_INT:
	ResetVector (vec[i], OPIHI_INT, Nelem);
	break;
      case COLTYPE_FLT:
      case COLTYPE_HMS:
      case COLTYPE_TIME:
      case COLTYPE_DATE:
	ResetVector (vec[i], OPIHI_FLT, Nelem);
	break;
      case COLTYPE_STR:
	ResetVector (vec[i], OPIHI_STR, Nelem);
	break;
      case COLTYPE_CHAR:
	sprintf (varname, "%s:n", listname[i]);
	set_int_variable (varname, Nelem);
	break;
      default:
	break;
    }
  }
  read_vectors_cleanup();
  return (TRUE);
}

# undef ESCAPE
# define ESCAPE(...) {		\
  gprint (GP_ERR, __VA_ARGS__); \
  if (CCDKeyword != NULL) free (CCDKeyword); \
  gfits_free_table  (&table); \
  gfits_free_header (&header); \
  return (FALSE);  }

int read_table_vectors (int argc, char **argv, char *extname) {

  off_t Nbytes;
  int i, j, N, Ny, Binary, vecType;
  char type[16], ID[80];
  FTable table;
  Header header;
  Vector **vec;

  table.buffer = NULL;
  header.buffer = NULL;

  int FITS_TRANSPOSE = FALSE;
  if ((N = get_argument (argc, argv, "-transpose"))) {
    remove_argument (N, &argc, argv);
    FITS_TRANSPOSE = TRUE;
  }

  char *CCDKeyword = NULL;
  if ((N = get_argument (argc, argv, "-keyword"))) {
    remove_argument (N, &argc, argv);
    CCDKeyword = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }

  int padIfShort = FALSE;
  if ((N = get_argument (argc, argv, "-pad-if-short"))) {
    remove_argument (N, &argc, argv);
    padIfShort = TRUE;
  }

  int getSizes = FALSE;
  if ((N = get_argument (argc, argv, "-sizes"))) {
    remove_argument (N, &argc, argv);
    getSizes = TRUE;
  }

  // note the order of -v -list-fields -q below:
  // VERBOSE = FALSE is the natural default, but if -list-fields is given, 
  // VERBOSE = TRUE becomes the default.  this can be overridden by -q
  // but to make that sequence work, we need to test for those three
  // options in that order: -v -list-fields -q
  // this is also true for -list (listExtnames)
  int VERBOSE = FALSE;
  if ((N = get_argument (argc, argv, "-v"))) {
    remove_argument (N, &argc, argv);
    VERBOSE = TRUE;
  }
  int listFields = FALSE;
  if ((N = get_argument (argc, argv, "-list-fields"))) {
    remove_argument (N, &argc, argv);
    listFields = TRUE;
    VERBOSE = TRUE;
  }
  int listExtnames = FALSE;
  if (!strcmp(extname, "-list")) {
    listExtnames = TRUE;
    VERBOSE = TRUE;
  }
  if ((N = get_argument (argc, argv, "-q"))) {
    remove_argument (N, &argc, argv);
    VERBOSE = FALSE;
  }

  int Nextend = -1;
  if ((N = get_argument (argc, argv, "-extnum"))) {
    remove_argument (N, &argc, argv);
    Nextend = atoi (extname);
  }

  int start = 0;
  int Nrows = -1; // -1 : read entire table
  if ((N = get_argument (argc, argv, "-range"))) {
    remove_argument (N, &argc, argv);
    start = atoi (argv[N]);
    remove_argument (N, &argc, argv);
    Nrows = atoi (argv[N]);
    remove_argument (N, &argc, argv);
  }

  // by default, we now store a string-type field as a string-type vector.

  // if CharAsList is selected (and My < 10000), char fields will be saved as $NAME:0 - $NAME:m for m rows

  // if CharAsVectors is selected, char fields will be saved as vectors NAME:0 -- NAME:n for n characters

  // if (Ny > 10000), force CharAsVectors
  int CharAsVectors = FALSE;
  if ((N = get_argument (argc, argv, "-char-vectors"))) {
    remove_argument (N, &argc, argv);
    CharAsVectors = TRUE;
  }
  int CharAsList = FALSE;
  if ((N = get_argument (argc, argv, "-char-list"))) {
    remove_argument (N, &argc, argv);
    CharAsList = TRUE;
  }

  // XXX ReadAll needs: deal with Extnum vs Extname, save vectors, etc
  // ReadAll = FALSE;
  // if ((N = get_argument (argc, argv, "-all"))) {
  //   remove_argument (N, &argc, argv);
  //   ReadAll = atoi (extname);
  //   if (argc != 1) ESCAPE ("-all option cannot be mixed with selected field");
  // }

  if ((argc < 2) && !getSizes && !listFields && !listExtnames) ESCAPE ("USAGE: read -fits extension [-extnum] [-keyword key] name name ...\n");
  if ((argc != 1) && getSizes)   ESCAPE ("USAGE: read -sizes -fits extension [-extnum] (does not read data values)\n");
  if ((argc != 1) && listFields) ESCAPE ("USAGE: read -list-fields -fits extension [-extnum] (does not read data values)\n");
  if ((argc != 1) && listExtnames) ESCAPE ("USAGE: read -list-fields -fits extension [-extnum] (does not read data values)\n");

  if (f == NULL) ESCAPE ("file not found\n");
  fseeko (f, 0LL, SEEK_SET);
  table.header = &header;

  if (listExtnames) {
    list_extnames (VERBOSE);
    return TRUE;
  }

  /**** find the appropriate extension and read header (if extname is a number, use count) ****/
  if (Nextend > -1) {
    // first extension is PHU, cannot be a table. 
    // Nextend counts from 0 for first extension
    if (!gfits_load_header (f, &header)) ESCAPE ("error reading primary header for file\n");
    Nbytes = gfits_data_size (&header);
    fseeko (f, Nbytes, SEEK_CUR);
    gfits_free_header (&header);

    for (i = 0; i < Nextend; i++) {
      if (!gfits_load_header (f, &header)) ESCAPE ("extension %d not found\n", i);
      Nbytes = gfits_data_size (&header);
      /* skip the prior data buffers */
      fseeko (f, Nbytes, SEEK_CUR);
      gfits_free_header (&header);
    }
    if (!gfits_load_header (f, &header)) ESCAPE ("error reading header for extension %d\n", Nextend);
  } else {
    if (CCDKeyword == NULL) {
      CCDKeyword = get_variable ("CCDKEYWORD");
    }
    if (CCDKeyword == NULL) {
      CCDKeyword = strcreate ("EXTNAME");
    }
    while (1) {
      if (!gfits_load_header (f, &header)) {
	gprint (GP_ERR, "extension %s not found in file\n", extname);
	if (CCDKeyword != NULL) free (CCDKeyword); 
	gfits_free_table  (&table); 
	gfits_free_header (&header); 
	return (TRUE);  
      }
      Nbytes = gfits_data_size (&header);

      if (!gfits_scan (&header, CCDKeyword, "%s", 1, ID)) {
	fseeko (f, Nbytes, SEEK_CUR);
	gfits_free_header (&header);
	continue;
      }
      if (strcmp (ID, extname)) {
	fseeko (f, Nbytes, SEEK_CUR);
	gfits_free_header (&header);
	continue;
      }
      break;
    }
  }

  int IsCompressed = gfits_extension_is_compressed_table (&header);

  if (getSizes) {
    read_table_sizes (&header, IsCompressed);
    if (CCDKeyword != NULL) free (CCDKeyword); 
    gfits_free_header (&header); 
    return TRUE;
  }
  if (listFields) {
    int tstatus = read_table_fields (&header, IsCompressed, VERBOSE);
    if (CCDKeyword != NULL) free (CCDKeyword); 
    gfits_free_header (&header); 
    if (!tstatus) { gprint (GP_ERR, "error reading table fields\n"); }
    return tstatus;
  }
  if (IsCompressed) {
    if ((start > 0) || (Nrows > -1)) {
      gprint (GP_ERR, "%s[%s] is compressed: must read entire table\n", filename, extname);
      gfits_free_header (&header); 
      return FALSE;
    }
  }

  if (Nrows == -1) {
    Nrows = header.Naxis[1] - start;
  }
  if (start < 0) ESCAPE ("invalid range: start < 0\n");
  if (start >= header.Naxis[1]) ESCAPE ("invalid range: start >= Ny ("OFF_T_FMT")\n", header.Naxis[1]);
  if (Nrows < 0) ESCAPE ("invalid range: Nrows < 0\n");

  // just a warning:
  if (start + Nrows > header.Naxis[1]) {
    if (VERBOSE) gprint (GP_ERR, "NOTE: reading last block will return only "OFF_T_FMT" rows\n", header.Naxis[1] - start);
  }

  // Ny = 100, start = 0, Nrows = -1 -> Nrows => 100
  // Ny = 100, start = 10, Nrows = 90

  if (IsCompressed) {
    if (!gfits_fread_ftable_data (f, &table, padIfShort)) ESCAPE ("error reading table for extension %d\n", Nextend);
  } else {
    // arg3 (FALSE) : seek to this segment start
    if (!gfits_fread_ftable_range (f, padIfShort, FALSE, &table, start, Nrows)) ESCAPE ("error reading table for extension %d\n", Nextend);
  }

  /* identify table type (ascii / binary) */
  Binary = FALSE;
  gfits_scan (&header, "XTENSION", "%s", 1, type);
  if (strcmp (type, "BINTABLE") && strcmp (type, "TABLE")) {
    ESCAPE ("specified extension %s is not a table\n", type);
  }
  Binary = !strcmp (type, "BINTABLE");
  Ny = header.Naxis[1];

  Header *outheader = &header;
  FTable *outtable  = &table;

  Header rawheader;
  FTable rawtable;
  if (IsCompressed) {
    rawtable.header = &rawheader;

    dump_cmp_table (&table, "rd cmp");

    int Nfields;
    if (!gfits_scan (&header, "TFIELDS", "%d", 1, &Nfields)) ESCAPE ("cannot find TFIELDS in header\n");
    for (i = 0; i < Nfields; i++) {
      gfits_byteswap_varlength_column (&table, i+1);
    }
    if (!gfits_uncompress_table (&table, &rawtable)) ESCAPE ("failed to uncompress table\n");
    gfits_uncompress_timing ();

    dump_raw_table (&rawtable, "rd raw");

    outheader = &rawheader;
    outtable  = &rawtable;
    gfits_free_header (&header);
    gfits_free_table (&table);
    Ny = rawheader.Naxis[1];
  }

  /* find columns which match requested vectors */
  for (i = 1; i < argc; i++) {
    void   *data;
    int Nval;
    char name[80];
      
    // for both BINARY and ASCII tables, Nval is the number of fields of the given type
    // e.g., there may be 3 related floating point fields fields or 21 connected ascii characters.

    Nval = 0;
    if (Binary) {
      if (!gfits_get_bintable_column_type (outheader, argv[i], type, &Nval)) ESCAPE ("requested field not found\n");
      if (!gfits_get_bintable_column_raw (outheader, outtable, argv[i], &data, IsCompressed)) ESCAPE ("error reading data from specified field\n");
    } else {
      if (!gfits_get_table_column_type (outheader, argv[i], type, &Nval)) ESCAPE ("requested field not found\n");
      if (!gfits_get_table_column (outheader, outtable, argv[i], &data)) ESCAPE ("error reading data from specified field\n");
    }
    if (Nval == 0) ESCAPE ("no data for field in table\n");
    
    vecType = OPIHI_INT;
    if (!strcmp (type, "double") || !strcmp (type, "float")) {
      vecType = OPIHI_FLT;
    }
	
    if (!FITS_TRANSPOSE) {
      // read string column into a list rather than a vector
      if (!strcmp (type, "char")) {
	// save char-type field as a List:
	if (CharAsList && (Ny < 3000)) {
	  char *fieldName = argv[i];
	  char *Ptr = data;
	  char varname[1024];  // used as a buffer for the names of string fields
	  for (j = 0; j < Ny; j++) {
	    set_list_varname (varname, fieldName, j, FALSE);
	    char *value = strncreate (&Ptr[j*Nval], Nval);
	    // replace instances of $ with _
	    char *p = strchr (value, '$');
	    while (p) { 
	      *p = '_';
	      p = strchr (p, '$');
	    }
	    set_str_variable (varname, value);
	    free (value);
	  }
	  sprintf (varname, "%s:n", fieldName);
	  set_int_variable (varname, Ny);
	  continue;
	}
	// save char-type field as a string-type vector:
	if (!CharAsVectors) {
	  Vector *myVector = NULL;
	  sprintf (name, "%s", argv[i]);
	  if ((myVector = SelectVector (name, ANYVECTOR, TRUE)) == NULL) ESCAPE ("bad vector name\n");
	  ResetVector (myVector, OPIHI_STR, Ny);

	  char *Ptr = data;
	  for (j = 0; j < Ny; j++) {
	    myVector[0].elements.Str[j] = strncreate (&Ptr[j*Nval], Nval);
	    // replace instances of $ with _
	    char *p = strchr (myVector[0].elements.Str[j], '$');
	    while (p) { *p = '_'; p = strchr (p, '$'); }
	  }
	  continue;
	}
      }

      // define the multifield vector names (Nval vectors x Ny elements)
      // CharAsVectors is handled below automatically
      ALLOCATE (vec, Vector *, Nval);
      for (j = 0; j < Nval; j++) {
	if (Nval == 1) 
	  sprintf (name, "%s", argv[i]);
	else
	  sprintf (name, "%s:%d", argv[i], j);
	if ((vec[j] = SelectVector (name, ANYVECTOR, TRUE)) == NULL) ESCAPE ("bad vector name\n");
	ResetVector (vec[j], vecType, Ny);
      }

      if (!VectorAssignData (vec, type, data, Ny, Nval)) ESCAPE ("bad column type %s\n", type);

    } else {
      // define the multifield vector names (Ny vectors x Nval elements)
      ALLOCATE (vec, Vector *, Ny);
      for (j = 0; j < Ny; j++) {
	if (Ny == 1) 
	  sprintf (name, "%s", argv[i]);
	else
	  sprintf (name, "%s:%d", argv[i], j);
	if ((vec[j] = SelectVector (name, ANYVECTOR, TRUE)) == NULL) ESCAPE ("bad vector name\n");
	ResetVector (vec[j], vecType, Nval);
      }

      if (!VectorAssignDataTranspose (vec, type, data, Ny, Nval)) ESCAPE ("bad column type %s\n", type);
    }
    free (data);
    free (vec);
  }
  if (CCDKeyword != NULL) free (CCDKeyword);
  gfits_free_table (outtable);
  gfits_free_header (outheader);
  return (TRUE);
}

void read_vectors_cleanup () {

  int i;

  if (col) free (col);
  if (coltype) free (coltype);
  if (buffer) free (buffer);
  if (listname) {
    for (i = 0; i < Nvec; i++) {
      if (listname[i]) free (listname[i]);
    }
    free (listname);
  }
  if (vec) free (vec);
}

// read -fits foo -sizes -- Nx, Ny, Nfields -> $table:Nx, $table:Ny, $table:$Nfields
// read -fits foo -fields 

int read_table_sizes (Header *header, int IsCompressed) {
  
  int Nfields;

  gfits_scan (header, "TFIELDS", "%d", 1, &Nfields);

  int Nx, Ny;
  if (IsCompressed) {
    if (!gfits_scan (header, "ZNAXIS1", "%d", 1, &Nx)) Nx = 0;
  } else {
    Nx = header->Naxis[0];
  }
  if (IsCompressed) {
    if (!gfits_scan (header, "ZNAXIS2", "%d", 1, &Ny)) Ny = 0;
  } else {
    Ny = header->Naxis[1];
  }

  set_int_variable ("table:Nx", Nx);
  set_int_variable ("table:Ny", Ny);
  set_int_variable ("table:Nfields", Nfields);

  return TRUE;
}

// read -fits foo -sizes -- Nx, Ny, Nfields -> $table:Nx, $table:Ny, $table:$Nfields
// read -fits foo -fields 

int read_table_fields (Header *header, int isCompressed, int VERBOSE) {
  
  int i, Nfields;
  char field[32], varname[32], type[80], comment[80], format[80], unit[80], null[80], nval[80];

  if (!gfits_scan (header, "TFIELDS", "%d", 1, &Nfields)) {
    if (VERBOSE) gprint (GP_ERR, "cannot read TFIELDS from header\n"); 
    set_int_variable ("table:Nx", header->Naxis[0]);
    set_int_variable ("table:Ny", header->Naxis[1]);
    set_int_variable ("table:Nfields", 0);
    set_int_variable ("tfields:n", 0);
    return TRUE;
  }
   
  set_int_variable ("table:Nx", header->Naxis[0]);
  set_int_variable ("table:Ny", header->Naxis[1]);
  set_int_variable ("table:Nfields", Nfields);
  set_int_variable ("tfields:n", Nfields);

  for (i = 1; i <= Nfields; i++) {
    memset (field,   0, 32);
    memset (type,    0, 80);
    memset (comment, 0, 80);
    memset (format,  0, 80);
    memset (unit,    0, 80);
    memset (null,    0, 80);
    memset (nval,    0, 80);

    snprintf (field, 32, "TTYPE%d", i);
    if (!gfits_scan (header, field, "%s", 1, type)) {
      if (VERBOSE) gprint (GP_ERR, "cannot read %s from header\n", field); 
    }
    
    snprintf (varname, 32, "tfields:%d", i - 1);
    set_str_variable (varname, type);

    gfits_scan_alt (header, field, "%C", 1, comment); 

    if (!isCompressed) {
      snprintf (field, 32, "TFORM%d", i);
    } else {
      snprintf (field, 32, "ZFORM%d", i);
    }
    if (!gfits_scan (header, field, "%s", 1, format)) {
      if (VERBOSE) gprint (GP_ERR, "cannot read %s from header\n", field); 
    }

    snprintf (field, 32, "TUNIT%d", i);
    gfits_scan (header, field, "%s", 1, unit);
    snprintf (field, 32, "TNULL%d", i);
    if (!gfits_scan (header, field, "%s", 1, null)) strcpy (null, "none");
    snprintf (field, 32, "TNVAL%d", i);
    if (!gfits_scan (header, field, "%s", 1, nval)) strcpy (nval, "none");

    if (VERBOSE) gprint (GP_LOG, "%-18s %-15s %-18s %-6s %-8s %-8s %-32s\n", type, varname, unit, format, null, nval, comment); 
  }
  return TRUE;
}

void list_extnames (int VERBOSE) {

  off_t Nbytes, Nelem;
  int i, Naxis, Ncomp, extend, status;
  char extname[82], varname[32], exttype[82], axisname[32];
  Header header;

  fseeko (f, 0, SEEK_SET);

  if (VERBOSE) gprint (GP_LOG, "%-30s %-15s %-15s NAXIS NAXIS(i)...\n", "extname", "varname", "datatype"); 

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
    if (VERBOSE) gprint (GP_LOG, "%-30s ", extname);

    snprintf (varname, 32, "extname:%d", Ncomp);
    set_str_variable (varname, extname);
    if (VERBOSE) gprint (GP_LOG, "%-15s ", varname);

    /* extract the datatype for this component (IMAGE for 0th component) */
    if (Ncomp == 0) {
      strcpy (exttype, "IMAGE");
    } else {
      status = gfits_scan (&header, "XTENSION", "%s", 1, exttype);
      if (!status) {
	strcpy (exttype, "UNKNOWN");
      }
    }
    if (VERBOSE) gprint (GP_LOG, "%-15s ", exttype);

    /* extract the rank of the component */
    status = gfits_scan (&header, "NAXIS",  "%d", 1, &Naxis);
    if (!status) {
      gprint (GP_ERR, "component %d is missing Naxis!\n", Ncomp);
      Ncomp ++;
      continue;
    }
    if (VERBOSE) gprint (GP_LOG, " %4d", Naxis);

    /* extract the individual axes */
    for (i = 0; i < Naxis; i++) {
      sprintf (axisname, "NAXIS%d", i+1);
      status = gfits_scan (&header, axisname,  OFF_T_FMT, 1,  &Nelem);
      if (!status) {
	gprint (GP_ERR, "missing %s\n", axisname);
      }
      if (VERBOSE) gprint (GP_LOG, " "OFF_T_FMT,  Nelem);
    }
    if (VERBOSE) gprint (GP_LOG, "\n");

    /* are extensions identified? (we will scan for them anyway) */
    if (Ncomp == 0) {
      extend = FALSE;
      gfits_scan_alt (&header, "EXTEND", "%t", 1, &extend);
      if (!extend) {
	gprint (GP_ERR, "no extensions listed in file\n");
      }
    }

    Nbytes = gfits_data_size (&header);
    fseeko (f, Nbytes, SEEK_CUR);

    Ncomp ++;
  }
  set_int_variable ("extname:n", Ncomp);
  return;
}

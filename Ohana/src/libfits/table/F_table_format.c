# include <ohana.h>
# include <gfitsio.h>

/***********************/
// get the format of a table column
// Nval : number of joined columns
// Nbytes : width of column, determined by the type (e.g., float = 4 bytes)
int gfits_bintable_format (char *format, char *type, int *Nval, int *Nbytes) {

  char *Fchar;

  if (format == (char *) NULL) return (FALSE); // format must be defined
  if (format[0] == 0) return (FALSE);	       // format must not be empty string
  // char *Lchar = &format[strlen(format) - 1];	       // last-char pointer

  int Nv = strtol (format, &Fchar, 10);            // Fchar points at end of leading number
  // if (Fchar != Lchar) return (FALSE);       -- this test is invalid for eg 1PB(nn)
  if ((Nv == 0) && (Fchar == format)) Nv = 1;

  // NOTE: X, L, B all are stored in 1-byte columns (X by default has at least room for 8 bits)
  // I report these as type 'byte' as opposed to 'char', which is interpreted as a string

  *Nbytes = 0;
  switch (*Fchar) {
  case  'X':
    { *Nbytes = 1;  strcpy (type, "gfbyte");   *Nval = 1 + (int) Nv / 8; }
    break;
  case  'L':
    { *Nbytes = 1;  strcpy (type, "gfbyte");   *Nval = Nv;               }
    break;
  case  'A':
    { *Nbytes = 1;  strcpy (type, "char");   *Nval = Nv;               }
    break;
  case  'B':
    { *Nbytes = 1;  strcpy (type, "gfbyte");   *Nval = Nv;               }
    break;
  case  'I':
    { *Nbytes = 2;  strcpy (type, "short");  *Nval = Nv;               }
    break;
  case  'J':
    { *Nbytes = 4;  strcpy (type, "int");    *Nval = Nv;               }
    break;
  case  'K':
    { *Nbytes = 8;  strcpy (type, "int64_t"); *Nval = Nv;              }
    break;
  case  'E':
    { *Nbytes = 4;  strcpy (type, "float");  *Nval = Nv;               }
    break;
  case  'D':
    { *Nbytes = 8;  strcpy (type, "double"); *Nval = Nv;               }
    break;
  case  'C':
    { *Nbytes = 8;  strcpy (type, "float");  *Nval = 2*Nv;             }
    break;
  case  'M':
    { *Nbytes = 16; strcpy (type, "double"); *Nval = 2*Nv;             }
    break;
  case  'P':
    { *Nbytes = 4;  strcpy (type, "var");    *Nval = 2*Nv;             }
    break;
  case  'Q':
    { *Nbytes = 8;  strcpy (type, "var64"); *Nval = 2*Nv;             }
    break;
  default:
    return (FALSE);
  }

  return (TRUE);
}

/* 
   valid BINTABLE column formats:
   L - logical
   X - bit
   I - 16 bit int
   J - 32 bit int
   K - 64 bit int
   A - char
   E - float 
   D - double
   B - unsigned bytes
   C - complex float
   M - complex double
   P - var length array descrpt (32 bit pointer)
   Q - var length array descrpt (64 bit pointer)
   
   all can be preceeded by integer N which specified number
   of entries in array
*/

/***********************/
// get the format of a table column
// Nval : number of fields (1 for all types except string)
// Nbytes : width of FITS table column, ie number of bytes in ASCII table
int gfits_table_format (char *format, char *type, int *Nval, int *Nbytes) {

  char Fchar, Size[80];
  int Nv;

  if (format == (char *) NULL) return (FALSE);
  if (format[0] == 0) return (FALSE);
  Fchar = format[0];

  Nv = strtol (&format[1], (char **) NULL, 10);
  if (Nv == 0) { 
    Nv = 1;
    strcpy (Size, "1");
  } else {
    strcpy (Size, &format[1]);
  }
  
  int isLong = FALSE;
  char Type = 'x';
  if (Fchar == 'D') { *Nbytes = Nv;  strcpy (type, "double"); Type = 'e'; *Nval =  1; isLong = TRUE; }
  if (Fchar == 'E') { *Nbytes = Nv;  strcpy (type, "float");  Type = 'e'; *Nval =  1; }
  if (Fchar == 'F') { *Nbytes = Nv;  strcpy (type, "float");  Type = 'f'; *Nval =  1; }
  if (Fchar == 'I') { *Nbytes = Nv;  strcpy (type, "int");    Type = 'd'; *Nval =  1; }
  if (Fchar == 'A') { *Nbytes =  1;  strcpy (type, "char");   Type = 's'; *Nval = Nv; }
  if (!*Nbytes) { return (FALSE); }
  
  if (0) {
    // this format is appropriate for scanning, but not printing
    if (isLong) {
      sprintf (format, "%%%sl%c", Size, Type);
    } else {
      sprintf (format, "%%%s%c", Size, Type);
    }
  }
  
  // this format is appropriate for printing, but not scanning
  sprintf (format, "%%-%s%c", Size, Type);

  return (TRUE);
}

/*
  valid TABLE column formats:
  FN.N  - floating point taking NN characters in the table
  INN   - integer taking NN characters in the table
  ANN   - NN char string
  
*/

/** extract a table subset to a vtable ***/
int gfits_table_to_vtable (FTable *ftable, VTable *vtable, off_t start, off_t Nkeep) {

  /* gfits_table_to_vtable (f, v, 0, Ny - 1) - keep all of table
     gfits_table_to_vtable (f, v, 0, 0)      - keep none of table 
  */  

  off_t i, Nx, Ny;

  gfits_scan (ftable[0].header, "NAXIS1", OFF_T_FMT, 1,  &Nx);
  gfits_scan (ftable[0].header, "NAXIS2", OFF_T_FMT, 1,  &Ny);
  
  if (start + Nkeep > Ny) return (FALSE);
  if (start < 0) return (FALSE);
  if (Nkeep < 0) return (FALSE);

  ALLOCATE (vtable[0].row, off_t, MAX (1, Nkeep));
  ALLOCATE (vtable[0].buffer, char *, MAX (1, Nkeep));
  for (i = 0; i < Nkeep; i++) {
    ALLOCATE (vtable[0].buffer[i], char, MAX (1, Nx));
    memcpy (vtable[0].buffer[i], &ftable[0].buffer[(i+start)*Nx], Nx);
    vtable[0].row[i] = i + start;
  }
  
  vtable[0].header = ftable[0].header;
  vtable[0].datasize = ftable[0].datasize;
  vtable[0].Nrow = Nkeep;
  vtable[0].pad = vtable[0].datasize - Nx*Ny;

  return (TRUE);
}

/** convert specified rows to vtable */
int gfits_vtable_from_ftable (FTable *ftable, VTable *vtable, off_t *row, off_t Nrow) {

  off_t i, N, Nx, Ny;

  gfits_scan (ftable[0].header, "NAXIS1", OFF_T_FMT, 1,  &Nx);
  gfits_scan (ftable[0].header, "NAXIS2", OFF_T_FMT, 1,  &Ny);

  /* make empty vtable from table */
  vtable[0].header   = ftable[0].header;  /* make this a copy? */
  vtable[0].datasize = ftable[0].datasize;
  vtable[0].pad      = vtable[0].datasize - Nx*Ny;
  vtable[0].Nrow     = Ny;

  /* insert selected rows in vtable (mask rows marked with -1) */ 
  ALLOCATE (vtable[0].row, off_t, Nrow);
  ALLOCATE (vtable[0].buffer, char *, Nrow);
  for (N = i = 0; i < Nrow; i++) {
    if (row[i] == -1) continue;
    vtable[0].row[N] = row[i];
    ALLOCATE (vtable[0].buffer[N], char, Nx);
    memcpy (vtable[0].buffer[N], &ftable[0].buffer[Nx*row[i]], Nx);
    N++;
  }
  vtable[0].Nrow = N;
  return (TRUE);
}

/* use table def to format a complete string */
char *gfits_table_print (FTable *table,...) { 
  
  off_t Nx, off;
  int i, Nchar, Nval, Nbytes, Nfields;
  char *line, format[64], field[64], type[64];
  va_list argp;
  
  va_start (argp, table);

  gfits_scan (table[0].header, "NAXIS1",  OFF_T_FMT, 1,  &Nx);
  gfits_scan (table[0].header, "TFIELDS", "%d", 1, &Nfields);

  ALLOCATE (line, char, Nx + 1);
  
  off = 0;
  for (i = 1; i <= Nfields; i++) {

    snprintf (field, 64, "TFORM%d", i);
    gfits_scan (table[0].header, field, "%s", 1, format);       /* get field format */
    gfits_table_format (format, type, &Nval, &Nbytes); /* convert to c-style */
    Nchar = Nval * Nbytes;
    if (!strcmp (type, "int"))   { 
      /* d = va_arg (argp, int); */
      snprintf (&line[off], Nchar + 1, format, va_arg (argp, int)); 
    }
    if (!strcmp (type, "float")) { 
      /* f = va_arg (argp, double); */
      snprintf (&line[off], Nchar + 1, format, va_arg (argp, double)); 
    }
    if (!strcmp (type, "char"))  { 
      /* c = va_arg (argp, char *); */
      snprintf (&line[off], Nchar + 1, format, va_arg (argp, char *));
    }
    off += Nchar;
  }
  va_end (argp);
  return (line);
}

// apply table tzero, tscal in situ (from storage to data)
int gfits_table_scale_data (FTable *ftable) {
  
  off_t Nx, Ny, off;
  int i, j, n, Nfields;
  int Nchar, Nval, Nbytes, status;
  char format[64], field[64], type[64];
  double tzero, tscale;
  char *tmpChar;
  short *tmpShort;
  int *tmpInt;
  int64_t *tmpInt64;

  off = 0;

  gfits_scan (ftable[0].header, "NAXIS1",  OFF_T_FMT, 1,  &Nx);
  gfits_scan (ftable[0].header, "NAXIS2",  OFF_T_FMT, 1,  &Ny);
  gfits_scan (ftable[0].header, "TFIELDS", "%d", 1, &Nfields);

  for (i = 1; i <= Nfields; i++) {
    snprintf (field, 64, "TFORM%d", i);
    gfits_scan (ftable[0].header, field, "%s", 1, format);       /* get field format */
    gfits_bintable_format (format, type, &Nval, &Nbytes); /* convert to c-style */
    Nchar = Nval * Nbytes;

    snprintf (field, 64, "TZERO%d", i);
    status = gfits_scan (ftable[0].header, field, "%lf", 1, &tzero);       /* get field format */
    if (!status) {
	off += Nchar; 
	continue;
    }

    snprintf (field, 64, "TSCAL%d", i);
    status = gfits_scan (ftable[0].header, field, "%lf", 1, &tscale);       /* get field format */
    if (!status) {
	off += Nchar; 
	continue;
    }
    if (tscale == 0.0) {
	off += Nchar; 
	continue;
    }
    if ((tscale == 1.0) && (tzero == 0.0)) {
	off += Nchar; 
	continue;
    }

    // Does it make sense to scale 'char' data (as opposed to 'byte')?
    if (!strcmp (type, "char"))   { 
      for (j = 0; j < Ny; j++) {
	for (n = 0; n < Nval; n++) {
	  tmpChar = (char *)&ftable[0].buffer[j*Nx + n*Nbytes + off];
	  *tmpChar = *tmpChar * tscale + tzero;
	}
      }
    }
    if (!strcmp (type, "gfbyte"))   { 
      for (j = 0; j < Ny; j++) {
	for (n = 0; n < Nval; n++) {
	  tmpChar = (char *)&ftable[0].buffer[j*Nx + n*Nbytes + off];
	  *tmpChar = *tmpChar * tscale + tzero;
	}
      }
    }
    if (!strcmp (type, "short"))   { 
      for (j = 0; j < Ny; j++) {
	for (n = 0; n < Nval; n++) {
	  tmpShort = (short *)&ftable[0].buffer[j*Nx + n*Nbytes + off];
	  *tmpShort = *tmpShort * tscale + tzero;
	}
      }
    }
    if (!strcmp (type, "int"))   { 
      for (j = 0; j < Ny; j++) {
	for (n = 0; n < Nval; n++) {
	  tmpInt = (int *)&ftable[0].buffer[j*Nx + n*Nbytes + off];
	  *tmpInt = *tmpInt * tscale + tzero;
	}
      }
    }
    if (!strcmp (type, "int64_t"))   { 
      for (j = 0; j < Ny; j++) {
        for (n = 0; n < Nval; n++) {
          tmpInt64 = (int64_t *)&ftable[0].buffer[j*Nx + n*Nbytes + off];
          *tmpInt64 = *tmpInt64 * tscale + tzero;
        }
      }
    }
    off += Nchar;
  }
  return (TRUE);
}

// apply table tzero, tscal in situ (from data to storage)
int gfits_table_scale_storage (FTable *ftable) {

  off_t Nx, Ny, off;
  int i, j, n, Nfields;
  int Nchar, Nval, Nbytes, status;
  char format[64], field[64], type[64];
  double tzero, tscale;
  char *tmpChar;
  short *tmpShort;
  int *tmpInt;
  int64_t *tmpInt64;

  off = 0;

  gfits_scan (ftable[0].header, "NAXIS1",  OFF_T_FMT, 1,  &Nx);
  gfits_scan (ftable[0].header, "NAXIS2",  OFF_T_FMT, 1,  &Ny);
  gfits_scan (ftable[0].header, "TFIELDS", "%d", 1, &Nfields);

  for (i = 1; i <= Nfields; i++) {
    snprintf (field, 64, "TFORM%d", i);
    gfits_scan (ftable[0].header, field, "%s", 1, format);       /* get field format */
    gfits_bintable_format (format, type, &Nval, &Nbytes); /* convert to c-style */
    Nchar = Nval * Nbytes;

    snprintf (field, 64, "TZERO%d", i);
    status = gfits_scan (ftable[0].header, field, "%lf", 1, &tzero);       /* get field format */
    if (!status) {
	off += Nchar; 
	continue;
    }

    snprintf (field, 64, "TSCAL%d", i);
    status = gfits_scan (ftable[0].header, field, "%lf", 1, &tscale);       /* get field format */
    if (!status) {
	off += Nchar; 
	continue;
    }
    if (tscale == 0.0) {
	off += Nchar; 
	continue;
    }
    if ((tscale == 1.0) && (tzero == 0.0)) {
	off += Nchar; 
	continue;
    }

    // does this make sense? (see note above)
    if (!strcmp (type, "char"))   { 
      for (j = 0; j < Ny; j++) {
	for (n = 0; n < Nval; n++) {
	  tmpChar = (char *)&ftable[0].buffer[j*Nx + n*Nbytes + off];
	  *tmpChar = (*tmpChar - tzero) / tscale;
	}
      }
    }
    if (!strcmp (type, "gfbyte"))   { 
      for (j = 0; j < Ny; j++) {
	for (n = 0; n < Nval; n++) {
	  tmpChar = (char *)&ftable[0].buffer[j*Nx + n*Nbytes + off];
	  *tmpChar = (*tmpChar - tzero) / tscale;
	}
      }
    }
    if (!strcmp (type, "short"))   { 
      for (j = 0; j < Ny; j++) {
	for (n = 0; n < Nval; n++) {
	  tmpShort = (short *)&ftable[0].buffer[j*Nx + n*Nbytes + off];
	  *tmpShort = (*tmpShort - tzero) / tscale;
	}
      }
    }
    if (!strcmp (type, "int"))   { 
      for (j = 0; j < Ny; j++) {
	for (n = 0; n < Nval; n++) {
	  tmpInt = (int *)&ftable[0].buffer[j*Nx + n*Nbytes + off];
	  *tmpInt = (*tmpInt - tzero) / tscale;
	}
      }
    }
    if (!strcmp (type, "int64_t"))   { 
      for (j = 0; j < Ny; j++) {
	for (n = 0; n < Nval; n++) {
	  tmpInt64 = (int64_t *)&ftable[0].buffer[j*Nx + n*Nbytes + off];
	  *tmpInt64 = *tmpInt64 * tscale + tzero;
	}
      }
    }
    off += Nchar;
  }
  return (TRUE);
}

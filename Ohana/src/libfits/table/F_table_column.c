# include <ohana.h>
# include <gfitsio.h>

/*********************** fits table column ****************************/
int gfits_table_column (Table *table, char *field, char *mode,...) {
/* we expect one more field: the pointer to the array we read in */

  char string[256], this_field[256], form[256], temp[256];
  int i, j, N, start, end, width, M;
  va_list argp;
  double **D;
  float  **F;
  char   ***C;
  int    **I;
  
  va_start (argp, mode);

  /* find the correct field */
  for (i = 0; i < table[0].Nfields; i++) {
    snprintf (string, 256, "TTYPE%d\0", i+1);
    gfits_scan (&table[0].header, string, "%s", 1, this_field);
    if (!strcmp (field, this_field)) {
      break;
    }
  }
  
  if (i == table[0].Nfields) {
    fprintf (stderr, "Table field %s does not exist\n", field);
    return (FALSE);
  }

  N = i + 1;

  snprintf (string, 256, "TBCOL%d\0", N);
  gfits_scan (&table[0].header, string, "%d", 1, &start);
  snprintf (string, 256, "TFORM%d\0", N);
  gfits_scan (&table[0].header, string, "%s", 1, form);
  /* we could use some error checking from the FITS table form, but
     it is not immediately crucial */

  if (N == table[0].Nfields) { 
    end = table[0].Naxis[0];
  } else {
    snprintf (string, 256, "TBCOL%d\0", N+1);
    gfits_scan (&table[0].header, string, "%d", 1, &end);
  }
  width = end - start;
  
  if (!strcmp (mode, "%d")) {
    I = va_arg (argp, int **);
    ALLOCATE ((*I), int, table[0].Naxis[1]);
    M = 0;
  }
  if (!strcmp (mode, "%f"))  {
    F = va_arg (argp, float **);
    ALLOCATE ((*F), float, table[0].Naxis[1]);
    M = 1;
  }
  if (!strcmp (mode, "%lf")) {
    D = va_arg (argp, double **);
    ALLOCATE ((*D), double, table[0].Naxis[1]);
    M = 2;
  }
  if (!strcmp (mode, "%s")) {
    C = va_arg (argp, char ***);
    ALLOCATE ((*C), char *, table[0].Naxis[1]);
    for (i = 0; i < table[0].Naxis[1]; i++) {
      ALLOCATE ((*C)[i], char, width + 1);
    }
    M = 3;
  }

  for (i = 0; i < table[0].Naxis[1]; i++) {
    strncpy_nowarn (temp, &table[0].buffer[i*table[0].Naxis[0] + start - 1], width);
    switch (M) {
    case 0:
      (*I)[i] = (int) atof (temp);
      break;
    case 1:
      (*F)[i] = atof (temp);
      break;
    case 2:
      (*D)[i] = atof (temp);
      break;
    case 3:
      strcpy ((*C)[i], temp);
      break;
    default:
      fprintf (stderr, "unknown gfits_table_column mode: %s\n", mode);
      return (FALSE);
    }
  }
  return (TRUE);
}

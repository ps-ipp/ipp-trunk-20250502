# include <ohana.h>
# include <gfitsio.h>

/*********************** fits get table column *****************************/
void gfits_get_table_column (table, X, mode, values) 
Table *table; 
int X; 
char *mode, **values;
{
  
  char Tform[256], field[256], tmp[256];
  int  start, Nchar, i, N;
  
  int    *Ivalues;
  long   *Lvalues;
  short  *Svalues;
  float  *Fvalues;
  double *Dvalues;
  char  **Cvalues;
  
  /* no error checking, all sorts of problems! */
  /* should I count fields from 0 or from 1? */
  
  snprintf (field, 256, "TBCOL%d\0", X);
  gfits_scan (&table[0].header, field, "%d", &start, 1);
  start --;
  snprintf (field, 256, "TFORM%d\0", X);
  gfits_scan (&table[0].header, field, "%s", Tform, 1);
  Nchar = atof (&Tform[1]);
  
  if (!strcmp (mode, "%d")) {
    ALLOCATE (Ivalues, int, table[0].Naxis[1]);
    for (i = 0; i < table[0].Naxis[1]; i++) {
      strncpy_nowarn (tmp, &table[0].buffer[i*table[0].Naxis[0] + start], Nchar);
      Ivalues[i] = atof (tmp);
    }
    values[0] = (char *) Ivalues;
  }
  if (!strcmp (mode, "%lf")) {
    ALLOCATE (Dvalues, double, table[0].Naxis[1]);
    for (i = 0; i < table[0].Naxis[1]; i++) {
      strncpy_nowarn (tmp, &table[0].buffer[i*table[0].Naxis[0] + start], Nchar);
      Dvalues[i] = atof (tmp);
    }
    values[0] = (char *) Dvalues;
  }
  if ((mode[0] == '%') && (mode[strlen(mode) - 1] == 'c')) {
    mode[strlen(mode) - 1] = 0;
    N = atof(&mode[1]);
    ALLOCATE (Cvalues, char *, table[0].Naxis[1]);
    for (i = 0; i < table[0].Naxis[1]; i++) {
      ALLOCATE (Cvalues[i], char, N + 1);
      strncpy_nowarn (Cvalues[i], &table[0].buffer[i*table[0].Naxis[0] + start], N);
    }
    values[0] = (char *) Cvalues;
  }

}

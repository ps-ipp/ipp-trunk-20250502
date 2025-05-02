# include <ohana.h>
# include <gfitsio.h>

/*********************** fits get table value ********************************/
  
void gfits_get_table_value (table, X, Y, mode, value)
Table *table; 
int X, Y; 
char *mode, *value;
{
  
  char Tform[256], field[256];
  int  start, Nchar, byte;
  char tmp[1000];
  
  /* no error checking, all sorts of problems! */
  
  snprintf (field, 256, "TBCOL%d\0", X);
  gfits_scan (&table[0].header, field, "%d", &start, 1);
  start --;
  
  snprintf (field, 256, "TFORM%d\0", X);
  gfits_scan (&table[0].header, field, "%s", Tform, 1);
  Nchar = atof (&Tform[1]);
  myAssert (Nchar < 999, "overflow");
  
  byte = Y*table[0].Naxis[0] + start;
  strncpy_nowarn (tmp, &table[0].buffer[byte], Nchar);

  if (!strcmp (mode, "%d"))  *(int      *) value = (int)      atof (tmp);
  if (!strcmp (mode, "%u"))  *(unsigned *) value = (unsigned) atof (tmp);
  if (!strcmp (mode, "%ld")) *(long     *) value = (long)     atof (tmp);
  if (!strcmp (mode, "%hd")) *(short    *) value = (short)    atof (tmp);
  if (!strcmp (mode, "%f"))  *(float    *) value = (float)    atof (tmp);
  if (!strcmp (mode, "%lf")) *(double   *) value = (double)   atof (tmp);

  if (!strcmp (mode, "%s")) 
    strcpy (value, tmp);

}


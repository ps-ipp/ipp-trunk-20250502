# include <ohana.h>
# include <gfitsio.h>

# if (0)
/*** special version of this function to define unsigned int columns (special values for BZERO, BSCALE) ********************/
int gfits_define_bintable_column_unsigned_int (Header *header, char *format, char *label, char *comment, char *unit, int inttype) {

  assert (label);
  assert (format);

  off_t Naxis1;
  int Nfields, Nbytes, Nval;
  char type[64], field[64];
  
  if (!gfits_bintable_format (format, type, &Nval, &Nbytes)) return (FALSE);
  
  Nfields = 0;
  gfits_scan (header, "TFIELDS", "%d", 1, &Nfields);
  gfits_scan (header, "NAXIS1",  OFF_T_FMT, 1,  &Naxis1);
  Nfields ++;
  Naxis1 += Nbytes*Nval;

  snprintf (field, 64, "TTYPE%d", Nfields);
  gfits_modify (header, field, "%s", 1, label);
  gfits_modify_alt (header, field, "%C", 1, comment);
  snprintf (field, 64, "TUNIT%d", Nfields);
  gfits_modify (header, field, "%s", 1, unit);
  snprintf (field, 64, "TFORM%d", Nfields);
  gfits_modify (header, field, "%s", 1, format);

  // add scaling parameters unless they amount to a noop
  if ((bscale != 1.0) || (bzero != 0.0)) {
      snprintf (field, 64, "TSCAL%d", Nfields);
      gfits_modify (header, field, "%lf", 1, bscale);
      snprintf (field, 64, "TZERO%d", Nfields);
      gfits_modify (header, field, "%lf", 1, bzero);
  }

  /* update TFIELDS & NAXIS1 */
  gfits_modify (header, "TFIELDS", "%d", 1, Nfields);
  gfits_modify (header, "NAXIS1",  OFF_T_FMT, 1,  Naxis1);
  header[0].Naxis[0] = Naxis1;

  return (TRUE);
}
# endif

/***********************/
int gfits_define_bintable_column (Header *header, char *format, char *label, char *comment, char *unit, double bscale, double bzero) {

  assert (label);
  assert (format);

  off_t Naxis1;
  int Nfields, Nbytes, Nval;
  char type[64], field[64];
  
  // this call supported multiple columns per named field
  if (!gfits_bintable_format (format, type, &Nval, &Nbytes)) return (FALSE);
  
  Nfields = 0;
  gfits_scan (header, "TFIELDS", "%d", 1, &Nfields);
  gfits_scan (header, "NAXIS1",  OFF_T_FMT, 1,  &Naxis1);
  Nfields ++;
  Naxis1 += Nbytes*Nval;

  snprintf (field, 64, "TTYPE%d", Nfields);
  gfits_modify (header, field, "%s", 1, label);
  if (comment) {
    gfits_modify_alt (header, field, "%C", 1, comment);
  }
  if (unit) {
    snprintf (field, 64, "TUNIT%d", Nfields);
    gfits_modify (header, field, "%s", 1, unit);
  }
  snprintf (field, 64, "TFORM%d", Nfields);
  gfits_modify (header, field, "%s", 1, format);

  // add scaling parameters unless they amount to a noop
  if ((bscale != 1.0) || (bzero != 0.0)) {
      snprintf (field, 64, "TSCAL%d", Nfields);
      gfits_modify (header, field, "%lf", 1, bscale);
      snprintf (field, 64, "TZERO%d", Nfields);
      gfits_modify (header, field, "%lf", 1, bzero);
  }

  /* update TFIELDS & NAXIS1 */
  gfits_modify (header, "TFIELDS", "%d", 1, Nfields);
  gfits_modify (header, "NAXIS1",  OFF_T_FMT, 1,  Naxis1);
  header[0].Naxis[0] = Naxis1;

  return (TRUE);
}

/***********************/
int gfits_define_table_column (Header *header, char *format, char *label, char *comment, char *unit) {

  off_t Naxis1;
  int Nstart, Nfields, Nbytes, Nval;
  char type[64], field[64], cformat[64];

  strcpy (cformat, format);
  if (!gfits_table_format (cformat, type, &Nval, &Nbytes)) return (FALSE);

  Nfields = 0;
  gfits_scan (header, "TFIELDS", "%d", 1, &Nfields);
  gfits_scan (header, "NAXIS1",  OFF_T_FMT, 1,  &Naxis1);
  Nstart = Naxis1 + 1;
  Nfields ++;
  Naxis1 += Nbytes*Nval;

  snprintf (field, 64, "TTYPE%d", Nfields);
  gfits_modify (header, field, "%s", 1, label);
  gfits_modify_alt (header, field, "%C", 1, comment);
  snprintf (field, 64, "TUNIT%d", Nfields);
  gfits_modify (header, field, "%s", 1, unit);
  snprintf (field, 64, "TFORM%d", Nfields);
  gfits_modify (header, field, "%s", 1, format);

  snprintf (field, 64, "TBCOL%d", Nfields);
  gfits_modify (header, field, "%d", 1, Nstart);

  gfits_modify (header, "TFIELDS", "%d", 1, Nfields);
  gfits_modify (header, "NAXIS1",  OFF_T_FMT, 1,  Naxis1);
  header[0].Naxis[0] = Naxis1;

  return (TRUE);
}

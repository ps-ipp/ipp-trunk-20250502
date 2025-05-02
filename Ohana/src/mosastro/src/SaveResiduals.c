# include "mosastro.h"

void SaveResiduals (FILE *f, Header *header) {

  int i, j, N, Nmatch;
  MatchData *match;
  Matrix matrix;
  Header theader;
  FTable table;

  header[0].extend = TRUE;
  header[0].Naxes = 0;
  gfits_modify (header, "NAXIS",   "%d", 1, 0);
  gfits_modify_alt (header, "EXTEND",  "%t", 1, TRUE);
  gfits_modify (header, "NEXTEND", "%d", 1, 1);

  /* add in some keywords to specify the datatype & software version? */

  /* create (empty) data matrix */
  gfits_create_matrix (header, &matrix);
    
  /* create bintable header */
  gfits_create_table_header (&theader, "BINTABLE", "MOSASTRO_RESIDUALS");

  /* define bintable layout */
  gfits_define_bintable_column (&theader, "D",    "R_RAW",      "ra (raw)",             "degrees",                        1.0, 0.0); 
  gfits_define_bintable_column (&theader, "D",    "D_RAW",      "dec (raw)",            "degrees",                        1.0, 0.0); 
  gfits_define_bintable_column (&theader, "E",    "P_RAW",      "P coord (raw)",        "pixels",                         1.0, 0.0); 
  gfits_define_bintable_column (&theader, "E",    "Q_RAW",      "Q coord (raw)",        "pixels",                         1.0, 0.0); 
  gfits_define_bintable_column (&theader, "E",    "L_RAW",      "L coord (raw)",        "pixels",                         1.0, 0.0); 
  gfits_define_bintable_column (&theader, "E",    "M_RAW",      "M coord (raw)",        "pixels",                         1.0, 0.0); 
  gfits_define_bintable_column (&theader, "E",    "X_RAW",      "X coord (raw)",        "pixels",                         1.0, 0.0); 
  gfits_define_bintable_column (&theader, "E",    "Y_RAW",      "Y coord (raw)",        "pixels",                         1.0, 0.0); 

  gfits_define_bintable_column (&theader, "D",    "R_REF",      "ra (ref)",             "degrees",                        1.0, 0.0); 
  gfits_define_bintable_column (&theader, "D",    "D_REF",      "dec (ref)",            "degrees",                        1.0, 0.0); 
  gfits_define_bintable_column (&theader, "E",    "P_REF",      "P coord (ref)",        "pixels",                         1.0, 0.0); 
  gfits_define_bintable_column (&theader, "E",    "Q_REF",      "Q coord (ref)",        "pixels",                         1.0, 0.0); 
  gfits_define_bintable_column (&theader, "E",    "L_REF",      "L coord (ref)",        "pixels",                         1.0, 0.0); 
  gfits_define_bintable_column (&theader, "E",    "M_REF",      "M coord (ref)",        "pixels",                         1.0, 0.0); 
  gfits_define_bintable_column (&theader, "E",    "X_REF",      "X coord (ref)",        "pixels",                         1.0, 0.0); 
  gfits_define_bintable_column (&theader, "E",    "Y_REF",      "Y coord (ref)",        "pixels",                         1.0, 0.0); 

  gfits_define_bintable_column (&theader, "E",    "MAG_REF",    "catalog mag",          "pixels",                         1.0, 0.0); 
  gfits_define_bintable_column (&theader, "E",    "DMAG_REF",   "catalog mag err",      "pixels",                         1.0, 0.0); 
  gfits_define_bintable_column (&theader, "E",    "MAG_RAW",    "instrum mag",          "pixels",                         1.0, 0.0); 
  gfits_define_bintable_column (&theader, "E",    "DMAG_RAW",   "instrum mag err",      "pixels",                         1.0, 0.0); 
  gfits_define_bintable_column (&theader, "B",    "MASK",       "excluded from fit?",   "",                               1.0, 0.0);
  gfits_define_bintable_column (&theader, "7A",   "DUMMY",      "padding",              "",                               1.0, 0.0);

  /* create table, add data values */
  gfits_create_table (&theader, &table);

  /* create output data block and assign */
  Nmatch = 0;
  for (i = 0; i < Nchip; i++) Nmatch += chip[i].Nmatch; 
  ALLOCATE (match, MatchData, Nmatch);

  for (i = N = 0; i < Nchip; i++) {
    for (j = 0; j < chip[i].Nmatch; j++, N++) {
      match[N].Rraw = chip[i].raw[j].R;
      match[N].Draw = chip[i].raw[j].D;
      match[N].Praw = chip[i].raw[j].P;
      match[N].Qraw = chip[i].raw[j].Q;
      match[N].Lraw = chip[i].raw[j].L;
      match[N].Mraw = chip[i].raw[j].M;
      match[N].Xraw = chip[i].raw[j].X;
      match[N].Yraw = chip[i].raw[j].Y;

      match[N].Rref = chip[i].ref[j].R;
      match[N].Dref = chip[i].ref[j].D;
      match[N].Pref = chip[i].ref[j].P;
      match[N].Qref = chip[i].ref[j].Q;
      match[N].Lref = chip[i].ref[j].L;
      match[N].Mref = chip[i].ref[j].M;
      match[N].Xref = chip[i].ref[j].X;
      match[N].Yref = chip[i].ref[j].Y;

      match[N].Mcat   = chip[i].ref[j].Mag;
      match[N].dMcat  = chip[i].ref[j].dMag;
      match[N].Minst  = chip[i].raw[j].Mag;
      match[N].dMinst = chip[i].raw[j].dMag;
      match[N].mask   = chip[i].raw[j].mask;
    }
  }

  /* fix byte order issues */
  ConvertMatch (match, sizeof (MatchData), Nmatch);
  gfits_add_rows (&table, (char *) match, Nmatch, sizeof (MatchData));

  gfits_fwrite_header  (f, header);
  gfits_fwrite_matrix  (f, &matrix);
  gfits_fwrite_Theader (f, &theader);
  gfits_fwrite_table   (f, &table);
  fclose (f);
  return;
}

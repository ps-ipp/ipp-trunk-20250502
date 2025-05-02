# include "mosastro.h"
# include <glob.h>

int main (int argc, char **argv) {

  int Nrefcat, Nastro;
  double Cerror, DL, DM;
  double Scale;
  glob_t pglob;
  StarData *refcat;
  Gradients *grad;


  ConfigInit (&argc, argv);
  args (&argc, argv); 

  if (argc != 4) { 
    fprintf (stderr, "USAGE: mosastro (inglob) (ext) (mosaic.phu)\n");
    exit (2);
  }

  pglob.gl_offs = 0;
  glob (argv[1], 0, NULL, &pglob);

  LoadStars (pglob.gl_pathc, pglob.gl_pathv);

  /* use per-chip astrometry to find ra,dec range */ 
  deproject_stars ();
  field_stats (); /** needs coords from deproject_stars **/
  init_field ();  /** needs results from field stats **/
  init_chips ();  /** needs results from init_field **/
  Scale = 3600.0 * field.project.cdelt1;

  /* use field model to get TP & FP coords */
  project_stars ();
  if ((DUMP != NULL) && !strcmp (DUMP, "rawstars")) dump_rawstars();

  refcat = greference (&Nrefcat);
  project_refcat (refcat, Nrefcat);
  if ((DUMP != NULL) && !strcmp (DUMP, "refcat")) dump_refcat(refcat, Nrefcat);

  match (refcat, Nrefcat);
  deproject_raw ();
  project_ref ();
  Cerror   = GetScatter (&Nastro, &DL, &DM, TRUE);
  fprintf (stderr, "scatter (raw) : %6.4f for %d stars (%6.4f, %6.4f)\n", Cerror, Nastro, DL*Scale, DM*Scale);
  if ((DUMP != NULL) && !strcmp (DUMP, "rawmatch")) dump_match();

  grad = GetGradients ();
  FitGradients (grad);
  Cerror   = GetScatter (&Nastro, &DL, &DM, TRUE);
  fprintf (stderr, "scatter (grad) : %6.4f for %d stars (%6.4f, %6.4f)\n", Cerror, Nastro, DL*Scale, DM*Scale);
  if ((DUMP != NULL) && !strcmp (DUMP, "fitgrads")) dump_match();
  Scale = 3600.0 * field.project.cdelt1;

  FitChips (ChipOrder);
  Cerror   = GetScatter (&Nastro, &DL, &DM, TRUE);
  fprintf (stderr, "scatter (chip) : %6.4f for %d stars (%6.4f, %6.4f)\n", Cerror, Nastro, DL*Scale, DM*Scale);
  if ((DUMP != NULL) && !strcmp (DUMP, "fitchips_unclip")) dump_match();

  ClipOnFP (2.5);
  Cerror   = GetScatter (&Nastro, &DL, &DM, TRUE);
  fprintf (stderr, "scatter (clip) : %6.4f for %d stars (%6.4f, %6.4f)\n", Cerror, Nastro, DL*Scale, DM*Scale);

  FitChips (ChipOrder);
  Cerror   = GetScatter (&Nastro, &DL, &DM, TRUE);
  fprintf (stderr, "scatter (chip) : %6.4f for %d stars (%6.4f, %6.4f)\n", Cerror, Nastro, DL*Scale, DM*Scale);

  ClipOnFP (2.5);
  Cerror   = GetScatter (&Nastro, &DL, &DM, TRUE);
  fprintf (stderr, "scatter (clip) : %6.4f for %d stars (%6.4f, %6.4f)\n", Cerror, Nastro, DL*Scale, DM*Scale);

  FitChips (ChipOrder);
  Cerror   = GetScatter (&Nastro, &DL, &DM, TRUE);
  fprintf (stderr, "scatter (chip) : %6.4f for %d stars (%6.4f, %6.4f)\n", Cerror, Nastro, DL*Scale, DM*Scale);

  ClipOnFP (2.5);
  Cerror   = GetScatter (&Nastro, &DL, &DM, TRUE);
  fprintf (stderr, "scatter (clip) : %6.4f for %d stars (%6.4f, %6.4f)\n", Cerror, Nastro, DL*Scale, DM*Scale);

  FitChips (ChipOrder);
  Cerror   = GetScatter (&Nastro, &DL, &DM, FALSE);
  fprintf (stderr, "scatter (brit) : %6.4f for %d stars (%6.4f, %6.4f)\n", Cerror, Nastro, DL*Scale, DM*Scale);
  Cerror   = GetScatter (&Nastro, &DL, &DM, TRUE);
  fprintf (stderr, "scatter (chip) : %6.4f for %d stars (%6.4f, %6.4f)\n", Cerror, Nastro, DL*Scale, DM*Scale);
  if ((DUMP != NULL) && !strcmp (DUMP, "fitchips")) dump_match();

  /* testcoords (); */
  output (argv[2], argv[3]);
  exit (0);

/*
  FitField (3);
  FitChips (3);
*/

}

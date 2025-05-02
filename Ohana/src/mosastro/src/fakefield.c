# include "mosastro.h"

/* determine an initial guess to field parameters from data */ 
int fake_field_center (double RA, double DEC) {

  field.Rmin = RA  - 0.5;
  field.Rmax = RA  + 0.5;
  field.Dmin = DEC - 0.5;
  field.Dmax = DEC + 0.5;

  /* bore site center guess */
  field.project.crval1 = 0.5*(field.Rmin + field.Rmax);
  field.project.crval2 = 0.5*(field.Dmin + field.Dmax);
  
  return (1);
}

/* set default field parameters (overridden in args_obs) */ 
int fake_field_defaults () {

  int i;

  /* bore site center guess */
  InitCoords (&field.project, "DEC--TAN");
  
  /* set TP plate scale */
  field.project.cdelt1 = 1.0/3600.0;
  field.project.cdelt2 = 1.0/3600.0;

  /** distort only has power in polyterms **/
  InitCoords (&field.distort, "DEC--PLY");

  return (1);
}


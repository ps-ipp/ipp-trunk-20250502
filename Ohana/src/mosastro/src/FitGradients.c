# include "mosastro.h"

/* fit dPdL, etc vs L,M */
void FitGradients (Gradients *grad) {

  int i, Norder;
  Gradients grfix;

  ALLOCATE (grfix.dPdL, double, grad[0].Npts);
  ALLOCATE (grfix.dPdM, double, grad[0].Npts);
  ALLOCATE (grfix.dQdL, double, grad[0].Npts);
  ALLOCATE (grfix.dQdM, double, grad[0].Npts);

  ALLOCATE (grfix.Lo,   double, grad[0].Npts);
  ALLOCATE (grfix.Mo,   double, grad[0].Npts);
  grfix.Npts = grad[0].Npts;

  /* where do we set field.distort.Npolyterms? */
  Norder = 3;

  fit_init (Norder - 1);
  for (i = 0; i < grad[0].Npts; i++) {
    fit_add (grad[0].Lo[i], grad[0].Mo[i], grad[0].dPdL[i], grad[0].dPdM[i]);
  }
  fit_eval ();
  fit_apply_grads (&field.distort, &field.project, 0);
  fit_correct_grads (grad, &grfix, 0);
  fit_free ();

  fit_init (Norder - 1);
  for (i = 0; i < grad[0].Npts; i++) {
    fit_add (grad[0].Lo[i], grad[0].Mo[i], grad[0].dQdL[i], grad[0].dQdM[i]);
  }
  fit_eval ();
  fit_apply_grads (&field.distort, &field.project, 1);
  fit_correct_grads (grad, &grfix, 1);
  fit_free ();

  /* use new model of field to get TP & FP coords */
  deproject_raw ();
  project_ref ();

  if ((DUMP != NULL) && !strcmp (DUMP, "grads")) {
    dump_grads (&grfix, "gradfix.dat");
    exit (0);
  }
}

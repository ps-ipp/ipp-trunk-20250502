# include "gastro2.h"

void gfit (CmpCatalog *Target, RefCatalog *Ref, int order) {

  int i, j, j0;
  int Npair, *idx1, *idx2;
  double Radius, Radius2;
  double dX, dY, dR;
  RefCatalog Subset;
  StarData *st, *sr;

  /* XXX why is this hardwired here? */
  NFIELD = 0.1;
  gproject (Target, Ref, &Subset);
  if (PLOTSTUFF) plot_fullfield (Target, &Subset);

  if (Subset.N < 3) {
    fprintf (stderr, "ERROR: solution off target\n");
    exit (1);
  }

  /* need the stars sorted in X */
  sort_stars_X (Target[0].stars, Target[0].N);
  sort_stars_X (Subset.stars, Subset.N);

  Radius = MAX (2.5 * Target[0].answer.dR, 0.5);
  Radius2 = Radius*Radius;

  st = Target[0].stars;
  sr = Subset.stars;

  /* find the matched pairs of stars within the radius */
  pair_init ();
  for (i = j = 0; (i < Target[0].N) && (j < Subset.N);) {
    /* get in right X range */
    dX = st[i].X - sr[j].X;
    if (dX < -Radius) {
      i++;
      continue;
    }
    if (dX > Radius) {
      j++;
      continue;
    }

    /* check for pairs in this X range */
    j0 = j;
    for (; (dX > -Radius) && (j < Subset.N); j++) {
    
      dX = st[i].X - sr[j].X;
      dY = st[i].Y - sr[j].Y;

      dR = dX*dX + dY*dY;
      if (dR > Radius2) {
	j++;
	continue;
      }
      pair_add (i, j);
    }
    j = j0;
    i ++;
  }
  
  Npair = pair_lists (&idx1, &idx2);
  /* find fit for matched pairs */
  fit_init (order);
  for (i = 0; i < Npair; i++) {
    fit_add (st[idx1[i]].X, st[idx1[i]].Y, sr[idx2[i]].P, sr[idx2[i]].Q, 1.0);
  }
  fit_eval ();

  /* XXX this is weak: the fit_scat call requires the coords from the fit_adjust call */
  Target[0].answer.N  = fit_adjust (&Target[0].coords);
  Target[0].answer.dR = fit_scat (st, sr, &Target[0].coords);

  if (PLOTSTUFF) fprintf (stderr, "ploting resid (2)\n");
  plot_resid_init (0, (double) Target[0].header.Naxis[0]);
  // XXX test: plotting fit_apply and RD_to_XY results plot_resid_init (1, (double) Target[0].header.Naxis[1]);
  plot_resid_init (1, (double) Target[0].header.Naxis[0]);
  if (PLOTSTUFF) plot_resid (st, sr, &Target[0].coords);
  free (idx1);
  free (idx2);
}

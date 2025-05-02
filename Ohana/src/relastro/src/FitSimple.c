# include "relastro.h"

void FitSimple (StarData *raw, StarData *ref, int Nmatch, Coords *coords) {

  int i;
  CoordFit *fit;
  double dP, dQ, dR;

  fit = fit_init (coords[0].Npolyterms);
  for (i = 0; i < Nmatch; i++) {
    if (raw[i].mask) continue;

    // require radius of XXX arcsec
    dP = raw[i].P - ref[i].P;
    dQ = raw[i].Q - ref[i].Q;
    dR = 3600.0 * hypot (dP, dQ);

    // XXX the value needs to be set in a more intelligent way
    if (dR > 0.50) continue;
    
    fit_add (fit, raw[i].X, raw[i].Y, ref[i].P, ref[i].Q, 1.0);
  }
  if (fit[0].Npts == 0) {
    fit_free (fit);
    return;
  }
  fit_eval (fit);
  fit_apply_coords (fit, coords, TRUE);
  fit_free (fit);

  // apply new coords to raw (X,Y -> P,Q)
  for (i = 0; i < Nmatch; i++) {
    XY_to_LM (&raw[i].L, &raw[i].M, raw[i].X, raw[i].Y, coords);
    raw[i].P = raw[i].L;
    raw[i].Q = raw[i].M;
  }
}

/* in the simple case, we only have three coord systems of interest:
   R,D : the sky
   P,Q : the tangent plane
   X,Y : the chip

   R,D -> P,Q (projection)
   P,Q -> X,Y (polynomial transformation)

   L,M is maintained, but is identical to P,Q
*/

/* XXX I'm not using the errors at all : this could at least be done with the dMag values */

/* XXX See notes in FitChips.c */

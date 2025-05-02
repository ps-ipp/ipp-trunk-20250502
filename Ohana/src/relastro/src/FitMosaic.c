# include "relastro.h"

void FitMosaic (StarData *raw, StarData *ref, int Nmatch, Coords *coords) {

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
    if (dR > 0.15) continue;
    
    fit_add (fit, raw[i].L, raw[i].M, ref[i].P, ref[i].Q, 1.0);
  }
  if (fit[0].Npts == 0) {
    fit_free (fit);
    return;
  }
  fit_eval (fit);
  fit_apply_coords (fit, coords, TRUE);
  fit_free (fit);

  // apply new coords to raw (X,Y -> L,M)
  for (i = 0; i < Nmatch; i++) {
    XY_to_LM (&raw[i].P, &raw[i].Q, raw[i].L, raw[i].M, coords);
  }
}

/* in the mosaic case, we have four coord systems of interest:
   R,D : the sky
   P,Q : the tangent plane
   L,M : the focal plane
   X,Y : the chip

   R,D -> P,Q (projection)
   P,Q -> L,M (polynomial transformation : DIS)
   L,M -> X,Y (polynomial transformation : WRP)
*/

/* XXX I'm not using the errors at all : this could at least be done with the dMag values */

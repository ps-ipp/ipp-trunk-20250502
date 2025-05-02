# include "mosastro.h"
/* match is done on tangent plane : should be focal plane in some cases? */

int match (StarData *refcat, int Nrefcat) {

  int i, j, k, K, Ntotal;
  int Nmatch, NMATCH;
  double dp, dq, radius, Radius;
  double *p, *q, *P, *Q;
  off_t *U, *u;

  /* requested radius is in arcsec ; internally radius is in pixels */
  Radius = RADIUS / (3600.0 * field.project.cdelt2);
  fprintf (stderr, "mosastro match radius: %f\n", Radius);

  /* sort the REFCAT data by P in tangent plane */ 
  ALLOCATE (P, double, Nrefcat);
  ALLOCATE (Q, double, Nrefcat);
  ALLOCATE (U, off_t, Nrefcat);
  for (i = 0; i < Nrefcat; i++) {
    U[i] = i;
    P[i] = refcat[i].P;
    Q[i] = refcat[i].Q;
  }
  sort_coords_index (P, Q, U, Nrefcat);

  for (i = 0; i < Nchip; i++) {

    Nmatch = 0;
    NMATCH = 1000;
    ALLOCATE (chip[i].raw, StarData, NMATCH);
    ALLOCATE (chip[i].ref, StarData, NMATCH);

    /* sort the star data by P in tangent plane */ 
    ALLOCATE (p, double, chip[i].Nstars);
    ALLOCATE (q, double, chip[i].Nstars);
    ALLOCATE (u, off_t,  chip[i].Nstars);
    for (j = 0; j < chip[i].Nstars; j++) {
      u[j] = j;
      p[j] = chip[i].stars[j].P;
      q[j] = chip[i].stars[j].Q;
    }
    sort_coords_index (p, q, u, chip[i].Nstars);

    /* find star matches in the tangent plane coord system */
    for (j = k = 0; (j < chip[i].Nstars) && (k < Nrefcat); ) {

      /** instrumental magnitude limits for raw data **/
      if (IMAG_MIN && (chip[i].stars[u[j]].Mag - ZERO_POINT < IMAG_MIN)) {
	j++;
	continue;
      }
      if (IMAG_MAX && (chip[i].stars[u[j]].Mag - ZERO_POINT > IMAG_MAX)) {
	j++;
	continue;
      }
      /* skip anything with dMag too large */
      if (SIGMA_LIM > 0.0) {
	if (chip[i].stars[u[j]].dMag > SIGMA_LIM) {
	  j++;
	  continue;
	}
      }
	
      dp = p[j] - P[k];
	
      if (dp <= -2*Radius) {
	j++;
	continue;
      }
      if (dp >= 2*Radius) {
	k++;
	continue;
      }

      K = k;
      for (; (dp > -2*Radius) && (k < Nrefcat); k++) {
	dp = p[j] - P[k];
	dq = q[j] - Q[k];
	radius = hypot (dp, dq);
	if (radius < Radius) {
	  chip[i].ref[Nmatch] = refcat[U[k]];
	  chip[i].raw[Nmatch] = chip[i].stars[u[j]];
	  chip[i].raw[Nmatch].Mag -= ZERO_POINT;  /* raw in instrumental mags */
	  Nmatch ++;
	  if (Nmatch == NMATCH) {
	    NMATCH += 1000;
	    REALLOCATE (chip[i].raw, StarData, NMATCH);
	    REALLOCATE (chip[i].ref, StarData, NMATCH);
	  }
	  goto done;
	}
      }
    done:
      k = K;
      j++;
    }
    REALLOCATE (chip[i].raw, StarData, MAX (1, Nmatch));
    REALLOCATE (chip[i].ref, StarData, MAX (1, Nmatch));
    chip[i].Nmatch = Nmatch;
    free (p);
    free (q);
    free (u);
  }
  free (P);
  free (Q);
  free (U);
  
  Ntotal = 0;
  for (i = 0; i < Nchip; i++) {
    Ntotal += chip[i].Nmatch;
    if (VERBOSE) fprintf (stderr, "chip %d: %d of %d stars\n", 
			  i, chip[i].Nmatch, chip[i].Nstars);
  }
  fprintf (stderr, "Nchips: %d  Nmatch:  %d\n", Nchip, Ntotal);
  return (1);
}

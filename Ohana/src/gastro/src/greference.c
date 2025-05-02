# include "gastro.h"

# define USNO 1
# define GSC  0

int greference (SStars **cat, int *Ncat, Coords *coords, int NX, int NY) {

  int  i;
  CatStats catstats;

  if (VERBOSE) fprintf (stderr, "\nloading astrometric reference data from %s\n", REFCAT); 

  *Ncat = 0;

  define_region (&catstats, coords, NX, NY);
  ALLOCATE (*cat, SStars, 1);

  /* get stars from the USNO catalog for the given region */
  if (!strcmp (REFCAT, "USNO") || !strcmp (REFCAT, "BOTH")) {
    catstats.RA[0] = ohana_normalize_angle (catstats.RA[0]);
    catstats.RA[1] = ohana_normalize_angle (catstats.RA[1]);

    /* if RA crosses 0,360 boundary, do 2 passes */
    if (catstats.RA[0] > catstats.RA[1]) {
      int Nusno1, Nusno2;
      USNOdata *usno1, *usno2;
      USNOstats usnostats;
      CatStats substats;

      substats = catstats;
      substats.RA[0] = 0.0;
      usno1 = getusno (&usnostats, &substats, &Nusno1);

      substats = catstats;
      substats.RA[1] = 360.0;
      usno2 = getusno (&usnostats, &substats, &Nusno2);

      REALLOCATE (*cat, SStars, MAX (Nusno1 + Nusno2, 1));
      for (i = 0; i < Nusno1; i++) {
	cat[0][i].X = usno1[i].R;
	cat[0][i].Y = usno1[i].D;
	cat[0][i].mag = fabs(usno1[i].r);
      }      
      for (i = Nusno1; i < Nusno2; i++) {
	cat[0][i].X = usno2[i].R;
	cat[0][i].Y = usno2[i].D;
	cat[0][i].mag = fabs(usno2[i].r);
      }      
      *Ncat = Nusno1 + Nusno2;
      free (usno1);
      free (usno2);
    } else {
      int Nusno;
      USNOdata *usno;
      USNOstats usnostats;

      usno = getusno (&usnostats, &catstats, &Nusno);

      REALLOCATE (*cat, SStars, MAX (Nusno, 1));
      for (i = 0; i < Nusno; i++) {
	cat[0][i].X = usno[i].R;
	cat[0][i].Y = usno[i].D;
	cat[0][i].mag = fabs(usno[i].r);
      }      
      *Ncat = Nusno;
      free (usno);
    }
    if (VERBOSE) fprintf (stderr, "%d stars from USNO 1.0\n", *Ncat);
  }

  if (!strcmp (REFCAT, "GSC") || !strcmp (REFCAT, "BOTH")) {
    int j, Ngsc;
    SStars *gsc;

    gsc = getgsc (&catstats, &Ngsc);
    REALLOCATE (*cat, SStars, MAX (Ngsc + *Ncat, 1));
    for (j = *Ncat, i = 0; i < Ngsc; i++) {
      cat[0][j] = gsc[i];
    }
    if (VERBOSE) fprintf (stderr, "%d stars from HST GSC\n", Ngsc);
    *Ncat += Ngsc;
  }

  if (!strcmp (REFCAT, "PTOLEMY")) {
    free (*cat);
    *cat = getptolemy (&catstats, Ncat);
    if (VERBOSE) fprintf (stderr, "%d stars from PTOLEMY\n", *Ncat);
  }
  return (TRUE);
}

void define_region (CatStats *catstats, Coords *coords, int NX, int NY) {
   
  int i, j;
  double X, Y, R, D;

  catstats[0].RA[0] = catstats[0].DEC[0] =  360.0;
  catstats[0].RA[1] = catstats[0].DEC[1] = -360.0;

  for (i = -1; i < 2; i++) {
    for (j = -1; j < 2; j++) {
      X = 0.5*(1.0 + i*NFIELD)*NX;
      Y = 0.5*(1.0 + j*NFIELD)*NY;
      XY_to_RD (&R, &D, X, Y, coords);
      /* coords returns a region all in same phase 
	 while (R < 0.0)    R += 360.0;
	 while (R >= 360.0) R -= 360.0;
      */
      catstats[0].RA[0]  = MIN (catstats[0].RA[0], R);
      catstats[0].RA[1]  = MAX (catstats[0].RA[1], R);
      catstats[0].DEC[0] = MIN (catstats[0].DEC[0], D);
      catstats[0].DEC[1] = MAX (catstats[0].DEC[1], D);
    }
  }
  fprintf (stderr, "full region: %f - %f, %f - %f\n", 
	   catstats[0].RA[0], catstats[0].RA[1], catstats[0].DEC[0], catstats[0].DEC[1]);
}

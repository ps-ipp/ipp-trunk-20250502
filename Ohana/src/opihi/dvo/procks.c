# include "dvoshell.h"

typedef struct {
  double ra[3];
  double dec[3];
  double X[3];
  double Y[3];
  unsigned int t[3];
  double mag[3];
} Rocks;
static Rocks *rocks = (Rocks *) NULL;
static int   Nrocks;

int procks (int argc, char **argv) {
  
  FILE *f;
  Vector Xvec, Yvec;
  int kapa, i, j, N, NROCKS;
  int N0, N1, SpeedClip, Reload;
  unsigned int t0, t1;
  double Mz, Mr, S0, S1;
  double Rmin, Rmax;
  Graphdata graphmode;
  char rockcat[256];

  VarConfig ("ROCK_CATALOG", "%s", rockcat);
  if (!GetGraph (&graphmode, &kapa, NULL)) return (FALSE);

  f = (FILE *) NULL;
  Mz = 17.0;
  Mr = -5.0;
  Rmin = graphmode.coords.crval1 - 182.0;
  Rmax = graphmode.coords.crval1 + 182.0;

  if ((N = get_argument (argc, argv, "-m"))) {
    remove_argument (N, &argc, argv);
    Mr  = 1000*atof(argv[N]);
    remove_argument (N, &argc, argv);
    Mz = 1000*atof(argv[N]);
    Mr = Mr - Mz;
    remove_argument (N, &argc, argv);
  }

  S0 = S1 = 0;
  SpeedClip = FALSE;
  if ((N = get_argument (argc, argv, "-speed"))) {
    SpeedClip = TRUE;
    remove_argument (N, &argc, argv);
    S0 = atof(argv[N]);
    remove_argument (N, &argc, argv);
    S1 = atof(argv[N]);
    remove_argument (N, &argc, argv);
  }

  Reload = FALSE;
  if ((N = get_argument (argc, argv, "-reload"))) {
    Reload = TRUE;
    remove_argument (N, &argc, argv);
  }

  if (argc != 1) {
    gprint (GP_ERR, "USAGE: procks [-m M M] [-speed s s] \n");
    return (FALSE);
  }
  
  f = fopen (rockcat, "r");
  if (f == (FILE *) NULL) {
    gprint (GP_ERR, "can't open rock catalog\n");
    return (TRUE);
  }

  if ((rocks == (Rocks *) NULL) || Reload) {
    if (rocks != (Rocks *) NULL) free (rocks);
    NROCKS = 100;
    ALLOCATE (rocks, Rocks, NROCKS);
    for (i = 0; fscanf (f, "%lf %lf %u%lf%lf%lf %u%lf%lf%lf %u%lf%lf%lf", 
			&rocks[i].X[0], &rocks[i].Y[0], 
			&rocks[i].t[0], &rocks[i].ra[0], &rocks[i].dec[0], &rocks[i].mag[0], 
			&rocks[i].t[1], &rocks[i].ra[1], &rocks[i].dec[1], &rocks[i].mag[1], 
			&rocks[i].t[2], &rocks[i].ra[2], &rocks[i].dec[2], &rocks[i].mag[2]
			) != EOF; i++) {
      if (i == NROCKS - 1) {
	NROCKS += 100;
	REALLOCATE (rocks, Rocks, NROCKS);
      }
    }
    Nrocks = i;
  }
      
  if (Nrocks == 0) {
    free (rocks);
    gprint (GP_ERR, "no rocks in datafile\n");
    return (TRUE);
  }

  /* data has been loaded, get ready to plot it */
  SetVector (&Xvec, OPIHI_FLT, 3*Nrocks);
  SetVector (&Yvec, OPIHI_FLT, 3*Nrocks);
  
  /* project stars to screen display coords */
  for (N = i = 0; i < Nrocks; i++) {
    if (SpeedClip && ((rocks[i].Y[0] < S0) || (rocks[i].Y[0] > S1))) continue;
    for (j = 0; j < 3; j++) {
      rocks[i].ra[j]= ohana_normalize_angle (rocks[i].ra[j]);	
      while (rocks[i].ra[j] < Rmin) rocks[i].ra[j] += 360.0;
      while (rocks[i].ra[j] > Rmax) rocks[i].ra[j] -= 360.0;
      RD_to_XY (&Xvec.elements.Flt[N], &Yvec.elements.Flt[N], rocks[i].ra[j], rocks[i].dec[j], &graphmode.coords);
      N ++;
    }
  }
  Yvec.Nelements = Xvec.Nelements = N;
  
  graphmode.style = KAPA_PLOT_POINTS; /* points */
  graphmode.etype = 0; /* no errorbars */
  PlotVectorPair (kapa, &Xvec, &Yvec, NULL, &graphmode);

  /* now plot vectors between two extrema */
  Yvec.Nelements = Xvec.Nelements = 2*Nrocks;
  REALLOCATE (Xvec.elements.Flt, opihi_flt, Xvec.Nelements);
  REALLOCATE (Yvec.elements.Flt, opihi_flt, Yvec.Nelements);
  
  /* project stars to screen display coords */
  for (N = i = 0; i < Nrocks; i++) {
    if (SpeedClip && ((rocks[i].Y[0] < S0) || (rocks[i].Y[0] > S1))) continue;
    N0 = N1 = 0;
    t0 = t1 = rocks[i].t[0];
    for (j = 1; j < 3; j++) {
      if (rocks[i].t[j] < t0) { N0 = j; t0 = rocks[i].t[j]; }
      if (rocks[i].t[j] > t1) { N1 = j; t1 = rocks[i].t[j]; }
    }
    rocks[i].ra[N0]= ohana_normalize_angle (rocks[i].ra[N0]);	
    rocks[i].ra[N1]= ohana_normalize_angle (rocks[i].ra[N1]);	
    while (rocks[i].ra[N0] < Rmin) rocks[i].ra[N0] += 360.0;
    while (rocks[i].ra[N0] > Rmax) rocks[i].ra[N0] -= 360.0;
    while (rocks[i].ra[N1] < Rmin) rocks[i].ra[N1] += 360.0;
    while (rocks[i].ra[N1] > Rmax) rocks[i].ra[N1] -= 360.0;
    RD_to_XY (&Xvec.elements.Flt[N], &Yvec.elements.Flt[N], rocks[i].ra[N0], rocks[i].dec[N0], &graphmode.coords);
    N ++;
    RD_to_XY (&Xvec.elements.Flt[N], &Yvec.elements.Flt[N], rocks[i].ra[N1], rocks[i].dec[N1], &graphmode.coords);
    N ++;
  }
  Yvec.Nelements = Xvec.Nelements = N;
  
  graphmode.style = KAPA_PLOT_POINTS; /* points */
  graphmode.ptype = KAPA_POINT_PAIR_CONNECT; /* connect pairs of points */
  graphmode.etype = 0; /* no errorbars */

  PlotVectorPair (kapa, &Xvec, &Yvec, NULL, &graphmode);

  free (Xvec.elements.Flt);
  free (Yvec.elements.Flt);
  return (TRUE);

}
  

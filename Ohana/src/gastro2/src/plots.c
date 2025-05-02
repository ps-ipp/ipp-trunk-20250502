# include "gastro2.h"

int Nv, NV;
float *xv, *yv;
Graphdata gv;

void plot_init_gridplot (void) {
  
  NV = 1000;
  Nv = 0;

  ALLOCATE (xv, float, NV);
  ALLOCATE (yv, float, NV);

  PlotReset (2);

  gv.xmin = -400;
  gv.xmax = +400;
  gv.ymin = -400;
  gv.ymax = +400;

  gv.style = 2;
  gv.ltype = 0;
  gv.etype = 0;
  gv.ebar  = 0;
  gv.size = 0.3;
  gv.lweight = 0;
  gv.color = 0;
  gv.ptype = 2;

}

int plot_addpt_gridplot (double x, double y) {

  if (x < gv.xmin) return (0);
  if (x > gv.xmax) return (0);
  if (y < gv.ymin) return (0);
  if (y > gv.ymax) return (0);

  xv[Nv] = x;
  yv[Nv] = y;
  Nv ++;
  
  if (Nv == NV) {
    NV += 1000;
    REALLOCATE (xv, float, NV);
    REALLOCATE (yv, float, NV);
  }
  return (1);
}

void plot_done_gridplot (void) {

  char c;

  /* send to Kapa */
  PrepPlotting (Nv, &gv, 2);
  PlotVector (Nv, xv, 0, 2);
  PlotVector (Nv, yv, 1, 2);

  DonePlotting (&gv, 2);
  fprintf (stderr, "type return to continue");
  if (fscanf (stdin, "%c", &c) != 1) fprintf (stderr, "\n");
  free (xv);
  free (yv);
}
  
static double Xm[2];
static int Xg[2] = {0, 2};

void plot_resid_init (int version, double xmax) {

  if (version > 1) return;
  if (version < 0) return;
  
  Xm[version] = xmax;
}


void plot_resid_plot (int version, float *xvect, float *yvect, int Nvect) {
 
  int i;
  char c;
  float xmin, xmax, ymin, ymax;
  Graphdata graphdata;
  
  xmin = xmax = xvect[0];
  ymin = ymax = yvect[0];
  for (i = 0; i < Nvect; i++) {
    xmax = MAX (xvect[i], xmax);
    xmin = MIN (xvect[i], xmin);
    ymax = MAX (yvect[i], ymax);
    ymin = MIN (yvect[i], ymin);
  }
    
  PlotReset (Xg[version]);

  graphdata.xmin = MIN (xmin,   0);
  graphdata.xmax = MAX (xmax, Xm[version]);
  graphdata.ymin = MIN (ymin, -10);
  graphdata.ymax = MAX (ymax, +10);

  graphdata.style = 2;
  graphdata.ltype = 0;
  graphdata.etype = 0;
  graphdata.ebar  = 0;
  graphdata.size = 0.5;
  graphdata.lweight = 0;
  graphdata.color = 0;
  graphdata.ptype = 2;

  /* send to Kapa */
  PrepPlotting (Nvect, &graphdata, Xg[version]);
  PlotVector (Nvect, xvect, 0, Xg[version]);
  PlotVector (Nvect, yvect, 1, Xg[version]);

  DonePlotting (&graphdata, Xg[version]);
  fprintf (stderr, "type return to continue");
  if (fscanf (stdin, "%c", &c) != 1) fprintf (stderr, "\n");
}

void plot_gridpts (double *pts, int Npts) {

  char c;
  int i;
  float *xvect, *yvect, ymax;
  Graphdata graphdata;
  
  ymax = 0;
  ALLOCATE (xvect, float, Npts);
  ALLOCATE (yvect, float, Npts);
  for (i = 0; i < Npts; i++) {
    xvect[i] = i;
    yvect[i] = pts[i];
    ymax = MAX (yvect[i], ymax);
  }
    
  PlotReset (0);

  graphdata.xmin = -10;
  graphdata.xmax = Npts + 10;
  graphdata.ymin = -1;
  graphdata.ymax = ymax + 1;

  graphdata.style = 1;
  graphdata.ltype = 0;
  graphdata.etype = 0;
  graphdata.ebar  = 0;
  graphdata.size = 0.5;
  graphdata.lweight = 0;
  graphdata.color = 0;
  graphdata.ptype = 0;

  /* send to Kapa */
  PrepPlotting (Npts, &graphdata, 0);
  PlotVector (Npts, xvect, 0, 0);
  PlotVector (Npts, yvect, 1, 0);
  free (xvect);
  free (yvect);

  DonePlotting (&graphdata, 0);
  fprintf (stderr, "type return to continue");
  if (fscanf (stdin, "%c", &c) != 1) fprintf (stderr, "\n");
}

static Graphdata gf;

void plot_fullfield (CmpCatalog *Target, RefCatalog *Ref) {

  int i, Nvect;
  float *xvect, *yvect, *zvect, M, dM, mRefMin, mRefMax;
  char c;
  
  dM = Target[0].lum.Mmax - Target[0].lum.Mmin;

  PlotReset (1);

  gf.xmin = -100;
  gf.xmax = Target[0].header.Naxis[0] + 100;
  gf.ymin = -100;
  gf.ymax = Target[0].header.Naxis[1] + 100;

  gf.style = 2;
  gf.ltype = 0;
  gf.etype = 0;
  gf.ebar  = 0;
  gf.size = -1;
  gf.lweight = 0;

  /* fill in vectors */
  Nvect = Target[0].N;
  ALLOCATE (xvect, float, Nvect);
  ALLOCATE (yvect, float, Nvect);
  ALLOCATE (zvect, float, Nvect);
  for (i = 0; i < Nvect; i++) {
    xvect[i] = Target[0].stars[i].X;
    yvect[i] = Target[0].stars[i].Y;
    M = (Target[0].lum.Mmax - Target[0].stars[i].M) / dM;
    zvect[i] = MIN (1.0, MAX (0.01, M));
  }

  /* send to Kapa */
  gf.ptype = 1;
  gf.color = 0;
  PrepPlotting (Nvect, &gf, 1);
  PlotVector (Nvect, xvect, 0, 1);
  PlotVector (Nvect, yvect, 1, 1);
  PlotVector (Nvect, zvect, 1, 1);
  free (xvect);
  free (yvect);
  free (zvect);

  /* fill in vectors */
  Nvect = Ref[0].N;
  ALLOCATE (xvect, float, Nvect);
  ALLOCATE (yvect, float, Nvect);
  ALLOCATE (zvect, float, Nvect);

  mRefMin = 30.0;
  mRefMax = -5.0;
  for (i = 0; i < Nvect; i++) {
      mRefMin = MIN (mRefMin, Ref[0].stars[i].M);
      mRefMax = MAX (mRefMax, Ref[0].stars[i].M);
  }
  dM = mRefMax - mRefMin;

  for (i = 0; i < Nvect; i++) {
    xvect[i] = Ref[0].stars[i].X;
    yvect[i] = Ref[0].stars[i].Y;
    M = (mRefMax - Ref[0].stars[i].M) / dM;
    zvect[i] = MIN (1.0, MAX (0.01, M));
  }

  /* send to Kapa */
  gf.color = 2;
  gf.ptype = 2;
  PrepPlotting (Nvect, &gf, 1);
  PlotVector (Nvect, xvect, 0, 1);
  PlotVector (Nvect, yvect, 1, 1);
  PlotVector (Nvect, zvect, 2, 1);
  free (xvect);
  free (yvect);
  free (zvect);

  DonePlotting (&gf, 1);
  fprintf (stderr, "type return to continue");
  if (fscanf (stdin, "%c", &c) != 1) fprintf (stderr, "\n");
}

void plot_fullfield_pairs (float *x, float *y, int n) {

  char c;

  /* send to Kapa */
  gf.color = 6;
  gf.ptype = 100;
  gf.size =  1.0;
  
  PrepPlotting (n, &gf, 1);
  PlotVector (n, x, 0, 1);
  PlotVector (n, y, 1, 1);
  fprintf (stderr, "type return to continue");
  if (fscanf (stdin, "%c", &c) != 1) fprintf (stderr, "\n");
}  

/* mag range -5 - 35, dmag = 0.25, Nbin = 160 */
# define MMIN -5
# define MMAX 35
# define dM 0.5
# define NMBIN 90

void plot_lumfunc (CmpCatalog *Target, RefCatalog *Ref) {

  int i, Nr, Nt;
  float tbin[NMBIN], tval[NMBIN], rbin[NMBIN], rval[NMBIN];
  double ymin, ymax;
  char c;
  Graphdata graphdata;

  fill_lumfunc (Target[0].stars, Target[0].N, tval, tbin, &Nt);
  fill_lumfunc (Ref[0].stars, Ref[0].N, rval, rbin, &Nr);

  /* construct plots of log(stars / degree square) */
  ymin = 5; ymax = -5;
  for (i = 0; i < Nt; i++) {
    tval[i] = tval[i] - log10 (Target[0].Area);
    ymin = MIN (tval[i], ymin);
    ymax = MAX (tval[i], ymax);
  }

  for (i = 0; i < Nr; i++) {
    rval[i] = rval[i] - log10 (Ref[0].Area);
    rbin[i] = rbin[i] + Ref[0].Moff;
    ymin = MIN (rval[i], ymin);
    ymax = MAX (rval[i], ymax);
  }

  graphdata.xmin = MMIN;
  graphdata.xmax = MMAX;
  graphdata.ymin = ymin - 0.1;
  graphdata.ymax = ymax + 0.1;

  graphdata.style = 1;
  graphdata.ptype = 2;
  graphdata.ltype = 0;
  graphdata.etype = 0;
  graphdata.ebar  = 0;
  graphdata.color = 0;

  graphdata.lweight = 0;
  graphdata.size = 0.5;

  PlotReset (0);

  PrepPlotting (Nt, &graphdata, 0);
  PlotVector (Nt, tbin, 0, 0);
  PlotVector (Nt, tval, 1, 0);

  graphdata.color = 2;
  graphdata.style = 1;
  graphdata.lweight = 0;
  PrepPlotting (Nr, &graphdata, 0);
  PlotVector (Nr, rbin, 0, 0);
  PlotVector (Nr, rval, 1, 0);

  /* plot truncated lum func */
  for (i = 0; i < Nr; i++) {
    if (rbin[i] < Target[0].lum.Mmin - 0.5) rval[i] = -1;
    if (rbin[i] > Target[0].lum.Mmax + 0.5) rval[i] = -1;
  }
  graphdata.ltype = 1;
  PrepPlotting (Nr, &graphdata, 0);
  PlotVector (Nr, rbin, 0, 0);
  PlotVector (Nr, rval, 1, 0);

  DonePlotting (&graphdata, 0);
  fprintf (stderr, "type return to continue");
  if (fscanf (stdin, "%c", &c) != 1) fprintf (stderr, "\n");

}


void plot_resid (StarData *st, StarData *sr, Coords *coords) {

  int i;
  double x, y, dx, dy;
  float *xvect0, *yvect0, *xvect1, *yvect1, *xvect2, *yvect2;
  int Npair, *idx1, *idx2;

  Npair = pair_lists (&idx1, &idx2);
  ALLOCATE (xvect0, float, Npair);
  ALLOCATE (yvect0, float, Npair);
  ALLOCATE (xvect1, float, Npair);
  ALLOCATE (yvect1, float, Npair);

  ALLOCATE (xvect2, float, 2*Npair);
  ALLOCATE (yvect2, float, 2*Npair);
  
  for (i = 0; i < Npair; i++) {
    RD_to_XY (&x, &y, sr[idx2[i]].R, sr[idx2[i]].D, coords);
    
    dx = st[idx1[i]].X - x;
    dy = st[idx1[i]].Y - y;
    
    xvect0[i] = st[idx1[i]].X;
    xvect1[i] = st[idx1[i]].Y;
    yvect0[i] = dx;
    yvect1[i] = dy;

    xvect2[2*i+0] = st[idx1[i]].X;
    yvect2[2*i+0] = st[idx1[i]].Y;
    xvect2[2*i+1] = x;
    yvect2[2*i+1] = y;
  }

  fprintf (stderr, "residuals using RD_to_XY\n");
  plot_resid_plot (0, xvect0, yvect0, Npair);
  // plot_resid_plot (1, xvect1, yvect1, Npair);
  plot_fullfield_pairs (xvect2, yvect2, 2*Npair);

  for (i = 0; i < Npair; i++) {
    fit_apply (&x, &y, st[idx1[i]].X, st[idx1[i]].Y);
    
    dx = (x - sr[idx2[i]].P)/coords[0].cdelt1;
    dy = (y - sr[idx2[i]].Q)/coords[0].cdelt2;
    
    xvect0[i] = st[idx1[i]].X;
    xvect1[i] = st[idx1[i]].Y;
    yvect0[i] = dx;
    yvect1[i] = dy;

    xvect2[2*i+0] = sr[idx2[i]].P;
    yvect2[2*i+0] = sr[idx2[i]].Q;
    xvect2[2*i+1] = x;
    yvect2[2*i+1] = y;
  }

  fprintf (stderr, "residuals using fit_apply\n");
  plot_resid_plot (1, xvect0, yvect0, Npair);
  // plot_resid_plot (1, xvect1, yvect1, Npair);
  // plot_fullfield_pairs (xvect2, yvect2, 2*Npair);

  free (xvect2);
  free (yvect2);
  free (xvect0);
  free (yvect0);
  free (xvect1);
  free (yvect1);

}

void fill_lumfunc (StarData *stars, int N, float *lbin, float *bin, int *nb) {

  int i, j, Nb;
  double mbin[NMBIN];

  bzero (mbin, NMBIN * sizeof (double));

  /* sum histogram */
  for (i = 0; i < N; i++) {
    if (stars[i].M < MMIN) continue;
    if (stars[i].M > MMAX) continue;

    j = (stars[i].M - MMIN) / dM;
    j = MIN (MAX (j, 0), (NMBIN - 1));
    mbin[j] ++;
  }

  /* select filled bins */
  for (Nb = i = 0; i < NMBIN; i++) {
    if (mbin[i] > 0) {
      bin[Nb]  = i * dM + MMIN;
      lbin[Nb] = log (mbin[i]) / log (10.0);
      Nb++;
    }
  }

  *nb = Nb;
}


# include "dvo.h"

/* AstromOffsetMap functions:
 */

int dump_map_data (float *x, float *y, float *f, int Npts, char *filename);
int AstromOffsetMapFit_Chisq (AstromOffsetMap *map, float *x, float *y, float *f, int Npts, int xdir);
int AstromOffsetMapFit_Mean (AstromOffsetMap *map, float *x, float *y, float *f, float *df, int Npts, int xdir);

float AstromOffsetMapValue (AstromOffsetMap *map, float x, float y, int xdir) {

  // given x,y find the offset (x-direction) at that location from the map

  int Nx = map->Nx;
  int Ny = map->Ny;

  float *V = xdir ? map->dXv : map->dYv;

  // if there is no spatial information, the value at the point is the value in the map
  if ((Nx == 1) && (Ny == 1)) return V[0];

  // if there is no spatial information in 1D, the interpolation is 1D
  if (Nx == 1) {
    float ymo = y * map->dY - 0.5;
    int   ymi = floor(ymo);
    ymi = MAX(0,MIN(Ny - 2, ymi)); // force range of ymi to be 0,Ny-2 (Ny must be > 1)

    float ymf = ymo - ymi;

    float value = ymf * V[ymi+1] + (1.0 - ymf) * V[ymi];
    return value;
  }

  // if there is no spatial information in 1D, the interpolation is 1D
  if (Ny == 1) {
    float xmo = x * map->dX - 0.5;
    int   xmi = floor(xmo);
    xmi = MAX(0,MIN(Nx - 2, xmi)); // force range of ymi to be 0,Nx-2  (Nx must be > 1)

    float xmf = xmo - xmi;

    float value = xmf * V[xmi+1] + (1.0 - xmf) * V[xmi];
    return value;
  }

  // x & y are in Big Image coordinates, convert to fractional map coordinates 

  float xmo = x * map->dX - 0.5;
  int   xmi = floor(xmo);
  xmi = MAX(0,MIN(Nx - 2, xmi)); // force range of ymi to be 0,Nx-1
  float xmf = xmo - xmi;

  float ymo = y * map->dY - 0.5;
  int   ymi = floor(ymo);
  ymi = MAX(0,MIN(Ny - 2, ymi)); // force range of ymi to be 0,Ny-2
  float ymf = ymo - ymi;

  float V00 = V[xmi+0 + (ymi+0)*Nx];
  float V01 = V[xmi+0 + (ymi+1)*Nx];
  float V10 = V[xmi+1 + (ymi+0)*Nx];
  float V11 = V[xmi+1 + (ymi+1)*Nx];

  float Vx0 = V10*xmf + V00*(1.0 - xmf);
  float Vx1 = V11*xmf + V01*(1.0 - xmf);

  float value = ymf * Vx1 + (1.0 - ymf) * Vx0;
  return value;
}

int AstromOffsetMapFit (AstromOffsetMap *map, float *x, float *y, float *f, float *df, int Npts, int xdir) {

  int status = AstromOffsetMapFit_Mean (map, x, y, f, df, Npts, xdir);
  return status;
}

// given (x,y),value vector sets, choose the map that minimizes the difference between 
// the values and bilinear interpolation of the map
int AstromOffsetMapFit_Chisq (AstromOffsetMap *map, float *x, float *y, float *f, int Npts, int xdir) {

  int i, j, ix, iy, jx, jy;

  // choose to map direction:
  float *Vptr = xdir ? map->dXv : map->dYv;

  // special cases:
  if ((map->Nx == 1) && (map->Ny == 1)) {

    // no spatial information, just return (clipped?) median or mean or something
    VStatsType stats;
    stats.statmode = VSTATS_INNER_MEAN;

    vstats_getstats_f (f, NULL, NULL, Npts, &stats);
    Vptr[0] = stats.mean;
    return TRUE;
  }

  // special cases:
  if (map->Nx == 1) {
    fprintf (stderr, "1 x N not yet coded\n");
    exit (1);
    return TRUE;
  }

  if (map->Ny == 1) {
    fprintf (stderr, "N x 1 not yet coded\n");
    exit (1);
    return TRUE;
  }

  // this bit is copied nearly intact from psImageMapFit.c

  // set up the redirection table so we can use sA[-1][-1], etc
  float SAm[3][3], *SAv[3], **sA;

  for (i = 0; i < 3; i++) {
    SAv[i] = SAm[i] + 1;
  }
  sA = SAv + 1;

  int Nx = map->Nx;
  int Ny = map->Ny;

  double **A, **B;
  ALLOCATE (A, double *, Nx*Ny);
  ALLOCATE (B, double *, Nx*Ny);
  for (i = 0; i < Nx*Ny; i++) {
    ALLOCATE (A[i], double, Nx*Ny);
    memset (A[i], 0, sizeof(double)*Nx*Ny);
    ALLOCATE (B[i], double, 1);
    memset (B[i], 0, sizeof(double));
  }    

  // we are looping over the Nx,Ny image map elements;
  // the matrix equation contains Nx*Ny rows and columns
  // for (int n = 1; n < Nx - 1; n++) {
  // for (int m = 1; m < Ny - 1; m++) {

  if (0) {
    FILE *fd = fopen ("stats.dump.txt", "w");
    for (i = 0; i < Npts; i++) {
      fprintf (fd, "%d %f %f %f\n", i, x[i], y[i], f[i]);
    }
    fclose(fd);
  }

  // float Total = 0.0;
  for (ix = 0; ix < Nx; ix++) {
    for (iy = 0; iy < Ny; iy++) {
      // define & init summing variables
      double rx_rx_ry_ry = 0;
      double rx_rx_dy_ry = 0;
      double dx_rx_ry_ry = 0;
      double dx_rx_dy_ry = 0;
      double fi_rx_ry    = 0;
      double rx_rx_py_py = 0;
      double rx_rx_qy_py = 0;
      double dx_rx_py_py = 0;
      double dx_rx_qy_py = 0;
      double fi_rx_py    = 0;
      double px_px_ry_ry = 0;
      double px_px_dy_ry = 0;
      double qx_px_ry_ry = 0;
      double qx_px_dy_ry = 0;
      double fi_px_ry    = 0;
      double px_px_py_py = 0;
      double px_px_qy_py = 0;
      double qx_px_py_py = 0;
      double qx_px_qy_py = 0;
      double fi_px_py    = 0;

      // generate the sums for the fitting matrix element I,J
      // I = n + nX*m
      // J = (n + jn) + nX*(m + jm)
      for (i = 0; i < Npts; i++) {

	// data value & weight for this point
	if (!isfinite(f[i])) continue;

	// if (mask && (mask[i] & maskValue)) continue;

	// base coordinate offset for this point (x,y) relative to this map element (n,m)
	// double dx = psImageBinningGetRuffX (map->binning, x->data.F32[i]) - (n + 0.5);
	// double dy = psImageBinningGetRuffY (map->binning, y->data.F32[i]) - (m + 0.5);

	double dx = x[i] * map->dX - ix - 0.5;
	double dy = y[i] * map->dY - iy - 0.5;

	// XXX do I need to do something different at the edge or not?  I want to allow
	// x[i] to extend beyond the grid

# if (0)
	// edge cases to include:
	bool edgeX = false;
	edgeX |= ((n == 1) && (dx < -1.0));
	edgeX |= ((n == Nx - 2) && (dx > +1.0));

	bool edgeY = false;
	edgeY |= ((m == 1) && (dy < -1.0));
	edgeY |= ((m == Ny - 2) && (dy > +1.0));

	// skip points outside of 2x2 grid centered on n,m:
	if (!edgeX && (fabs(dx) > 1.0)) continue;
	if (!edgeY && (fabs(dy) > 1.0)) continue;
# endif

	if (fabs(dx) > 1.0) continue;
	if (fabs(dy) > 1.0) continue;

	// related offset values
	double rx = 1.0 - dx;
	double ry = 1.0 - dy;
	double px = 1.0 + dx;
	double py = 1.0 + dy;
	double qx = 0.0 - dx;
	double qy = 0.0 - dy;

	// sum the appropriate elements for the different quadrants
	int Qx = (dx >= 0) ? 1 : 0;
	int Qy = (dy >= 0) ? 1 : 0;

	// XXX why change the selection at the edges?
	// if (n ==      0) Qx = 1;
	// if (n == Nx - 1) Qx = 0;
	// if (m ==      0) Qy = 1;
	// if (m == Ny - 1) Qy = 0;

	float fi = f[i];
	assert (isfinite(fi));
	assert (isfinite(rx));
	assert (isfinite(ry));

	// points at offset 1,1
	if ((Qx == 1) && (Qy == 1)) {
	  rx_rx_ry_ry += rx*rx*ry*ry;
	  rx_rx_dy_ry += rx*rx*dy*ry;
	  dx_rx_ry_ry += dx*rx*ry*ry;
	  dx_rx_dy_ry += dx*rx*dy*ry;
	  fi_rx_ry    += fi*rx*ry;
	}
	// points at offset 1,0
	if ((Qx == 1) && (Qy == 0)) {
	  rx_rx_py_py += rx*rx*py*py;
	  rx_rx_qy_py += rx*rx*qy*py;
	  dx_rx_py_py += dx*rx*py*py;
	  dx_rx_qy_py += dx*rx*qy*py;
	  fi_rx_py    += fi*rx*py;
	}
	// points at offset 0,1
	if ((Qx == 0) && (Qy == 1)) {
	  px_px_ry_ry += px*px*ry*ry;
	  px_px_dy_ry += px*px*dy*ry;
	  qx_px_ry_ry += qx*px*ry*ry;
	  qx_px_dy_ry += qx*px*dy*ry;
	  fi_px_ry    += fi*px*ry;
	}
	// points at offset 0,0
	if ((Qx == 0) && (Qy == 0)) {
	  px_px_py_py += px*px*py*py;
	  px_px_qy_py += px*px*qy*py;
	  qx_px_py_py += qx*px*py*py;
	  qx_px_qy_py += qx*px*qy*py;
	  fi_px_py    += fi*px*py;
	}
      }

      // the chi-square derivatives have elements of the form g(n+jn,m+jm)*A(jn,jm),
      // jn,jm = -1 to +1. Convert the sums above into the correct coefficients
      sA[-1][-1] = qx_px_qy_py;
      sA[-1][ 0] = qx_px_ry_ry + qx_px_py_py;
      sA[-1][+1] = qx_px_dy_ry;
      sA[ 0][-1] = rx_rx_qy_py + px_px_qy_py;
      sA[ 0][ 0] = rx_rx_ry_ry + px_px_ry_ry + rx_rx_py_py + px_px_py_py;
      sA[ 0][+1] = rx_rx_dy_ry + px_px_dy_ry;
      sA[+1][-1] = dx_rx_qy_py;
      sA[+1][ 0] = dx_rx_ry_ry + dx_rx_py_py;
      sA[+1][+1] = dx_rx_dy_ry;

      // I[ 0][ 0] = index for this n,m element:
      int I = ix + Nx * iy;
      B[I][0] = fi_rx_ry + fi_rx_py + fi_px_ry + fi_px_py;

      // insert these values into their corresponding locations in A, B
      // float Sum = 0.0;
      for (jx = -1; jx <= +1; jx++) {
	if (ix + jx <   0) continue;
	if (ix + jx >= Nx) continue;
	for (jy = -1; jy <= +1; jy++) {
	  if (iy + jy <   0) continue;
	  if (iy + jy >= Ny) continue;
	  int J = (ix + jx) + Nx * (iy + jy);
	  A[J][I] = sA[jx][jy];
	  if (abs(A[J][I]) > 1000) {
	    fprintf (stderr, "A %d %d (%d %d : %d %d): %f\n", I, J, ix, iy, ix + jx, iy + jy, sA[jx][jy]);
	  }
	  // Sum += sA[jn][jm];
	}
      }
      // fprintf (stderr, "B %d (%d %d) : %f  :  %f\n", I, n, m, B->data.F32[I], Sum);
      // Total += Sum;
    }
  }
  // fprintf (stderr, "Total: %f\n", Total);

  // test for empty diagonal elements (unconstained cells), mark, and set pivots to 1.0
  int *Empty = NULL;
  ALLOCATE (Empty, int, Nx*Ny);

  // find the max value
  double MaxPivot = 0.0;
  for (i = 0; i < Nx*Ny; i++) {
    MaxPivot = MAX(A[i][i], MaxPivot);
  }

  // elements of A[i][j] are just the fractional weight of the number of input points to the pixel.
  // any pixels which have < 5% contribution are not very well constrained.  NAN them for now...
  double MinPivot = 0.05 * MaxPivot;
  for (i = 0; i < Nx*Ny; i++) {
    Empty[i] = 0;
    if (fabs(A[i][i]) < MinPivot) {
      Empty[i] = 1;
      for (j = 0; j < Nx*Ny; j++) {
	A[i][j] = 0.0;
	A[j][i] = 0.0;
      }
      A[i][i] = 1.0;
      B[i][0] = 0.0;
    }
  }

  if (1) {
    FILE *fd = fopen ("matrix.dat", "w");
    for (i = 0; i < Nx*Ny; i++) {
      for (j = 0; j < Nx*Ny; j++) {
	fprintf (fd, "%10.4f ", A[i][j]);
      }
      fprintf (fd, " : %10.4f\n", B[i][0]);
    }
    fclose (fd);
  }

  if (!dgaussjordan(A, B, Nx*Ny, 1)) {
    fprintf (stderr, "FAIL\n");
    exit (1);
  }

  // set bad values to NaN
  for (i = 0; i < Nx*Ny; i++) {
    if (Empty[i]) {
      B[i][0] = NAN;
      A[i][i] = 0;
    }
  }

  for (ix = 0; ix < Nx; ix++) {
    for (iy = 0; iy < Ny; iy++) {
      int I = ix + Nx * iy;
      Vptr[I] = B[I][0];
      // error[ix][iy] = sqrt(A[I][I]);
    }
  }

  // XXX free someone?
  for (i = 0; i < Nx*Ny; i++) {
    free (A[i]);
    free (B[i]);
  }    
  free (A);
  free (B);
  free (Empty);

  return TRUE;
}

// given (x,y),value vector sets, choose the map that minimizes the difference between 
// the values and bilinear interpolation of the map
int AstromOffsetMapFit_Mean (AstromOffsetMap *map, float *x, float *y, float *f, float *df, int Npts, int xdir) {

  int i, ix, iy;

  // choose to map direction:
  float *Vptr = xdir ? map->dXv : map->dYv;

  // measure clipped median in each bin
  VStatsType stats;
  stats.statmode = VSTATS_INNER_MEAN;

  int Nx = map->Nx;
  int Ny = map->Ny;
  int Npix = Nx*Ny;

  double **values, **dvalues;
  int    *Nvalue;
  ALLOCATE (Nvalue, int, Npix);
  ALLOCATE (values, double *, Npix);
  ALLOCATE (dvalues, double *, Npix);
  for (i = 0; i < Npix; i++) {
    ALLOCATE (values[i], double, Npts);
    ALLOCATE (dvalues[i], double, Npts);
    Nvalue[i] = 0;
  }

  // assign the points to the map cells
  for (i = 0; i < Npts; i++) {

    // data value & weight for this point
    if (!isfinite(f[i])) continue;
    if (df && !isfinite(df[i])) continue;
    if (df && df[i] == 0.0) continue;

    // if (mask && (mask[i] & maskValue)) continue;
    
    // base coordinate offset for this point (x,y) relative to this map element (n,m)
    // double dx = psImageBinningGetRuffX (map->binning, x->data.F32[i]) - (n + 0.5);
    // double dy = psImageBinningGetRuffY (map->binning, y->data.F32[i]) - (m + 0.5);

    // bin for this point
    ix = MAX(0, MIN(Nx-1, (int)(x[i] * map->dX)));
    iy = MAX(0, MIN(Ny-1, (int)(y[i] * map->dY)));
    int I = ix + Nx * iy;

    int N = Nvalue[I];
    values[I][N] = f[i];
    dvalues[I][N] = df ? df[i] : 1.0;
    Nvalue[I] ++;
  }    

  for (ix = 0; ix < Nx; ix++) {
    for (iy = 0; iy < Ny; iy++) {
      int I = ix + Nx * iy;
      if (Nvalue[I] > 5) {
	vstats_getstats (values[I], dvalues[I], NULL, Nvalue[I], &stats);
	Vptr[I] = stats.mean;
      } else {
	Vptr[I] = NAN;
      }
    }
  }

  // XXX free someone?
  for (i = 0; i < Npix; i++) {
    free (values[i]);
    free (dvalues[i]);
  }    
  free (values);
  free (dvalues);
  free (Nvalue);

  return TRUE;
}

// this function repairs an image with NAN pixels (only valid for a small-scale map -- no robust mean)
int AstromOffsetMapRepair (AstromOffsetMap *map, int xdir) {

  // we are going to repair the image by:
  // 1) finding NAN pixels
  // 2) if any of the neighbors are valid,
  //    replace with the mean of the neighbors
  // 3) otherwise, replace with the image mean

  int Nx = map->Nx;
  int Ny = map->Ny;

  int ix, iy;

  float *Vfix = NULL;
  ALLOCATE (Vfix, float, Nx*Ny);
  for (ix = 0; ix < Nx; ix++) {
    for (iy = 0; iy < Ny; iy++) {
      Vfix[ix + iy*Nx] = NAN;
    }
  }

  // find the global mean
  float mean = 0.0;
  float npix = 0.0;

  for (ix = 0; ix < Nx; ix++) {
    for (iy = 0; iy < Ny; iy++) {
      int I = ix + iy*Nx;
      float value = xdir ? map->dXv[I] : map->dYv[I];
      if (isnan(value)) continue;
      mean += value;
      npix += 1.0;
    }
  }
  mean /= npix;

  // find the NAN pixels:
  for (ix = 0; ix < Nx; ix++) {
    for (iy = 0; iy < Ny; iy++) {
      int I = ix + iy*Nx;
      float value = xdir ? map->dXv[I] : map->dYv[I];
      if (!isnan(value)) {
	Vfix[I] = value;
	continue;
      }

      // find mean of all possible neighbors
      float meanLocal = 0.0;
      float npixLocal = 0.0;

      int jx, jy;
      for (jx = -1; jx <= +1; jx++) {
	int nx = ix + jx;
	if (nx < 0) continue;
	if (nx >= Nx) continue;
	for (jy = -1; jy <= +1; jy++) {
	  int ny = iy + jy;
	  if (ny < 0) continue;
	  if (ny >= Ny) continue;
	  int I = ix + iy*Nx;
	  float value = xdir ? map->dXv[I] : map->dYv[I];
	  if (isnan(value)) continue;

	  meanLocal += value;
	  npixLocal += 1.0;
	}
      }
      // if there are no valid (non-NAN) local pixels, use global mean
      meanLocal = (npixLocal > 0.0) ? meanLocal / npixLocal : mean;
      Vfix[I] = meanLocal;
    }
  }
    
  // replace the bad pixels:
  for (ix = 0; ix < Ny; ix++) {
    for (iy = 0; iy < Ny; iy++) {
      int I = ix + iy*Nx;
      if (xdir) {
	map->dXv[I] = Vfix[I];
      } else {
	map->dYv[I] = Vfix[I];
      }
    }
  }
  free (Vfix);

  return TRUE;
}

int dump_map_data (float *x, float *y, float *f, int Npts, char *filename) {

  FILE *fout = fopen (filename, "w");
  
  int i;
  for (i = 0; i < Npts; i++) {
    fprintf (fout, "%d %f %f %f\n", i, x[i], y[i], f[i]);
  }
  fclose (fout);
  return TRUE;
} 


# include "data.h"
# define LL { \
 dx =  d00 / (*v01 - *v00); \
 dy = -d00 / (*v10 - *v00); \
 x = i - 0.5; \
 y = j - dy - 0.5; }

# define UL { \
 tmp = d00 / (*v10 - *v00); \
 dy = 1 - tmp; \
 dx =  d10 / (*v11 - *v10); \
 x = i - 0.5; \
 y = j + tmp - 0.5; }
      
# define LR { \
 tmp = d00 / (*v01 - *v00); \
 dx = 1 - tmp; \
 dy = d01 / (*v11 - *v01); \
 x = i + tmp - 0.5; \
 y = j - 0.5; }

# define UR { \
 tmp = d10 / (*v11 - *v10); \
 dx = 1 - tmp; \
 dy = -d11 / (*v01 - *v11); \
 x = i + tmp - 0.5; \
 y = j + 1 - 0.5; }
      
# define HZ { \
 tmp = d00 / (*v10 - *v00); \
 dy = d01 / (*v11 - *v01) - tmp; \
 dx = 1; \
 x = i - 0.5; \
 y = j + tmp - 0.5; }

# define VT { \
 tmp = d00 / (*v01 - *v00); \
 dx = d10 / (*v11 - *v10) - tmp; \
 x = i + tmp - 0.5; \
 dy = 1; \
 y = j - 0.5; }

Vector *xv, *yv;
int N, NVEC;

void DUMP (opihi_flt x, opihi_flt y, opihi_flt dx, opihi_flt dy) {
  
  xv[0].elements.Flt[N]   = x;
  xv[0].elements.Flt[N+1] = x+dx;
  yv[0].elements.Flt[N]   = y;
  yv[0].elements.Flt[N+1] = y+dy;
  
  N+=2;

  if (N >= NVEC - 2) {
    NVEC += 100;
    REALLOCATE (xv[0].elements.Flt, opihi_flt, NVEC);
    REALLOCATE (yv[0].elements.Flt, opihi_flt, NVEC);
  }
}

int contour (int argc, char **argv) {

  int i, j, Nx, Ny;
  opihi_flt x, y, dx, dy;
  float level, d00, d01, d10, d11, tmp;
  float *v00, *v01, *v10, *v11;
  float *matrix;
  Buffer *buf;
  
  if (argc != 5) {
    gprint (GP_ERR, "USAGE: contour <buffer> X Y level\n");
    return (FALSE);
  }

  if ((buf = SelectBuffer (argv[1], OLDBUFFER, TRUE)) == NULL) return (FALSE);
  if ((xv = SelectVector (argv[2], ANYVECTOR, TRUE)) == NULL) return (FALSE);
  if ((yv = SelectVector (argv[3], ANYVECTOR, TRUE)) == NULL) return (FALSE);

  level = atof (argv[4]);
  matrix = (float *)(buf[0].matrix.buffer);
  Nx = buf[0].matrix.Naxis[0];
  Ny = buf[0].matrix.Naxis[1];

  v00 = matrix;
  v01 = matrix + 1;
  v10 = matrix + Nx;
  v11 = matrix + Nx + 1;
  d01 = (level - *v00);
  d11 = (level - *v10);

  N = 0;
  NVEC = 100;
  ResetVector (xv, OPIHI_FLT, NVEC);
  ResetVector (yv, OPIHI_FLT, NVEC);

  for (j = 1; j < Ny; j++) {
    if (!(j%10)) gprint (GP_ERR, ".");
    for (i = 1; i < Nx; i++, v00++, v01++, v10++, v11++) {

      d00 = d01;
      d10 = d11;
      d01 = (level - *v01);
      d11 = (level - *v11);

      if (((d00 > 0) && (d01 > 0) && (d10 > 0) && (d11 > 0)) ||
	  ((d00 < 0) && (d01 < 0) && (d10 < 0) && (d11 < 0)))
	continue;

      if ((d00 > 0) && (d10 <= 0)) { 
	if ((d01 <= 0) && (d11 <= 0)) { /* \  */
	  LL;
	  DUMP (x,y,dx,dy);
	  continue;
	}
	if ((d01 > 0) && (d11 <= 0)) { /* -  */
	  HZ;
	  DUMP (x,y,dx,dy);
	  continue;
	}
	if ((d01 > 0) && (d11 > 0)) { /* /  */
	  UL;
	  DUMP (x,y,dx,dy);
	  continue;
	}
	if ((d01 <= 0) && (d11 > 0)) { /* \\  */
	  LL;
	  DUMP (x,y,dx,dy);
	  UR;
	  DUMP (x,y,dx,dy);
	  continue;
	}
      }

      if ((d00 <= 0) && (d10 > 0)) {
	if ((d01 > 0) && (d11 > 0)) { /* \  */
	  LL;
	  DUMP (x,y,dx,dy);
	  continue;
	}
	if ((d01 <= 0) && (d11 > 0)) { /* -  */
	  HZ;
	  DUMP (x,y,dx,dy);
	  continue;
	}
	if ((d01 <= 0) && (d11 <= 0)) { /* /  */
	  UL;
	  DUMP (x,y,dx,dy);
	  continue;
	}
	if ((d01 > 0) && (d11 <= 0)) { /* //  */
	  UL;
	  DUMP (x,y,dx,dy);
	  LR;
	  DUMP (x,y,dx,dy);
	  continue;
	}
      }
      

      if ((d00 <= 0) && (d10 <= 0)) { 
	if ((d01 > 0) && (d11 <= 0)) {
	  LR;
	  DUMP (x,y,dx,dy);
	  continue;
	}
	if ((d01 <= 0) && (d11 > 0)) {
	  UR;
	  DUMP (x,y,dx,dy);
	  continue;
	}
	if ((d01 > 0) && (d11 > 0)) {
	  VT;
	  DUMP (x,y,dx,dy);
	  continue;
	}
      }

      if ((d00 > 0) && (d10 > 0)) { 
	if ((d01 <= 0) && (d11 > 0)) {
	  LR;
	  DUMP (x,y,dx,dy);
	  continue;
	}
	if ((d01 > 0) && (d11 <= 0)) {
	  UR;
	  DUMP (x,y,dx,dy);
	  continue;
	}
	if ((d01 <= 0) && (d11 <= 0)) {
	  VT;
	  DUMP (x,y,dx,dy);
	  continue;
	}
      }

    }
    /* skip left-hand edge */
    v00++; v01++; v10++; v11++;
    d01 = (level - *v00);
    d11 = (level - *v10);
  }

  /****** bottom line *******/
  v00 = matrix;  
  v01 = matrix + 1;  
  y = 0;
  dx = 0;
  dy = -0.5;
  for (i = 1; i < Nx; i++, v00++, v01++) { /* do the edges */
    if (((*v00 > level) && (*v01 <= level)) || ((*v00 <= level) && (*v01 > level))) {
      x = i + (level - *v01)/(*v01 - *v00);
      DUMP (x,y,dx,dy);
    }
  }

  /********** top line *******/
  v00 = matrix + Nx*(Ny - 1);  
  v01 = v00 + 1;
  y = Ny - 1;
  dx = 0;
  dy = 0.5;
  for (i = 1; i < Nx; i++, v00++, v01++) { /* do the edges */
    if (((*v00 > level) && (*v01 <= level)) || ((*v00 <= level) && (*v01 > level))) {
      x = i + (level - *v01)/(*v01 - *v00);
      DUMP (x,y,dx,dy);
    }
  }

  /******** left line *********/
  v00 = matrix; 
  v01 = matrix + Nx;
  x = 0;
  dx = -0.5;
  dy = 0;
  for (j = 1; j < Ny; j++, v00+=Nx, v01+=Nx) { /* do the edges */
    if (((*v00 > level) && (*v01 <= level)) || ((*v00 <= level) && (*v01 > level))) {
      y = j + (level - *v01)/(*v01 - *v00);
      DUMP (x,y,dx,dy);
    }
  }

  /******** right line *********/
  v00 = matrix + Nx - 1; 
  v01 = v00 + Nx;
  x = Nx - 1;
  dx = 0.5;
  dy = 0;
  for (j = 1; j < Ny; j++, v00+=Nx, v01+=Nx) { /* do the edges */
    if (((*v00 > level) && (*v01 <= level)) || ((*v00 <= level) && (*v01 > level))) {
      y = j + (level - *v01)/(*v01 - *v00);
      DUMP (x,y,dx,dy);
    }
  }
  
/* free anything? */

  xv[0].Nelements = N;
  yv[0].Nelements = N;
  return (TRUE);
}

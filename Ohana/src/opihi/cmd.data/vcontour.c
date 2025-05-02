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

# define DUMP { \
overlay[Noverlay].type = KII_OVERLAY_LINE; \
overlay[Noverlay].x = Npix*x; \
overlay[Noverlay].y = Npix*y; \
overlay[Noverlay].dx = Npix*dx; \
overlay[Noverlay].dy = Npix*dy; \
Noverlay ++; \
CHECK_REALLOCATE (overlay, KiiOverlay, NOVERLAY, Noverlay, 1000); \
}

int vcontour (int argc, char **argv) {

  int i, j, ii, jj, n, Nbuf, Npix, Nx, Ny, Nline;
  float level, d00, d01, d10, d11, tmp;
  float x, y, dx, dy;
  float *v00, *v01, *v10, *v11;
  float *Vout, *Vin, *matrix;
  char *buffer, line[17];
  int Ximage, Nimage, N;
  Vector *xvec, *yvec, *zvec;

  gprint (GP_ERR, "vcontour not working yet\n");
  return (FALSE);

  if (argc != 7) {
    gprint (GP_ERR, "USAGE: vcontour x y z Xc Yc (level)\n");
    return (FALSE);
  }
  
  if ((xvec = SelectVector (argv[1], OLDVECTOR, TRUE)) == NULL) return (FALSE);
  if ((yvec = SelectVector (argv[2], OLDVECTOR, TRUE)) == NULL) return (FALSE);
  if ((zvec = SelectVector (argv[3], OLDVECTOR, TRUE)) == NULL) return (FALSE);
  if ((foo = SelectVector (argv[4], ANYVECTOR, TRUE)) == NULL) return (FALSE);
  if ((foo = SelectVector (argv[5], ANYVECTOR, TRUE)) == NULL) return (FALSE);

  level = atof (argv[6]);

  /* convert the x,y,z vectors to a matrix */
  *V = xvec[0].elements;
  max = min = *V;
  for (i = 0; i < xvec[0].Nelements; i++, V++) {
    if (!finite (*V)) continue;
    max = MAX (*V, max);
    min = MIN (*V, min);
  }      
  Xmax = max; Xmin = min;

  *V = yvec[0].elements;
  max = min = *V;
  for (i = 0; i < yvec[0].Nelements; i++, V++) {
    if (!finite (*V)) continue;
    max = MAX (*V, max);
    min = MIN (*V, min);
  }      
  Ymax = max; Ymin = min;

  /* not really finished */

  Noverlay = 0;
  NOVERLAY = 1000;
  ALLOCATE (overlay, KiiOverlay, NOVERLAY);

  v01 = matrix + 1;
  v10 = matrix + Nx;
  v11 = matrix + Nx + 1;
  d01 = (level - *v00);
  d11 = (level - *v10);
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
	  DUMP;
	  continue;
	}
	if ((d01 > 0) && (d11 <= 0)) { /* -  */
	  HZ;
	  DUMP;
	  continue;
	}
	if ((d01 > 0) && (d11 > 0)) { /* /  */
	  UL;
	  DUMP;
	  continue;
	}
	if ((d01 <= 0) && (d11 > 0)) { /* \\  */
	  LL;
	  DUMP;
	  UR;
	  DUMP;
	  continue;
	}
      }

      if ((d00 <= 0) && (d10 > 0)) {
	if ((d01 > 0) && (d11 > 0)) { /* \  */
	  LL;
	  DUMP;
	  continue;
	}
	if ((d01 <= 0) && (d11 > 0)) { /* -  */
	  HZ;
	  DUMP;
	  continue;
	}
	if ((d01 <= 0) && (d11 <= 0)) { /* /  */
	  UL;
	  DUMP;
	  continue;
	}
	if ((d01 > 0) && (d11 <= 0)) { /* //  */
	  UL;
	  DUMP;
	  LR;
	  DUMP;
	  continue;
	}
      }
      

      if ((d00 <= 0) && (d10 <= 0)) { 
	if ((d01 > 0) && (d11 <= 0)) {
	  LR;
	  DUMP;
	  continue;
	}
	if ((d01 <= 0) && (d11 > 0)) {
	  UR;
	  DUMP;
	  continue;
	}
	if ((d01 > 0) && (d11 > 0)) {
	  VT;
	  DUMP;
	  continue;
	}
      }

      if ((d00 > 0) && (d10 > 0)) { 
	if ((d01 <= 0) && (d11 > 0)) {
	  LR;
	  DUMP;
	  continue;
	}
	if ((d01 > 0) && (d11 <= 0)) {
	  UR;
	  DUMP;
	  continue;
	}
	if ((d01 <= 0) && (d11 <= 0)) {
	  VT;
	  DUMP;
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
      DUMP;
      continue;
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
      DUMP;
      continue;
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
      DUMP;
      continue;
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
      DUMP;
      continue;
    }
  }
  
  KiiLoadOverlay (Ximage, overlay, Noverlay, argv[2]);
  free (overlay);

  if (Npix != 1) free (matrix);
  gprint (GP_ERR, "\n");
}

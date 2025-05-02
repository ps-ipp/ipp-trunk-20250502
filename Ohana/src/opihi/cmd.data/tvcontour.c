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

int tvcontour (int argc, char **argv) {

  int i, j, ii, jj, Npix, Nx, Ny;
  float level, d00, d01, d10, d11, tmp;
  float x, y, dx, dy;
  float *v00, *v01, *v10, *v11;
  float *Vout, *Vin, *matrix;
  char *name;
  int kapa, N, Noverlay, NOVERLAY;
  Buffer *buf;
  KiiOverlay *overlay;
  
  name = NULL;
  if ((N = get_argument (argc, argv, "-n"))) {
    remove_argument (N, &argc, argv);
    name = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }
  if (!GetImage (NULL, &kapa, name)) return (FALSE);
  FREE (name);

  int channel = 0;
  if ((N = get_argument (argc, argv, "-ch"))) {
    channel = GetKapaChannelFromString (argv[N]);
    if (!channel) return FALSE;
    KiiSetChannel (kapa, channel - 1);
  }

  if ((argc != 4) && (argc != 5)) {
    gprint (GP_ERR, "USAGE: contour <buffer> (overlay) level [Npix]\n");
    return (FALSE);
  }

  if ((buf = SelectBuffer (argv[1], OLDBUFFER, TRUE)) == NULL) return (FALSE);

  level = atof (argv[3]);
  if (argc == 5) 
    Npix = (int) MAX (atof (argv[4]), 1); 
  else 
    Npix = 1;
  level *= Npix*Npix;

  /*** make rebinned image ***/
  Nx = buf[0].header.Naxis[0]/Npix;
  Ny = buf[0].header.Naxis[1]/Npix;
  if (Npix != 1) {
    gprint (GP_LOG, "rebin by a factor of %d (%d,%d)\n", Npix, Nx, Ny);
    ALLOCATE (matrix, float, Nx*Ny);
    bzero (matrix, Nx*Ny*sizeof(float));
	
    for (j = 0; j < Ny; j++) {
      for (jj = 0; jj < Npix; jj++) {
	Vout = matrix + j*Nx;
	Vin  = (float *)(buf[0].matrix.buffer) + (j*Npix + jj)*buf[0].header.Naxis[0];
	for (i = 0; i < Nx; i++, Vout++) {
	  for (ii = 0; ii < Npix; ii++, Vin++) {
	    *Vout += *Vin;
	  }
	}
      }
    }
  } else {
    gprint (GP_ERR, "using scale of 1\n");
    matrix = (float *)(buf[0].matrix.buffer);
  }

  Noverlay = 0;
  NOVERLAY = 1000;
  ALLOCATE (overlay, KiiOverlay, NOVERLAY);

  v00 = matrix;
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
    }
  }
  
  KiiLoadOverlay (kapa, overlay, Noverlay, argv[2]);
  free (overlay);

  if (Npix != 1) free (matrix);
  gprint (GP_ERR, "\n");
  return (TRUE);
}

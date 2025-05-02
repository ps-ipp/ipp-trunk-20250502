# include "data.h"

int imhist (int argc, char **argv) {
  
  int i, j, N, Nbins;
  int bin;
  float *V;
  double dx;
  Vector *vec1, *vec2;
  Buffer *buf;

  int Quiet = FALSE;
  if ((N = get_argument (argc, argv, "-q"))) {
    Quiet = TRUE;
    remove_argument (N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "-quiet"))) {
    Quiet = TRUE;
    remove_argument (N, &argc, argv);
  }

  float delta = 0;
  if ((N = get_argument (argc, argv, "-delta"))) {
    remove_argument (N, &argc, argv);
    delta = atof (argv[N]);
    remove_argument (N, &argc, argv);
  }

  int ClipNAN = FALSE;
  if ((N = get_argument (argc, argv, "-clip-nan"))) {
    remove_argument (N, &argc, argv);
    ClipNAN = TRUE;
  }  

  double min = 0.0, max = 0.0;
  if ((N = get_argument (argc, argv, "-range"))) {
    remove_argument (N, &argc, argv);
    min = atof (argv[N]);
    remove_argument (N, &argc, argv);
    max = atof (argv[N]);
    remove_argument (N, &argc, argv);
  }

  int sx = 0, sy = 0;
  int nx = 0, ny = 0;
  if ((N = get_argument (argc, argv, "-region"))) {
    remove_argument (N, &argc, argv);
    sx = atof (argv[N]);
    remove_argument (N, &argc, argv);
    sy = atof (argv[N]);
    remove_argument (N, &argc, argv);
    nx = atof (argv[N]);
    remove_argument (N, &argc, argv);
    ny = atof (argv[N]);
    remove_argument (N, &argc, argv);
  }

  if (argc != 4) {
    gprint (GP_ERR, "USAGE: histogram <buffer> <x> <y> [-region sx sy nx ny] [-range min max] [-delta binsize]\n");
    return (FALSE);
  }

  if ((buf = SelectBuffer (argv[1], OLDBUFFER, TRUE)) == NULL) return (FALSE);
  
  /* if either range is set to zero, use the rest of the chip */
  if (nx == 0)
    nx = buf[0].matrix.Naxis[0] - sx;
  if (ny == 0)
    ny = buf[0].matrix.Naxis[1] - sy;

  if ((sx < 0) || (sy < 0) || 
      (sx+nx > buf[0].matrix.Naxis[0]) || 
      (sy+ny > buf[0].matrix.Naxis[1])) {
    gprint (GP_ERR, "region out of range\n");
    return (FALSE);
  }

  if ((vec1 = SelectVector (argv[2], ANYVECTOR, TRUE)) == NULL) return (FALSE);
  if ((vec2 = SelectVector (argv[3], ANYVECTOR, TRUE)) == NULL) return (FALSE);

  /* unfortunately, we must do this in two passes:
     first pass finds the max and min and defines the bin size
     second pass counts the pixes in each bin 
     */

  if ((max == 0) && (min == 0)) {
    max = min = *((float *)(buf[0].matrix.buffer) + sy*buf[0].matrix.Naxis[0] + sx);
    gprint (GP_ERR, "sx: %d, sy: %d, first: %f\n", sx, sy, max);
    for (j = sy; j < sy + ny; j++) {
      V = (float *)(buf[0].matrix.buffer) + j*buf[0].matrix.Naxis[0] + sx; 
      for (i = 0; i < nx; i++, V++) {
	if (ClipNAN && isnan(*V)) continue;
	max = MAX (max, *V);
	min = MIN (min, *V);
      }
    }
  }

  if (delta == 0) {
    Nbins = 1024;
    dx = (max - min) / Nbins;
  } else {
    dx = delta;
    Nbins = (max - min) / dx;
  }
  if (Quiet) {
    set_variable ("MIN", min);
    set_variable ("MAX", max);
    set_variable ("DX",  dx);
  } else {
    gprint (GP_LOG, "max %f, min %f, dx %f\n", max, min, dx);
  }  

  ResetVector (vec1, OPIHI_FLT, Nbins + 1);
  ResetVector (vec2, OPIHI_FLT, Nbins + 1);
  bzero (vec1[0].elements.Flt, vec1[0].Nelements*sizeof(opihi_flt));
  bzero (vec2[0].elements.Flt, vec2[0].Nelements*sizeof(opihi_flt));
  
  for (j = sy; j < sy + ny; j++) {
    V = (float *)(buf[0].matrix.buffer) + j*buf[0].matrix.Naxis[0] + sx; 
    for (i = 0; i < nx; i++, V++) {
      if (ClipNAN && isnan(*V)) continue;
      bin = MAX (MIN (Nbins, (*V - min) / dx), 0);
      vec2[0].elements.Flt[bin] += 1.0;
    }
  }
  for (i = 0; i < Nbins + 1; i++, V++) {
    vec1[0].elements.Flt[i] = i*dx + min;
  }
  
  return (TRUE);
}


# include "data.h"

int rebin (int argc, char **argv) {
  
  int i, j, status, n, nx, ny, Nx, Ny, x, y, N, *Npix, *Vn;
  int Ignore, IgnoreValue, VERBOSE, Normalize, ExactScale;
  char temp[1024];
  float *Vout, *Vin, *Out, *In;
  double scale, scale2, fx, fy, dX, dY;
  Buffer *in, *out;

  Vn = Npix = NULL;
  Normalize = FALSE;
  if ((N = get_argument (argc, argv, "-norm"))) {
    remove_argument (N, &argc, argv);
    Normalize = TRUE;
  }

  VERBOSE = FALSE;
  if ((N = get_argument (argc, argv, "-v"))) {
    remove_argument (N, &argc, argv);
    VERBOSE = TRUE;
  }

  Ignore = FALSE;
  IgnoreValue = 0.0;
  if ((N = get_argument (argc, argv, "-ignore"))) {
    Ignore = TRUE;
    remove_argument (N, &argc, argv);
    IgnoreValue = atof (argv[N]);
    remove_argument (N, &argc, argv);
  }

  if (argc != 4) {
    gprint (GP_ERR, "USAGE: rebin <from> <to> scale \n");
    gprint (GP_ERR, "  negative integer scale expands image\n");
    return (FALSE);
  }

  if ((in  = SelectBuffer (argv[1], OLDBUFFER, TRUE)) == NULL) return (FALSE);
  if ((out = SelectBuffer (argv[2], ANYBUFFER, TRUE)) == NULL) return (FALSE);
  gfits_free_matrix (&out[0].matrix);
  gfits_free_header (&out[0].header);

  scale  = atof (argv[3]);
  if ((scale == (int) scale) || ((1.0/scale) == (int)(1.0/scale))) {
    ExactScale = TRUE;
    if (scale > 0) {
      nx = in[0].header.Naxis[0] / scale;
      ny = in[0].header.Naxis[1] / scale;
    } else {
      nx = in[0].header.Naxis[0] * fabs(scale);
      ny = in[0].header.Naxis[1] * fabs(scale);
    }    
  } else {
    ExactScale = FALSE;
    if (scale > 0) {
      nx = (int) (in[0].header.Naxis[0] / scale) + 1;
      ny = (int) (in[0].header.Naxis[1] / scale) + 1;
    } else {
      nx = (int) (in[0].header.Naxis[0] * fabs(scale)) + 1;
      ny = (int) (in[0].header.Naxis[1] * fabs(scale)) + 1;
    }      
  }
  if (VERBOSE) gprint (GP_LOG, "rebin %s to %s ("OFF_T_FMT","OFF_T_FMT" to %d,%d)\n", argv[1], argv[2],  in[0].header.Naxis[0],  in[0].header.Naxis[1], nx, ny);

  Nx = in[0].header.Naxis[0];
  Ny = in[0].header.Naxis[1];
  out[0].bitpix = in[0].bitpix;
  out[0].unsign = in[0].unsign;
  out[0].bscale = in[0].bscale;
  out[0].bzero  = in[0].bzero;
  gfits_copy_header (&in[0].header, &out[0].header);
  gfits_modify (&out[0].header, "NAXIS1", "%d", 1, nx);
  gfits_modify (&out[0].header, "NAXIS2", "%d", 1, ny);

  status =  gfits_scan (&out[0].header, "CDELT1", "%lf", 1, &dX);
  status &= gfits_scan (&out[0].header, "CDELT2", "%lf", 1, &dY);
  if (scale > 0) {
    dX *= scale;
    dY *= scale;
  } else {
    dX /= fabs(scale);
    dY /= fabs(scale);
  }    
  if (status) {
    gfits_modify (&out[0].header, "CDELT1", "%lf", 1, dX);
    gfits_modify (&out[0].header, "CDELT2", "%lf", 1, dY);
  }

  status =  gfits_scan (&out[0].header, "CRPIX1", "%lf", 1, &dX);
  status &= gfits_scan (&out[0].header, "CRPIX2", "%lf", 1, &dY);
  if (scale > 0) {
    dX /= scale;
    dY /= scale;
  } else {
    dX *= fabs(scale);
    dY *= fabs(scale);
  }    
  if (status) {
    gfits_modify (&out[0].header, "CRPIX1", "%lf", 1, dX);
    gfits_modify (&out[0].header, "CRPIX2", "%lf", 1, dY);
  }

  out[0].header.Naxis[0] = nx;
  out[0].header.Naxis[1] = ny;
  gfits_create_matrix (&out[0].header, &out[0].matrix);
  temp[0] = 0;
  if ((in[0].file[0] != '*') && (in[0].file[0] != '(')) {
    strcpy (temp, "*");
  }
  strcat (temp, in[0].file);
  strcpy (out[0].file, temp);

  if (Normalize) {
    ALLOCATE (Npix, int, nx*ny);
    bzero (Npix, nx*ny*sizeof(int));
  }

  if (ExactScale) {
    n = scale;
    if (n > 0) {
      for (j = 0; j < ny; j++) {
	for (y = 0; y < n; y++) {
	  Vout = (float *)(out[0].matrix.buffer) + j*nx;
	  Vin  = (float *)(in[0].matrix.buffer)  + (j*n + y)*in[0].header.Naxis[0];
	  if (Normalize) { Vn = Npix + j*nx; }
	  for (i = 0; i < nx; i++, Vout++) {
	    for (x = 0; x < n; x++, Vin++) {
	      if (isnan(*Vin)) continue;
	      if (isinf(*Vin)) continue;
	      if (Ignore && (*Vin == IgnoreValue)) continue;
	      *Vout += *Vin;
	      if (Normalize) {(*Vn) ++;}
	      // if ((i == 1) && (j == 1)) fprintf (stderr, "%d,%d : %d,%d : %f : %f : %d\n", i, j, x, y, *Vin, *Vout, *Vn);
	    }
	    if (Normalize) {Vn ++;}
	  }
	}
      }
    } else {
      n = fabs (n);
      for (j = 0; j < in[0].header.Naxis[1]; j++) {
	for (y = 0; y < n; y++) {
	  Vout = (float *)(out[0].matrix.buffer) + (j*n + y)*nx;
	  Vin  = (float *)(in[0].matrix.buffer)  + j*in[0].header.Naxis[0];
	  if (Normalize) { Vn = Npix + j*nx; }
	  for (i = 0; i < in[0].header.Naxis[0]; i++, Vin++) {
	    if (isnan(*Vin) || isinf(*Vin) || (Ignore && (*Vin == IgnoreValue))) { 
	      Vout += n; 
	      if (Normalize) Vn += n; 
	      continue; 
	    }
	    for (x = 0; x < n; x++, Vout++) {
	      *Vout = *Vin;
	      if (Normalize) {(*Vn) ++; Vn ++;}
	    }
	  }
	}
      }
    }
    if (Normalize) {
      Vn = Npix;
      Vout = (float *)out[0].matrix.buffer;
      for (i = 0; i < nx*ny; i++, Vout++, Vn++) {
	if (*Vn) { 
	  *Vout /= *Vn; 
	} else {
	  *Vout = 0;
	}
      }
    }
  } else {
    
    /* normalization is broken.  repair please */
    if (Normalize) { gprint (GP_ERR, "normalize not enabled for fractional scaling\n"); }

    if (scale < 0) scale = 1.0 / fabs(scale);
    In = (float *)in[0].matrix.buffer;
    Out = (float *)out[0].matrix.buffer;
    scale2 = scale*scale;
    if (scale > 1) {
      for (i = 0; i < Ny; i++) {
	y = 0.5 + (i - 0.5) / scale;
	fy = 0.5 + MIN (0.5, (y + 0.5) * scale - i);
	for (j = 0; j < Nx; j++, In++) {
	  x = 0.5 + (j - 0.5) / scale;
	  fx = 0.5 + MIN (0.5, (x + 0.5) * scale - j);
	  Vout = Out + y*nx + x;
	  *Vout += fx*fy*(*In);
	  if (fx < 1) {
	    *(Vout+1)    += (1-fx)*fy*(*In);
	  }
	  if (fy < 1) {
	    *(Vout+nx) += fx*(1-fy)*(*In);
	  }
	  if ((fx < 1) && (fy < 1)) {
	    *(Vout+1+nx) += (1-fx)*(1-fy)*(*In);
	  }
	}
      }
    } else {
      for (i = 0; i < ny; i++) {
	y = 0.5 + (i - 0.5) * scale;
	fy = 0.5 + MIN (0.5, (y + 0.5) / scale - i);
	for (j = 0; j < nx; j++, Out++) {
	  x = 0.5 + (j - 0.5) * scale;
	  fx = 0.5 + MIN (0.5, (x + 0.5) / scale - j);
	  Vin = In + y*Nx + x;
	  *Out += *Vin*fx*fy;
	  if (fx < 1) {
	    *Out += *(Vin+1)*(1-fx)*fy;
	  }
	  if (fy < 1) {
	    *Out += *(Vin+Nx)*fx*(1-fy);
	  }
	  if ((fx < 1) && (fy < 1)) {
	    *Out += *(Vin+1+Nx)*(1-fx)*(1-fy);
	  }
	  *Out = *Out * scale2;
	}
      }
    }
  }

  if (Normalize) free (Npix);

  return (TRUE);

}


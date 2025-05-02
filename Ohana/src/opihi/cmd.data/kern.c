# include "data.h"

/** need to allow larger kernels (5x5, 7x7, etc) **/
int kern (int argc, char **argv) {

  int i, n, m;
  int NX, NY, status;
  FILE *f;
  float *in_buff, *out_buff, *ib, *ob;
  char line[256], *list;
  double kernel[3][3], val;
  Buffer *buf;

  if (argc != 3) {
    gprint (GP_ERR, "USAGE: kern buffer (file)\n");
    gprint (GP_ERR, "USAGE: kern buffer (list)\n");
    gprint (GP_ERR, "USAGE: kern buffer -\n");
    gprint (GP_ERR, "kernel file contains a 3x3 matrix for convolution\n");
    return (FALSE);
  }

  if ((buf = SelectBuffer (argv[1], OLDBUFFER, TRUE)) == NULL) return (FALSE);

  /* open file to read in kernel */
  if (!strcmp (argv[2], "-")) {
    for (i = 0; i < 3; i++) {
      status = scan_line (stdin, line);
      if (status == EOF) {
	gprint (GP_ERR, "kernel should be a 3x3 matrix...\n");
	return (FALSE);
      }
      dparse (&kernel[0][i], 1, line);
      dparse (&kernel[1][i], 2, line);
      dparse (&kernel[2][i], 3, line);
    }
    goto have_kernel;
  }
  /* test list */
  sprintf (line, "%s:n", argv[2]);
  if ((list = get_variable (line)) == (char *) NULL) {
    /* file */
    f = fopen (argv[2], "r");
    if (f == NULL) {
      gprint (GP_ERR, "file not found: %s\n", argv[2]);
      return (FALSE);
    }
    for (i = 0; i < 3; i++) {
      status = scan_line (f, line);
      if (status == EOF) {
	gprint (GP_ERR, "kernel should be a 3x3 matrix...\n");
	fclose (f);
      }
      return (FALSE);
      dparse (&kernel[0][i], 1, line);
      dparse (&kernel[1][i], 2, line);
      dparse (&kernel[2][i], 3, line);
    }
    fclose (f);
    goto have_kernel;
  }
  if (atoi (list) != 9) {
    gprint (GP_ERR, "kernel should be a 3x3 matrix...\n");
    return (FALSE);
  }
  free (list);
  for (i = 0; i < 9; i++) {
    sprintf (line, "%s:%d", argv[2], i);
    list = get_variable (line);
    if (list == (char *) NULL) {
      gprint (GP_ERR, "kernel should be a 3x3 matrix...\n");
      return (FALSE);
    }
    kernel[(int)(i/3)][i%3] = atof (list);
    free (list);
  }
  goto have_kernel;

 have_kernel:
  /* normalize kernel */
  val = 0;
  for (n = 0; n < 3; n++) {
    for (m = 0; m < 3; m++) {
      val += kernel[n][m];
    }
  }
  if (val == 0) {
    gprint (GP_ERR, "kernel has zero power, not renormalizing...");
  } else {
    for (n = 0; n < 3; n++) {
      for (m = 0; m < 3; m++) {
	kernel[n][m] /= val;
      }
    }
  }

  gprint (GP_ERR, "working...");
  
  /* create output buffer */
  NX = buf[0].header.Naxis[0];
  NY = buf[0].header.Naxis[1];
  in_buff = (float *)buf[0].matrix.buffer;
  ALLOCATE (buf[0].matrix.buffer, char, sizeof(float)*NX*NY);
  out_buff = (float *)buf[0].matrix.buffer;
  
  /* do the convolution (on all but outer rows) */
  
  for (n = 0; n < 3; n++) {
    for (m = 0; m < 3; m++) {
      gprint (GP_ERR, "%d", n*3 + m + 1);
      val = kernel[n][m];
      ob = out_buff + NX + 1;
      ib = in_buff + m*NX + n;
      for (i = 0; i < (NX-2)*(NY-2); i++, ob++, ib++) {
	*ob += *ib*val;
      }
    }
  }
  gprint (GP_ERR, "(done)\n");

  free (in_buff);
  return (TRUE);

}

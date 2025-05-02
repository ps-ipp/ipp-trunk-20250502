# include "data.h"

int rotate (int argc, char **argv) {
  
  int i, j, NX, NY;
  float *in_buff, *out_buff, *c;
  double Xo, Yo, x, y, X1, Y1;
  double pc11, pc12, pc21, pc22, PC11, PC12, PC21, PC22;
  Buffer *buf;

//  Xo = 0;
//  Yo = 0;
//  if ((N = get_argument (argc, argv, "-center"))) {
//    remove_argument (N, &argc, argv);
//    Xo  = atof(argv[N]);
//    remove_argument (N, &argc, argv);
//    Yo  = atof(argv[N]);
//    remove_argument (N, &argc, argv);
//  }

  if (argc != 3) {
    gprint (GP_ERR, "USAGE: rotate <buffer> <angle>\n");
    return (FALSE);
  }
  if ((buf = SelectBuffer (argv[1], OLDBUFFER, TRUE)) == NULL) return (FALSE);

  if ((atof (argv[1]) < -180) || (atof (argv[1]) > 180)) {
    gprint (GP_ERR, "valid rotate angle between -180 and +180 degrees\n");
    return (FALSE);
  }

  /* save starting values */
  NX = buf[0].header.Naxis[0];
  NY = buf[0].header.Naxis[1];
  in_buff = (float *) buf[0].matrix.buffer;  /* don't lose reference */

  if (!strcasecmp (argv[2], "LEFT") || (atof (argv[2]) == -90)) {
    buf[0].header.Naxis[0] = NY;
    buf[0].header.Naxis[1] = NX;
    gfits_modify (&buf[0].header, "NAXIS1", "%d", 1, NY);
    gfits_modify (&buf[0].header, "NAXIS2", "%d", 1, NX);
    gfits_print_alt (&buf[0].header, "HISTORY", "%S", 1, "WARNING: rotated image!");
    gfits_create_matrix (&buf[0].header, &buf[0].matrix);
    out_buff = (float *)buf[0].matrix.buffer;
    for (i = NX - 1; i > -1; i--) {
      for (j = 0; j < NY; j++, out_buff++) {
	*out_buff = in_buff[i + j*NX];
      }
    }
    /* fix reference pixel */
    gfits_scan (&buf[0].header, "CRPIX1", "%lf", 1, &Xo);
    gfits_scan (&buf[0].header, "CRPIX2", "%lf", 1, &Yo);
    X1 = Yo;
    Y1 = NX - Xo;
    gfits_modify (&buf[0].header, "CRPIX1", "%lf", 1, X1);
    gfits_modify (&buf[0].header, "CRPIX2", "%lf", 1, Y1);
    
    /* fix rotate matrix */
    gfits_scan (&buf[0].header, "PC001001", "%lf", 1, &pc11);
    gfits_scan (&buf[0].header, "PC001002", "%lf", 1, &pc12);
    gfits_scan (&buf[0].header, "PC002001", "%lf", 1, &pc21);
    gfits_scan (&buf[0].header, "PC002002", "%lf", 1, &pc22);
    PC11 = pc21;
    PC12 = pc22;
    PC21 = -pc11;
    PC22 = -pc12;
    gfits_modify (&buf[0].header, "PC001001", "%le", 1, PC11);
    gfits_modify (&buf[0].header, "PC001002", "%le", 1, PC12);
    gfits_modify (&buf[0].header, "PC002001", "%le", 1, PC21);
    gfits_modify (&buf[0].header, "PC002002", "%le", 1, PC22);

    free (in_buff);
    return (TRUE);
  }

  if (!strcasecmp (argv[2], "RIGHT") || (atof (argv[2]) == 90)) {
    buf[0].header.Naxis[0] = NY;
    buf[0].header.Naxis[1] = NX;
    gfits_modify (&buf[0].header, "NAXIS1", "%d", 1, NY);
    gfits_modify (&buf[0].header, "NAXIS2", "%d", 1, NX);
    gfits_print_alt (&buf[0].header, "HISTORY", "%S", 1, "WARNING: rotated image!");
    gfits_create_matrix (&buf[0].header, &buf[0].matrix);
    out_buff = (float *)buf[0].matrix.buffer;
    for (i = 0; i < NX; i++) {
      for (j = NY - 1; j > -1; j--, out_buff++) {
	*out_buff = in_buff[i + j*NX];
      }
    }
    /* fix reference pixel */
    gfits_scan (&buf[0].header, "CRPIX1", "%lf", 1, &Xo);
    gfits_scan (&buf[0].header, "CRPIX2", "%lf", 1, &Yo);
    X1 = NY - Yo;
    Y1 = Xo;
    // gprint (GP_ERR, "%f %f -> %f %f\n", Xo, Yo, X1, Y1);
    gfits_modify (&buf[0].header, "CRPIX1", "%lf", 1, X1);
    gfits_modify (&buf[0].header, "CRPIX2", "%lf", 1, Y1);
    
    /* fix rotate matrix */
    gfits_scan (&buf[0].header, "PC001001", "%lf", 1, &pc11);
    gfits_scan (&buf[0].header, "PC001002", "%lf", 1, &pc12);
    gfits_scan (&buf[0].header, "PC002001", "%lf", 1, &pc21);
    gfits_scan (&buf[0].header, "PC002002", "%lf", 1, &pc22);
    PC11 = -pc21;
    PC12 = -pc22;
    PC21 = pc11;
    PC22 = pc12;
    // gprint (GP_ERR, "%f %f  ->  %f %f\n", pc11, pc12, PC11, PC12);
    // gprint (GP_ERR, "%f %f  ->  %f %f\n", pc21, pc22, PC21, PC22);
    gfits_modify (&buf[0].header, "PC001001", "%le", 1, PC11);
    gfits_modify (&buf[0].header, "PC001002", "%le", 1, PC12);
    gfits_modify (&buf[0].header, "PC002001", "%le", 1, PC21);
    gfits_modify (&buf[0].header, "PC002002", "%le", 1, PC22);

    free (in_buff);
    return (TRUE);
  }

  if (!strcasecmp (argv[2], "UPSIDE") || (atof (argv[2]) == -180) || (atof (argv[2]) == 180)) {
    gfits_print_alt (&buf[0].header, "HISTORY", "%S", 1, "WARNING: rotated image!");
    gfits_create_matrix (&buf[0].header, &buf[0].matrix);
    out_buff = (float *)buf[0].matrix.buffer;
    for (j = NY - 1; j > -1; j--) {
      for (i = NX - 1; i > -1; i--, out_buff++) {
	*out_buff = in_buff[i + j*NX];
      }
    }
    /* fix reference pixel */
    gfits_scan (&buf[0].header, "CRPIX1", "%lf", 1, &Xo);
    gfits_scan (&buf[0].header, "CRPIX2", "%lf", 1, &Yo);
    X1 = NX - Xo;
    Y1 = NY - Yo;
    gfits_modify (&buf[0].header, "CRPIX1", "%lf", 1, X1);
    gfits_modify (&buf[0].header, "CRPIX2", "%lf", 1, Y1);
    
    /* fix rotate matrix */
    gfits_scan (&buf[0].header, "PC001001", "%lf", 1, &pc11);
    gfits_scan (&buf[0].header, "PC001002", "%lf", 1, &pc12);
    gfits_scan (&buf[0].header, "PC002001", "%lf", 1, &pc21);
    gfits_scan (&buf[0].header, "PC002002", "%lf", 1, &pc22);
    PC11 = -pc11;
    PC12 = -pc12;
    PC21 = -pc21;
    PC22 = -pc22;
    gfits_modify (&buf[0].header, "PC001001", "%le", 1, PC11);
    gfits_modify (&buf[0].header, "PC001002", "%le", 1, PC12);
    gfits_modify (&buf[0].header, "PC002001", "%le", 1, PC21);
    gfits_modify (&buf[0].header, "PC002002", "%le", 1, PC22);

    free (in_buff);
    return (TRUE);
  }

  if (!strcasecmp (argv[2], "FLIPY")) {
    gfits_print_alt (&buf[0].header, "HISTORY", "%S", 1, "WARNING: rotated image!");
    gfits_create_matrix (&buf[0].header, &buf[0].matrix);
    out_buff = (float *)buf[0].matrix.buffer;
    for (j = NY - 1; j > -1; j--) {
      for (i = 0; i < NX; i++, out_buff++) {
	*out_buff = in_buff[i + j*NX];
      }
    }
    /* fix reference pixel */
    gfits_scan (&buf[0].header, "CRPIX1", "%lf", 1, &Xo);
    gfits_scan (&buf[0].header, "CRPIX2", "%lf", 1, &Yo);
    X1 = Xo;
    Y1 = NY - Yo;
    gfits_modify (&buf[0].header, "CRPIX1", "%lf", 1, X1);
    gfits_modify (&buf[0].header, "CRPIX2", "%lf", 1, Y1);
    
    /* fix rotate matrix */
    gfits_scan (&buf[0].header, "PC001001", "%lf", 1, &pc11);
    gfits_scan (&buf[0].header, "PC001002", "%lf", 1, &pc12);
    gfits_scan (&buf[0].header, "PC002001", "%lf", 1, &pc21);
    gfits_scan (&buf[0].header, "PC002002", "%lf", 1, &pc22);
    PC11 = pc11;
    PC12 = -pc12;
    PC21 = pc21;
    PC22 = -pc22;
    gfits_modify (&buf[0].header, "PC001001", "%le", 1, PC11);
    gfits_modify (&buf[0].header, "PC001002", "%le", 1, PC12);
    gfits_modify (&buf[0].header, "PC002001", "%le", 1, PC21);
    gfits_modify (&buf[0].header, "PC002002", "%le", 1, PC22);

    free (in_buff);
    return (TRUE);
  }

  if (!strcasecmp (argv[2], "FLIPX")) {
    gfits_print_alt (&buf[0].header, "HISTORY", "%S", 1, "WARNING: rotated image!");
    gfits_create_matrix (&buf[0].header, &buf[0].matrix);
    out_buff = (float *)buf[0].matrix.buffer;
    for (j = 0; j < NY; j++) {
      for (i = NX - 1; i > -1; i--, out_buff++) {
	*out_buff = in_buff[i + j*NX];
      }
    }
    /* fix reference pixel */
    gfits_scan (&buf[0].header, "CRPIX1", "%lf", 1, &Xo);
    gfits_scan (&buf[0].header, "CRPIX2", "%lf", 1, &Yo);
    X1 = NX - Xo;
    Y1 = Yo;
    gfits_modify (&buf[0].header, "CRPIX1", "%lf", 1, X1);
    gfits_modify (&buf[0].header, "CRPIX2", "%lf", 1, Y1);
    
    /* fix rotate matrix */
    gfits_scan (&buf[0].header, "PC001001", "%lf", 1, &pc11);
    gfits_scan (&buf[0].header, "PC001002", "%lf", 1, &pc12);
    gfits_scan (&buf[0].header, "PC002001", "%lf", 1, &pc21);
    gfits_scan (&buf[0].header, "PC002002", "%lf", 1, &pc22);
    PC11 = -pc11;
    PC12 = pc12;
    PC21 = -pc21;
    PC22 = pc22;
    gfits_modify (&buf[0].header, "PC001001", "%le", 1, PC11);
    gfits_modify (&buf[0].header, "PC001002", "%le", 1, PC12);
    gfits_modify (&buf[0].header, "PC002001", "%le", 1, PC21);
    gfits_modify (&buf[0].header, "PC002002", "%le", 1, PC22);

    free (in_buff);
    return (TRUE);
  }

  double angle = atof (argv[2]);
  double CosAngle = cos (angle*RAD_DEG);
  double SinAngle = sin (angle*RAD_DEG);
  
  gprint (GP_ERR, "rotating: %f %f %f\n", angle, CosAngle, SinAngle);

  // we are rotating about the center pixel, (NX/2, NY/2),
  // but we are then putting the result in a new image
  // of size (Lx,Ly).

  // (x,y) = (i - Nx/2),(j - Ny/2)
  // (x',y') = R(theta) (x,y)
  // (I,J) = (x' + Lx/2),(y' + Ly/2)

  int Lx = NX*fabs(CosAngle) + NY*fabs(SinAngle);
  int Ly = NX*fabs(SinAngle) + NY*fabs(CosAngle);

  /* fix reference pixel */
  gfits_scan (&buf[0].header, "CRPIX1", "%lf", 1, &Xo);
  gfits_scan (&buf[0].header, "CRPIX2", "%lf", 1, &Yo);
  X1 = (Xo - NX/2)*CosAngle + (Yo - NY/2)*SinAngle + Lx/2;
  Y1 = (NX/2 - Xo)*SinAngle + (Yo - NY/2)*CosAngle + Ly/2;
  gfits_modify (&buf[0].header, "CRPIX1", "%lf", 1, X1);
  gfits_modify (&buf[0].header, "CRPIX2", "%lf", 1, Y1);

  /* fix rotate matrix */
  gfits_scan (&buf[0].header, "PC001001", "%lf", 1, &pc11);
  gfits_scan (&buf[0].header, "PC001002", "%lf", 1, &pc12);
  gfits_scan (&buf[0].header, "PC002001", "%lf", 1, &pc21);
  gfits_scan (&buf[0].header, "PC002002", "%lf", 1, &pc22);
  PC11 = pc11*CosAngle + pc21*SinAngle;
  PC12 = pc12*CosAngle + pc22*SinAngle;
  PC21 = pc21*CosAngle - pc11*SinAngle;
  PC22 = pc22*CosAngle - pc12*SinAngle;
  gfits_modify (&buf[0].header, "PC001001", "%le", 1, PC11);
  gfits_modify (&buf[0].header, "PC001002", "%le", 1, PC12);
  gfits_modify (&buf[0].header, "PC002001", "%le", 1, PC21);
  gfits_modify (&buf[0].header, "PC002002", "%le", 1, PC22);

  buf[0].header.Naxis[0] = Lx;
  buf[0].header.Naxis[1] = Ly;
  gfits_modify (&buf[0].header, "NAXIS1", "%d", 1, Lx);
  gfits_modify (&buf[0].header, "NAXIS2", "%d", 1, Ly);
  gfits_create_matrix (&buf[0].header, &buf[0].matrix);
  gfits_print_alt (&buf[0].header, "HISTORY", "%S", 1, "WARNING: rotated image!");

  out_buff = (float *)buf[0].matrix.buffer;

  for (j = 0; j < Ly; j++) {
    for (i = 0; i < Lx; i++, out_buff++) {

      float xo = (i - Lx/2);
      float yo = (j - Ly/2);

      x =  xo*CosAngle + yo*SinAngle;
      y = -xo*SinAngle + yo*CosAngle;

      int I = x + NX/2;
      int J = y + NY/2;

      if (I < 0) continue;
      if (I >= NX - 1) continue;
      if (J < 0) continue;
      if (J >= NY - 1) continue;

      c = &in_buff[I + NX*J];

      int X = (int) x;
      int Y = (int) y;
      double fx = x - X;
      double fy = y - Y;

      *out_buff = (c[0]*(1-fx) + c[1]*fx)*(1-fy) + (c[NX+1]*fx + c[NX]*(1-fx))*fy;
//    *out_buff = c[0];
    }
  }
  free (in_buff);
  return (TRUE);

}

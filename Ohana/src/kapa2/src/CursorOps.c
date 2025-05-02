# include "Ximage.h"

// input coordinates are relative to the picture bounding box
// XXX pre-calculate fexpand, Xc, Yc when expand is set?
void Picture_to_Image (double *x1, double *y1, double x2, double y2, Picture *picture) {

  double expand, dx, dy;

  expand = 1.0;
  if (picture[0].expand > 0) {
    expand = 1 / (1.0*picture[0].expand);
  } else {
    expand = fabs((double)picture[0].expand);
  }
  
  // pixel coordinates in picture frame
  dx = expand*(x2 - (int)(0.5*picture[0].dx));
  dy = expand*(y2 - (int)(0.5*picture[0].dy));

  // set the true center (image and screen pixel boundaries must be aligned)
  // Xc = (int)(picture[0].Xc / expand) * expand;
  // Yc = (int)(picture[0].Yc / expand) * expand;

  // picture[0].X,Y is the image coordinate in the center of the picture
  *x1 = picture[0].flipx ? picture[0].Xc - dx : picture[0].Xc + dx;
  *y1 = picture[0].flipy ? picture[0].Yc - dy : picture[0].Yc + dy;
}

// input coordinates are relative to the X window
void Screen_to_Image (double *x1, double *y1, double x2, double y2, Picture *picture) {

  double xp, yp;

  // pixel coordinates in picture frame
  xp = x2 - picture[0].x;
  yp = y2 - picture[0].y;

  Picture_to_Image (x1, y1, xp, yp, picture);
}

void Image_to_Picture (double *x1, double *y1, double x2, double y2, Picture *picture) {

  double expand, dx, dy;

  /* notice that here, expand is the reciprocal of the expand above */
  expand = 1.0;
  if (picture[0].expand > 0) {
    expand = picture[0].expand;
  } else {
    expand = 1 / fabs((double)picture[0].expand);
  }
  
  // set the true center (image and screen pixel boundaries must be aligned)
  // Xc = ((int)(picture[0].Xc * expand)) / expand;
  // Yc = ((int)(picture[0].Yc * expand)) / expand;

  // pixel coordinates in picture frame
  dx = picture[0].flipx ? picture[0].Xc - x2 : x2 - picture[0].Xc;
  dy = picture[0].flipy ? picture[0].Yc - y2 : y2 - picture[0].Yc;
    
  *x1 = expand*dx + (int)(0.5*picture[0].dx);
  *y1 = expand*dy + (int)(0.5*picture[0].dy);
}

void Image_to_Screen (double *x1, double *y1, double x2, double y2, Picture *picture) {

  double xp, yp;

  Image_to_Picture (&xp, &yp, x2, y2, picture);

  // pixel coordinates in screen frame
  *x1 = xp + picture[0].x;
  *y1 = yp + picture[0].y;
}

// input coordinates are relative to the picture bounding box
void Picture_Lower (int *i_start, int *j_start, int *I_start, int *J_start, Matrix *matrix, Picture *picture) {

  double Ix, Iy, Sx, Sy;

  // Ix, Iy are the image coordinates of the specified screen pixel
  Picture_to_Image (&Ix, &Iy, 0.0, 0.0, picture);

  // round up (down) to nearest pixel boundary
  if (Ix > (int)(Ix)) {
    Ix = picture[0].flipx ? (int)(Ix) : (int)(Ix) + 1;
  }
  if (Iy > (int)(Iy)) {
    Iy = picture[0].flipy ? (int)(Iy) : (int)(Iy) + 1;
  }

  // Ix, Iy are now limited to valid image coordinates
  Ix = MIN (MAX (Ix, 0), matrix[0].Naxis[0]);
  Iy = MIN (MAX (Iy, 0), matrix[0].Naxis[1]);

  // I_start, J_start are the first displayed image pixel 
  *I_start = Ix;
  *J_start = Iy;

  // Sx, Sy are the screen coordinates of the Ix,Iy pixel
  Image_to_Picture (&Sx, &Sy, Ix, Iy, picture);
  
  // Sx,Sy should be forced into the pixel
  int Sxi = Sx + 0.5;
  int Syi = Sy + 0.5;

  // i_start, j_start are now limited to valid screen coordinates
  *i_start = MIN (MAX (Sxi, 0), picture[0].dx);
  *j_start = MIN (MAX (Syi, 0), picture[0].dy);
}
  
// input coordinates are relative to the picture bounding box
void Picture_Upper (int *i_end, int *j_end, int i_start, int j_start, Matrix *matrix, Picture *picture) {

  int nExtra;
  double Ix, Iy, Sx, Sy;

  // Ix, Iy are the image coordinates of the specified screen pixel
  Picture_to_Image (&Ix, &Iy, picture[0].dx, picture[0].dy, picture);

  // round down (up) to nearest pixel boundary
  if (Ix > (int)(Ix)) {
    Ix = picture[0].flipx ? (int)(Ix) + 1: (int)(Ix);
  }
  if (Iy > (int)(Iy)) {
    Iy = picture[0].flipy ? (int)(Iy) + 1: (int)(Iy);
  }

  // Ix, Iy are now limited to valid image coordinates
  Ix = MIN (MAX (Ix, 0), matrix[0].Naxis[0]);
  Iy = MIN (MAX (Iy, 0), matrix[0].Naxis[1]);

  // Sx, Sy are the screen coordinates of the Ix,Iy pixel
  Image_to_Picture (&Sx, &Sy, Ix, Iy, picture);

  // Sx,Sy should be forced into the pixel
  int Sxi = Sx + 0.5;
  int Syi = Sy + 0.5;

  // double IxTest, IyTest;
  // Picture_to_Image (&IxTest, &IyTest, Sx, Sy, picture);
  // fprintf (stderr, "dx,dy: %d,%d : %f,%f : %f,%f : %f:%f\n", picture[0].dx, picture[0].dy, Ix, Iy, Sx, Sy, IxTest, IyTest);

  // i_start, j_start are now limited to valid screen coordinates
  // XXX: i_end, j_end *should* be the last valid screen pixel plus 1
  *i_end = MIN (MAX (Sxi, 0), picture[0].dx);
  *j_end = MIN (MAX (Syi, 0), picture[0].dy);

  // round off error can leave us with a small number of extra pixels here.
  if (picture[0].expand > 1) {
    nExtra = (*i_end - i_start) % picture[0].expand;
    *i_end -= nExtra;
    nExtra = (*j_end - j_start) % picture[0].expand;
    *j_end -= nExtra;
  }
}

# include "markstar.h"
# define OLDSTYLE 1
# if (OLDSTYLE)
double opening_angle ();
# endif

Image *find_images (catstats, Nimages)
CatStats catstats[];
int *Nimages;
{
  
  Header header;
  Image *timage, *image;
  int i, j, k, found, nimage, Nimage, NIMAGE, NTIMAGE, Nloop, Nlast;
  int n, Nim, status;
  FILE *f;
  double Xc[6], Yc[6], Xi[6], Yi[6], r, d, x, y, dx, dy;
  Coords *tcoords;

  /* we make positional comparisons in the projection of catalog */
  tcoords = &catstats[0].coords;
  /* define catalog corners */
  Xc[0] = catstats[0].RA[0]; Yc[0] = catstats[0].DEC[0];
  Xc[1] = catstats[0].RA[1]; Yc[1] = catstats[0].DEC[0];
  Xc[2] = catstats[0].RA[1]; Yc[2] = catstats[0].DEC[1];
  Xc[3] = catstats[0].RA[0]; Yc[3] = catstats[0].DEC[1];
  Xc[4] = catstats[0].RA[0]; Yc[4] = catstats[0].DEC[0];
  Xc[5] = catstats[0].RA[1]; Yc[5] = catstats[0].DEC[1];
  for (j = 0; j < 6; j++) {
    r = Xc[j]; d = Yc[j];
    RD_to_XY (&Xc[j], &Yc[j], r, d, tcoords);
  }
  /* find Y positions of RA center */
  r = 0.5*(catstats[0].RA[0] + catstats[0].RA[1]);
  d = catstats[0].DEC[0];
  RD_to_XY (&x, &y, r, d, tcoords);
  Yc[0] = MIN (y, Yc[0]);
  Yc[1] = MIN (y, Yc[1]);
  Yc[4] = MIN (y, Yc[4]);
  /* find Y positions of RA center */
  r = 0.5*(catstats[0].RA[0] + catstats[0].RA[1]);
  d = catstats[0].DEC[1];
  RD_to_XY (&x, &y, r, d, tcoords);
  Yc[2] = MAX (y, Yc[2]);
  Yc[3] = MAX (y, Yc[3]);
  Yc[5] = MAX (y, Yc[5]);

  dx = 0.02*(Xc[2] - Xc[0]);
  dy = 0.02*(Yc[2] - Yc[0]);
  Xc[0] -= dx; Yc[0] -= dy;
  Xc[1] += dx; Yc[1] -= dy;
  Xc[2] += dx; Yc[2] += dy;
  Xc[3] -= dx; Yc[3] += dy;
  Xc[4] -= dx; Yc[4] -= dy;
  Xc[5] -= dx; Yc[5] -= dy;

  /* check if image datafile exists, get header, number of images */
  if (!gfits_read_header (ImageCat, &header)) {
    fprintf (stderr, "ERROR: No images in catalog %s (1)\n", ImageCat);
    exit (0);
  }
  Nimage = 0;
  gfits_scan (&header, "NIMAGES", "%d", 1, &Nimage);
  if (Nimage == 0) {
    fprintf (stderr, "ERROR: No images in catalog %s (1)\n", ImageCat);
    exit (0);
  }

  /* get ready to read data on images */ 
  f = fopen (ImageCat, "r");
  if (f == (FILE *) NULL) {
    fprintf (stderr, "ERROR: trouble opening Image catalog: %s (2)\n", ImageCat);
    exit (0);
  }
  fseeko (f, header.datasize, SEEK_SET); 

  /* set up buffers for images, temporary storage */
  NTIMAGE = 100;
  ALLOCATE (timage, Image, NTIMAGE);
  NIMAGE = 100;
  ALLOCATE (image, Image, NIMAGE);
  nimage = 0;
  Nloop = Nimage / NTIMAGE + 1;
  Nlast = Nimage % NTIMAGE;
  
  /* read in images in groups of NTIMAGE (100) */
  for (n = 0; n < Nloop; n++) {
    Nim = (n == Nloop - 1) ? Nlast : NTIMAGE;
    status = Fread (timage, sizeof(Image), Nim, f, "image");
    if (status != Nim) {
      fprintf (stderr, "ERROR: couldn't read images from image catalog: %s\n", ImageCat);
      exit (0);
    }
    /* test each image in block */
    for (i = 0; i < Nim; i++) {
      /* define image corners */
      Xi[0] = 0;            Yi[0] = 0;
      Xi[1] = timage[i].NX; Yi[1] = 0;
      Xi[2] = timage[i].NX; Yi[2] = timage[i].NY;
      Xi[3] = 0;            Yi[3] = timage[i].NY;
      Xi[4] = 0;            Yi[4] = 0;
      Xi[5] = timage[i].NX; Yi[5] = timage[i].NY;
      found = FALSE;
      /* transform to tcoords */
      if (catstats[0].DEC[1] > 86.25) { /* pole */
	for (j = 0; j < 6; j++) {
	  XY_to_RD (&r, &d, Xi[j], Yi[j], &timage[i].coords);
	  if (d > catstats[0].DEC[0] - 0.5) found = TRUE;
	}
      } else {
	for (j = 0; j < 6; j++) {
	  XY_to_RD (&r, &d, Xi[j], Yi[j], &timage[i].coords);
	  RD_to_XY (&Xi[j], &Yi[j], r, d, tcoords);
	}
	/* check if edges cross */
	for (j = 0; (j < 5) && !found; j++) {
	  for (k = 0; (k < 5) && !found; k++) {
	    found |= edge_check (&Xi[j], &Yi[j], &Xc[k], &Yc[k]);
	  }
	}
      }
      if (found) {
	image[nimage] = timage[i]; 
	image[nimage].code = 0;
	nimage ++;
	if (nimage == NIMAGE) {
	  NIMAGE += 100;
	  REALLOCATE (image, Image, NIMAGE);
	}
      }
    }
  }
      
  if (VERBOSE) { 
    for (i = 0; i < nimage; i++) {
      XY_to_RD (&r, &d, 0.5*image[i].NX, 0.5*image[i].NY, &image[i].coords);
      fprintf (stderr, "associated images: %d %8.4f %8.4f %10d %6d  %5.3f %6.3f %6.3f\n", 
	       i, r, d, image[i].tzero, image[i].nstar, 0.001*image[i].secz, 
	       0.001*image[i].Mcal, 0.001*image[i].dMcal);
    }
  }

  REALLOCATE (image, Image, MAX (nimage, 1));
  free (timage);
  *Nimages = nimage;
  fclose (f);
  return (image);
}

# if (OLDSTYLE)
int edge_check (x1, y1, x2, y2)
double *x1, *y1, *x2, *y2;
{

  double theta1, theta2;
  double Theta1, Theta2;

  theta1 = opening_angle (x1[0], y1[0], x2[0], y2[0], x1[1], y1[1]); 
  theta2 = opening_angle (x1[0], y1[0], x2[0], y2[0], x2[1], y2[1]); 

  if (theta1*theta2 < 0.0) {
    return (FALSE);
  }

  if (fabs(theta1) < fabs(theta2)) {
    return (FALSE);
  }

  Theta1 = theta1;
  Theta2 = theta2;
  theta1 = opening_angle (x2[0], y2[0], x1[1], y1[1], x2[1], y2[1]); 
  theta2 = opening_angle (x2[0], y2[0], x1[1], y1[1], x1[0], y1[0]); 
  
 
  if (theta1*theta2 < 0.0) {
    return (FALSE);
  }

  if (fabs(theta1) < fabs(theta2)) {
    return (FALSE);
  }

  return (TRUE);

}

double opening_angle (x1, y1, x2, y2, x3, y3)
double x1, y1, x2, y2, x3, y3;
{

  double dx1, dy1, dx2, dy2, ct, st, theta;

  dx1 = x1 - x2;
  dy1 = y1 - y2;
  
  dx2 = x3 - x2;
  dy2 = y3 - y2;
  
  ct = (dx1*dx2 + dy1*dy2);
  st = (dx1*dy2 - dx2*dy1);

  theta = atan2 (st, ct);

  return (theta);

}
#else 

int edge_check (x1, y1, x2, y2)
double *x1, *y1, *x2, *y2;
{

  double dot;
  double dx1, dx2, dy1, dy2, dx3, dy3, x, y;
  double cross, L1, L3, d1, d2;

  dx1 = x1[1] - x1[0]; dy1 = y1[1] - y1[0];
  dx2 = x2[1] - x2[0]; dy2 = y2[1] - y2[0];

  cross = dx2*dy1 - dx1*dy2;
  
  if (cross == 0) {  /* lines are parallel, are they inline? */
    dx3 = x2[1] - x1[0]; dy2 = y2[1] - y1[0];
    L1 = hypot (dx1,dy1);
    L3 = hypot (dx3,dy3);

    dot = fabs (dx1*dx3 + dy1*dy3) / (L1*L3);
    if (dot == 1.0) { /* lines are inline, do they overlap? */
      d1 = (x1[1]-x2[1])*(x1[1]-x2[0]) + (y1[1]-y2[1])*(y1[1]-y2[0]);
      d2 = (x1[0]-x2[1])*(x1[0]-x2[0]) + (y1[0]-y2[1])*(y1[0]-y2[0]);
      if (d1*d2 < 0) { /* lines overlap */
	return (TRUE);
      } else {
	return (FALSE);
      }
    } else {
      return (FALSE);
    }
  }

  x = (dx1*dx2*(y2[0] - y1[0]) + (x1[0]*dy1*dx2 - x2[0]*dy2*dx1)) / cross;

  if (dx1 != 0) {
    y = y1[0] + (x - x1[0])*dy1/dx1;
  } else {
    y = y2[0] + (x - x2[0])*dy2/dx2;
  }

  d1 = (x - x1[0])*(x - x1[1]) + (y - y1[0])*(y - y1[1]);
  d2 = (x - x2[0])*(x - x2[1]) + (y - y2[0])*(y - y2[1]);
  if ((d1 > 0) || (d2 > 0)) {
    return (FALSE);
  }

  return (TRUE);

}
      

#endif


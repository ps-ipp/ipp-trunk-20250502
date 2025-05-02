# include "markstar.h"
double opening_angle ();

Image *find_images (FITS_DB *db, CatStats *catstats, int *Nimages) {
  
  Header header;
  Image *timage, *image;
  int i, j, k, found, nimage, Nimage, NIMAGE, NTIMAGE, Nloop, Nlast;
  int n, Nim, status, InRange;
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
  for (j = 0; j < 5; j++) {
    r = Xc[j]; d = Yc[j];
    RD_to_XY (&Xc[j], &Yc[j], r, d, tcoords);
  }

  timage = gfits_table_get_Image (&db[0].ftable, &Ntimage, &db[0].scaledValue, &db[0].nativeOrder);
  if (!timage) {
    fprintf (stderr, "ERROR: failed to read images\n");
    exit (2);
  }

  /* set up buffers for images, temporary storage */
  nimage = 0;
  NIMAGE = 100;
  ALLOCATE (image, Image, NIMAGE);

  /* test each image in block */
  for (i = 0; i < Ntimage; i++) {
    /* define image corners */
    Xi[0] = 0;            Yi[0] = 0;
    Xi[1] = timage[i].NX; Yi[1] = 0;
    Xi[2] = timage[i].NX; Yi[2] = timage[i].NY;
    Xi[3] = 0;            Yi[3] = timage[i].NY;
    Xi[4] = 0;            Yi[4] = 0;
    found = FALSE;
    /* transform to tcoords */
    if (catstats[0].DEC[1] > 86.25) { /* pole */
      for (j = 0; j < 5; j++) {
	XY_to_RD (&r, &d, Xi[j], Yi[j], &timage[i].coords);
	if (d > catstats[0].DEC[0] - 0.5) found = TRUE;
      }
    } else {
      for (j = 0; j < 6; j++) {
	XY_to_RD (&r, &d, Xi[j], Yi[j], &timage[i].coords);
	InRange = RD_to_XY (&Xi[j], &Yi[j], r, d, tcoords);
	if (!InRange) {
	  /* if RD_to_XY returns false, the coords are ~180 away from
	     the projection center */ 
	  goto imskip;
	}
      }
      /* check if image corner inside catalog */
      for (j = 0; (j < 4) && !found; j++) {
	found |= corner_check (&Xi[j], &Yi[j], &Xc[0], &Yc[0]);
      }
      /* check if catalog corner inside image */
      for (j = 0; (j < 4) && !found; j++) {
	found |= corner_check (&Xc[j], &Yc[j], &Xi[0], &Yi[0]);
      }
      /* check if edges cross */
      for (j = 0; (j < 4) && !found; j++) {
	for (k = 0; (k < 4) && !found; k++) {
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
  imskip:
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
  *Nimages = nimage;
  return (image);
}

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

/* returns the opening angle between the three points (2 is in middle) 
   in range -pi to pi */
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

/* check if point x1,y1 is in box formed by x2[0-4] */
int corner_check (x1, y1, x2, y2)
double *x1, *y1, *x2, *y2;
{

  int i;
  double theta;

  theta = 0;

  for (i = 0; i < 4; i++) {
    theta += opening_angle (x2[i], y2[i], x1[0], y1[0], x2[i+1], y2[i+1]); 
  }
  if (fabs(theta) > 6) {
    return (TRUE);
  } else {
    return (FALSE);
  }
}

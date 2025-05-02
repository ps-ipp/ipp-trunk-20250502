# include "skycells.h"
# include "assert.h"
# define iSWAP(X,Y) {int tmp=(X); (X) = (Y); (Y) = tmp;}

// we use a static refcoords structure to avoid multiple alloc / init steps
static Coords *refcoords = NULL;

int sky_tessellation (FITS_DB *db, int level, int Nmax, int mode, double scale) {

  sky_tessellation_init (scale);

  switch (mode) {
    case SQUARES:
      sky_tessellation_squares (db, level, Nmax);
      return TRUE;
    case TRIANGLES:
      sky_tessellation_triangles (db, level, Nmax);
      return TRUE;
    case LOCAL:
      sky_tessellation_local (db, level, Nmax);
      return TRUE;
    case RINGS:
      sky_tessellation_rings (db, level, Nmax);
      return TRUE;
    case TAMAS:
      sky_tessellation_tamas (db, level, Nmax);
      return TRUE;
    case CFIS:
      sky_tessellation_cfis (db, level, Nmax);
      return TRUE;
    default:
      break;
  }

  return FALSE;
}

int sky_tessellation_triangles (FITS_DB *db, int level, int Nmax) {

  int i, j, Ndigit, Ntriangles, Nbase, Ntotal, Ltop, Nout, Nimages;
  double Ntop, fLtop;
  SkyTriangle *base, *tri, *new;
  Image *image;
  char format[16];

  // generate the initial base set
  base = sky_base_triangles (&Nbase);

  sky_base_rotation (base, Nbase);

  // how many triangles total for this level?
  Ntotal = Nbase*pow(4.0, level);
  Ndigit = (int)(log10(Ntotal)) + 1 ;
  snprintf_nowarn (format, 16, "skytri.%%0%dd", Ndigit);

  // to what depth do we need to go to have only Nmax foreach subcell?
  Ntop = Ntotal / Nmax;
  if (Ntop > Nbase) {
    fLtop = log10(Ntop / Nbase) / log10(4.0);
    if (fLtop > (int)(fLtop)) {
      Ltop = fLtop + 1;
    } else {
      Ltop = fLtop;
    }
  } else {
    Ltop = 0;
  }

  // subdivide the base set to Ltop level
  for (i = 0; i < Ltop; i++) {
    new = sky_divide_triangles (base, &Nbase);
    free (base);
    base = new;
  }

  // for each base triangle, subdivide the rest of the way and save
  Nout = 0;
  for (i = 0; i < Nbase; i++) {
    ALLOCATE (tri, SkyTriangle, 1);
    tri[0] = base[i];
    Ntriangles = 1;
    for (j = Ltop; j < level; j++) {
      new = sky_divide_triangles (tri, &Ntriangles);
      free (tri);
      tri = new;
    }

    // convert the SkyTriangles to Image
    ALLOCATE (image, Image, Ntriangles);
    for (j = 0; j < Ntriangles; j++) {
      sky_triangle_to_image (&image[j], &tri[j]);
      snprintf_nowarn (image[j].name, DVO_IMAGE_NAME_LEN, format, Nout);
      Nout++;
    }  
    Nimages = Ntriangles;

    /* add the new images and save */
    dvo_image_addrows (db, image, Nimages);
    SetProtect (TRUE);
    dvo_image_update (db, VERBOSE);
    SetProtect (FALSE);
    dvo_image_clear_vtable (db);

    free (image);
    free (tri);
  }
  return (TRUE);
}

int sky_tessellation_squares (FITS_DB *db, int level, int Nmax) {

  int i, j, Nname, Ndigit, Ntriangles, Nbase, Nimage, Ntotal, Ntop, Ltop, Nsubset, Nx, Ny;
  double fLtop;
  SkyTriangle *base, *tri, *new;
  SkyRectangle *rectangle, *subset;
  Image *image;
  char format[16];

  Nx = NX_SUB;
  Ny = NY_SUB;

  // generate the initial base set
  base = sky_base_triangles (&Nbase);

  sky_base_rotation (base, Nbase);

  // how many total cells for this level (multiply by subdivisions, if used)?
  Ntotal = Nbase*pow(4.0, level);
  Ndigit = (int)(log10(Ntotal)) + 1 ;
  snprintf_nowarn (format, 16, "skycell.%%0%dd", Ndigit);

  // to what depth do we need to go to have only Nmax foreach subcell?
  Ntop = Ntotal / (Nmax*Nx*Ny) ;
  if (Ntop > Nbase) {
    fLtop = log10(Ntotal / (double)(Ntop * Nbase)) / log10(4.0);
    if (fLtop > (int)(fLtop)) {
      Ltop = fLtop + 1;
    } else {
      Ltop = fLtop;
    }
  } else {
    Ltop = 0;
  }

  // subdivide the base set to Ltop level
  for (i = 0; i < Ltop; i++) {
    new = sky_divide_triangles (base, &Nbase);
    free (base);
    base = new;
  }

  // for each base triangle, subdivide the rest of the way and save
  Nname = 0;
  for (i = 0; i < Nbase; i++) {
    ALLOCATE (tri, SkyTriangle, 1);
    tri[0] = base[i];
    Ntriangles = 1;
    for (j = Ltop; j < level; j++) {
      new = sky_divide_triangles (tri, &Ntriangles);
      free (tri);
      tri = new;
    }

    // convert the SkyTriangles to SkyRectangles
    ALLOCATE (rectangle, SkyRectangle, Ntriangles);
    for (j = 0; j < Ntriangles; j++) {
      sky_triangle_to_rectangle (&rectangle[j], &tri[j]);
    }  

    // drop the appropriate subset
    ALLOCATE (subset, SkyRectangle, Ntriangles);
    for (j = Nsubset = 0; j < Ntriangles; j++) {
      if (!strcmp(rectangle[j].coords.ctype, "DROP")) continue;
      memcpy (&subset[Nsubset], &rectangle[j], sizeof(SkyRectangle));
      snprintf_nowarn (subset[Nsubset].name, DVO_IMAGE_NAME_LEN, format, Nname);
      Nname++;
      Nsubset++;
    }  
    free (rectangle);

    // subdivide each image (Nx x Ny subcells)
    Nimage = Nx*Ny*Nsubset;
    ALLOCATE (image, Image, Nimage);
    for (j = 0; j < Nsubset; j++) {
      // convert the SkyRectangles to Images for output
      sky_subdivide_image (&image[j*Nx*Ny], &subset[j], Nx, Ny);
    }

    /* add the new images and save */
    dvo_image_addrows (db, image, Nimage);
    SetProtect (TRUE);
    dvo_image_update (db, VERBOSE);
    SetProtect (FALSE);
    dvo_image_clear_vtable (db);

    free (subset);
    free (image);
    free (tri);
  }
  return (TRUE);
}

// use CENTER_RA and CENTER_DEC as the starting point.  generate a single projection 
// of size RANGE_RA and RANGE_DEC, and subdivide as specified.  This pseudo-tessellation may be used
// for local projects such as the PS1 Medium Deep fields
int sky_tessellation_local (FITS_DB *db, int level, int Nmax) {
  OHANA_UNUSED_PARAM(level);
  OHANA_UNUSED_PARAM(Nmax);

  int Nimage;
  SkyRectangle rectangle;
  Image *image;

  // this tessellation consists of a single projection center with Nx * Ny cells
  sky_rectangle_local (&rectangle);
  strcpy (rectangle.name, "skycell");

  if (strlen(PROJECTION_NUMBER) > 0) {
    // We store projection number as a string so one can pass in
    // values like 00, 01, etc. 
    strcat(rectangle.name, ".");
    strcat(rectangle.name, PROJECTION_NUMBER);
  }

  // subdivide each image (Nx x Ny subcells)
  Nimage = NX_SUB*NY_SUB;
  ALLOCATE (image, Image, Nimage);

  // convert the SkyRectangle to Images for output
  sky_subdivide_image (image, &rectangle, NX_SUB, NY_SUB);

  /* add the new images and save */
  dvo_image_addrows (db, image, Nimage);
  SetProtect (TRUE);
  dvo_image_update (db, VERBOSE);
  SetProtect (FALSE);
  dvo_image_clear_vtable (db);

  free (image);
  return (TRUE);
}

// the CADC / CFIS tessellation uses the constant DEC offsets and simple cos(DEC)-scaled 
// RA offsets.  The skycell names are defined to match the RA,DEC sequence
int sky_tessellation_cfis (FITS_DB *db, int level, int Nmax) {
  OHANA_UNUSED_PARAM(level);
  OHANA_UNUSED_PARAM(Nmax);

  char format[32];

  int Ndec = 0;

  // generate the a collection of rectangles for each ring
  for (double dec = -90.0; dec < +90.01; dec += CELLSIZE, Ndec ++) {

    snprintf_nowarn (format, 32, "skycell.%%03d.%03d", Ndec);

    int Nring;
    SkyRectangle *ring = sky_rectangle_cfis (dec, &Nring, format);
    if (!ring) continue;

    // nominal CFIS uses a single skycell per projection center

    // subdivide each image (Nx x Ny subcells)
    int Nimage = NX_SUB*NY_SUB*Nring;
    ALLOCATE_PTR (image, Image, Nimage);
    for (int j = 0; j < Nring; j++) {
      // convert the SkyRectangles to Images for output
      sky_subdivide_image (&image[j*NX_SUB*NY_SUB], &ring[j], NX_SUB, NY_SUB);
      // printf("%s %8.2f %8.2f\n", ring[j].name, ring[j].coords.crval1, ring[j].coords.crval2);
    }

    /* add the new images and save */
    dvo_image_addrows (db, image, Nimage);
    SetProtect (TRUE);
    dvo_image_update (db, VERBOSE);
    SetProtect (FALSE);
    dvo_image_clear_vtable (db);
    
    free (ring);
    free (image);
  }    
  return (TRUE);
}

// the RINGS tessellation uses the declination zones proposed by Tamas Budavari
// we generate projects on uniform rings of constant dec height
int sky_tessellation_rings (FITS_DB *db, int level, int Nmax) {
  OHANA_UNUSED_PARAM(level);
  OHANA_UNUSED_PARAM(Nmax);

  int j, nDEC, Nimage, Nring, Ntotal, Ndigit;
  float dec, dDEC;
  SkyRectangle *ring;
  Image *image;
  char format[16];

  // The tessellation has one input parameter: the approximate cell size.  Starting with
  // the cell size, determine the optimal projection cell height (dDEC) that results in an
  // integer number of dec zones between -90 and +90

  // in fact, we place a single image on each pole, so the real range of dec is 180.0 - CELLSIZE:

  nDEC = (180.0 - CELLSIZE) / CELLSIZE;
  dDEC = (180.0 - CELLSIZE) / nDEC;
  nDEC += 2;

  // how many total projection cells for this realization?  divide sky area by cell area:
  // this is used to set the number of digits, so it does not need to be very accurate...
  Ntotal = 41254.2 / (dDEC*dDEC);
  Ndigit = (int)(log10(Ntotal)) + 1 ;
  snprintf_nowarn (format, 16, "skycell.%%0%dd", Ndigit);

  // generate the a collection of rectangles for each ring
  for (dec = -90.0; dec < +90.0 + 0.5*dDEC; dec += dDEC) {

    ring = sky_rectangle_ring (dec, dDEC, &Nring, format);
    if (!ring) continue;


    // subdivide each image (Nx x Ny subcells)
    Nimage = NX_SUB*NY_SUB*Nring;
    ALLOCATE (image, Image, Nimage);
    for (j = 0; j < Nring; j++) {
      // convert the SkyRectangles to Images for output
      sky_subdivide_image (&image[j*NX_SUB*NY_SUB], &ring[j], NX_SUB, NY_SUB);
      // printf("%s %8.2f %8.2f\n", ring[j].name, ring[j].coords.crval1, ring[j].coords.crval2);
    }

    /* add the new images and save */
    dvo_image_addrows (db, image, Nimage);
    SetProtect (TRUE);
    dvo_image_update (db, VERBOSE);
    SetProtect (FALSE);
    dvo_image_clear_vtable (db);
    
    free (ring);
    free (image);
  }    
  return (TRUE);
}

// the RINGS tessellation uses the declination zones proposed by Tamas Budavari,
// based on code supplied by Tamas 2012.07.23
int sky_tessellation_tamas (FITS_DB *db, int level, int Nmax) {
  OHANA_UNUSED_PARAM(level);
  OHANA_UNUSED_PARAM(Nmax);

  int j, nDEC, Nimage, Nring, Ntotal, Ndigit;
  double dec, dDEC;
  SkyRectangle *ring;
  Image *image;
  char format[16];

  // The tessellation has one input parameter: the approximate cell size.  Starting with
  // the cell size, determine the optimal projection cell height (dDEC) that results in an
  // integer number of dec zones between -90 and +90

  // in fact, we place a single image on each pole, so the real range of dec is 180.0 - CELLSIZE:

  nDEC = (180.0 - CELLSIZE) / CELLSIZE;
  dDEC = (180.0 - CELLSIZE) / nDEC;
  nDEC += 2;

  // how many total projection cells for this realization?  divide sky area by cell area:
  // this is used to set the number of digits, so it does not need to be very accurate...
  Ntotal = 41254.2 / (dDEC*dDEC);
  Ndigit = (int)(log10(Ntotal)) + 1 ;
  snprintf_nowarn (format, 16, "skycell.%%0%dd", Ndigit);

  double d2r = M_PI / 180; // is RAD_DEG

  // parameter 'a' is the cell size in degrees
  double adeg = 3.955;

  // half of 'a' in radians and its atan
  double halfa = adeg / 2 * d2r;
  double halftheta = atan(halfa);

  // loop init
  dec = 0; // starting Decl. - could change this...
  
  while (dec < M_PI / 2 - halftheta) {
        double dm = dec - halftheta; // eq.5
        if (dec == 0) dm = 0; // initial

	// dec is modified by the call below
	ring = sky_rectangle_tamas (&dec, dm, halfa, halftheta, &Nring, format);
	if (!ring) continue;

	// subdivide each image (Nx x Ny subcells)
	Nimage = NX_SUB*NY_SUB*Nring;
	ALLOCATE (image, Image, Nimage);
	for (j = 0; j < Nring; j++) {
	  // convert the SkyRectangles to Images for output
	  sky_subdivide_image (&image[j*NX_SUB*NY_SUB], &ring[j], NX_SUB, NY_SUB);
	  // printf("%s %8.2f %8.2f\n", ring[j].name, ring[j].coords.crval1, ring[j].coords.crval2);
	}

	/* add the new images and save */
	dvo_image_addrows (db, image, Nimage);
	SetProtect (TRUE);
	dvo_image_update (db, VERBOSE);
	SetProtect (FALSE);
	dvo_image_clear_vtable (db);
    
	free (ring);
	free (image);
  }    
  return (TRUE);
}

// an allocated image is supplied, we fill in the values
int sky_triangle_to_image (Image *image, SkyTriangle *triangle) {

  int i, NX, NY;
  double xv[3], yv[3];	      // coordinates of the vertex in the reference projection 
  double scale;
  double Xmin, Xmax, Ymin, Ymax;

  // calculate the triangle coordinates in r,d
  sky_triangle_coords (triangle);

  // we will project to the triangle center position
  refcoords[0].crval1 = triangle[0].r;
  refcoords[0].crval2 = triangle[0].d;

  // project the vertices to this projection, find bounds
  Xmin = Xmax = Ymin = Ymax = 0.0; // 0,0 is center of triangle
  for (i = 0; i < 3; i++) {
    RD_to_XY (&xv[i], &yv[i], triangle[0].rv[i], triangle[0].dv[i], refcoords);
    Xmin = MIN (xv[i], Xmin);
    Xmax = MAX (xv[i], Xmax);
    Ymin = MIN (yv[i], Ymin);
    Ymax = MAX (yv[i], Ymax);
  }

  // set NX, NY to the roughly full-width box (centered at 0,0)
  NX = Xmax - Xmin;
  NY = Ymax - Ymin;

  memset (image, 0, sizeof(Image));
  image[0].coords = *refcoords;
  image[0].coords.pc1_1 = 1.0 * X_PARITY;
  image[0].coords.pc2_2 = 1.0;
  image[0].coords.pc1_2 = image[0].coords.pc2_1 = 0.0;

  // We cannot use the correction below if we want to set cdelt1,2 to our desired pixel scale
  // use this test to raise an error (60000 x 60000 is a very large image...)
  strcpy (image[0].coords.ctype, "TRI--TAN");
  scale = 0;
  for (i = 0; i < 3; i++) {
    scale = MAX (abs(xv[i]), scale);
    scale = MAX (abs(yv[i]), scale);
  }
  if (scale > 32000) {
    scale /= 30000.0;
    NX /= scale;
    NY /= scale;
    image[0].coords.cdelt1 *= scale;
    image[0].coords.cdelt2 *= scale;
    for (i = 0; i < 3; i++) {
      xv[i] /= scale;
      yv[i] /= scale;
    }
  }
  image[0].NX = NX;
  image[0].NY = NY;

  image[0].photcode = 1; // this needs to be set more sensibly

  // XXX these overload these value in a silly way
  image[0].dXpixSys = xv[0];  	   image[0].dYpixSys   = yv[0];
  image[0].dMagSys  = xv[1];  	   image[0].nFitAstrom = yv[1];
  image[0].photom_map_id = xv[2];  image[0].astrom_map_id = yv[2];

  return (TRUE);
}

// an allocated image is supplied, we fill in the values
// we are only keeping ~half of the images
int sky_triangle_to_rectangle (SkyRectangle *rectangle, SkyTriangle *triangle) {

  int i, parity, peak, b1, b2, NX, NY, right;
  double xv[3], yv[3];	      // coordinates of the vertex in the reference projection 
  double xo, yo, xc, yc, xcr, ycr, angle;
  double dB, dP, s1, s2, r1, r2, dr, dx, dy;
  double angle_b1, angle_b2, slope;

  // calculate the triangle coordinates in r,d
  sky_triangle_coords (triangle);

  // we will project to the triangle center position
  refcoords[0].crval1 = triangle[0].r;
  refcoords[0].crval2 = triangle[0].d;

  // find the size, rotation, and parity of the image
  // project the vertices and find the image parity
  parity = 1;
  for (i = 0; i < 3; i++) {
    RD_to_XY (&xv[i], &yv[i], triangle[0].rv[i], triangle[0].dv[i], refcoords);
    parity *= SIGN(yv[i]);
  }

  // choose the peak vertex
  peak = -1;
  for (i = 0; (peak == -1) && (i < 3); i++) {
    if (parity == SIGN(yv[i])) {
      peak = i;
    }
  }
  assert (peak != -1);

  // angle is from the center to the peak corner
  angle = atan2(parity*xv[peak], parity*yv[peak]); // note that this is x/y not y/x (and in radians)

  // find the base and height
  b1 = (peak + 1) % 3;
  b2 = (peak + 2) % 3;

  // angle is from the center to the peak corner
  angle_b1 = DEG_RAD*atan(yv[b1] / xv[b1]);
  angle_b2 = DEG_RAD*atan(yv[b2] / xv[b2]);

  // if one of the base-center angles is very small, the parity is marginal.  Use additional
  // information to choose the parity.  note that both angle_b1 and angle_b2 cannot be close to
  // zero.
  if (fabs(angle_b1) < 10.0) {
    right = (xv[b1] > 0);     // pointing left or right?
    slope = (xv[peak] -  xv[b2]) / (yv[peak] - yv[b2]);
    if ( right && (slope >= 0.0)) parity = +1;
    if ( right && (slope <  0.0)) parity = -1;
    if (!right && (slope <= 0.0)) parity = +1;
    if (!right && (slope >  0.0)) parity = -1;
    if (parity > 0) {
      if (yv[peak] < yv[b2]) iSWAP(peak, b2); // require peak to be top (bottom) point
    } else {
      if (yv[peak] > yv[b2]) iSWAP(peak, b2); // require peak to be top (bottom) point
    }
    angle = atan2(parity*xv[peak], parity*yv[peak]); // note that this is x/y not y/x (and in radians)
  }
  if (fabs(angle_b2) < 10.0) {
    right = (xv[b2] > 0);     // pointing left or right?
    slope = (xv[peak] - xv[b1]) / (yv[peak] - yv[b1]); // tilt of opposite line
    if ( right && (slope >= 0.0)) parity = +1;
    if ( right && (slope <  0.0)) parity = -1;
    if (!right && (slope <= 0.0)) parity = +1;
    if (!right && (slope >  0.0)) parity = -1;
    if (parity > 0) {
      if (yv[peak] < yv[b1]) iSWAP(peak, b1); // require peak to be top (bottom) point
    } else {
      if (yv[peak] > yv[b1]) iSWAP(peak, b1); // require peak to be top (bottom) point
    }
    angle = atan2(parity*xv[peak], parity*yv[peak]); // note that this is x/y not y/x (and in radians)
  }

  // xo, yo is the center of the baseline
  xo = 0.5*(xv[b2] + xv[b1]);
  yo = 0.5*(yv[b2] + yv[b1]);

  // find the max perpendicular distance from the peak

  // dB[b1] == dB[b2] (since xo,yo is the midpoint of [b1] to [b2]
  dB = hypot(xo      -xv[b1], yo      -yv[b1]);
  dP = hypot(xv[peak]-xo,     yv[peak] -yo);
	     
  // XXX we could just choose the point based on s1 vs s2...
  s1 = hypot(xv[peak]-xv[b1], yv[peak]-yv[b1]);
  s2 = hypot(xv[peak]-xv[b2], yv[peak]-yv[b2]);

  r1 = (SQ(s1) - SQ(dB) - SQ(dP)) / (2*dP);
  r2 = (SQ(s2) - SQ(dB) - SQ(dP)) / (2*dP);

  // dr >= 0
  dr = MAX (r1, r2);

  dx = -parity*dr*sin(angle);
  dy = -parity*dr*cos(angle);

  xo += dx;
  yo += dy;

  // xc, yc is the true image center
  xc = 0.5*(xv[peak] + xo);
  yc = 0.5*(yv[peak] + yo);

  // NX,NY are the size of the circumscribed square, expanded by PADDING
  NX = hypot((xv[b2]-xv[b1]),(yv[b2]-yv[b1])) * (1 + PADDING);
  NY = hypot((xv[peak]-xo),(yv[peak]-yo)) * (1 + PADDING);

  memset (rectangle, 0, sizeof(SkyRectangle));
  rectangle[0].coords = *refcoords;

  if (FIX_NS) {
    rectangle[0].coords.pc1_1 = +1.0 * X_PARITY;
    rectangle[0].coords.pc1_2 = +0.0;
    rectangle[0].coords.pc2_1 = -0.0;
    rectangle[0].coords.pc2_2 = +1.0;
    xcr = xc*cos(angle) - yc*sin(angle); 
    ycr = yc*cos(angle) + xc*sin(angle); 
  } else {
    rectangle[0].coords.pc1_1 = +cos(angle) * X_PARITY;
    rectangle[0].coords.pc1_2 = +sin(angle);
    rectangle[0].coords.pc2_1 = -sin(angle) * X_PARITY;
    rectangle[0].coords.pc2_2 = +cos(angle);
    xcr = xc;
    ycr = yc;
  }
  
  // crpix1,crpix2 is the projection center
  rectangle[0].coords.crpix1 = 0.5*NX - xcr;
  rectangle[0].coords.crpix2 = 0.5*NY - ycr;

  // only keep one of the parity rectangles
  if (((triangle[0].d >= 0) && (parity == +1)) || ((triangle[0].d < 0) && (parity == -1))) {
    strcpy (rectangle[0].coords.ctype, "DEC--TAN");
  } else {
    strcpy (rectangle[0].coords.ctype, "DROP");
  }

  rectangle[0].NX = NX;
  rectangle[0].NY = NY;
  rectangle[0].photcode = 1; // this needs to be set more sensibly

  return (TRUE);
}

// define the parameters of a single sky projection center
int sky_rectangle_local (SkyRectangle *rectangle) {

  int NX, NY;
  float angle;

  memset (rectangle, 0, sizeof(SkyRectangle));

  InitCoords (&rectangle[0].coords, "DEC--TAN");

  rectangle[0].coords.crval1 = CENTER_RA;
  rectangle[0].coords.crval2 = CENTER_DEC;

  // we will add this as an option later
  angle = 0.0;
  if (FIX_NS) {
    rectangle[0].coords.pc1_1 = +1.0 * X_PARITY;
    rectangle[0].coords.pc1_2 = +0.0;
    rectangle[0].coords.pc2_1 = -0.0;
    rectangle[0].coords.pc2_2 = +1.0;
  } else {
    rectangle[0].coords.pc1_1 = +cos(angle) * X_PARITY;
    rectangle[0].coords.pc1_2 = +sin(angle);
    rectangle[0].coords.pc2_1 = -sin(angle) * X_PARITY;
    rectangle[0].coords.pc2_2 = +cos(angle);
  }
  
  // range values are in projected degrees
  NX = RANGE_RA  * 3600.0 / SCALE;
  NY = RANGE_DEC * 3600.0 / SCALE;

  // crpix1,crpix2 is the projection center
  rectangle[0].coords.crpix1 = 0.5*NX;
  rectangle[0].coords.crpix2 = 0.5*NY;

  rectangle[0].coords.cdelt1 = SCALE / 3600.0;
  rectangle[0].coords.cdelt2 = SCALE / 3600.0;

  rectangle[0].NX = NX;
  rectangle[0].NY = NY;
  rectangle[0].photcode = 1; // this needs to be set more sensibly

  return (TRUE);
}

// define the parameters of a projection centers for this DEC band 
// dec : ~ center of band in Dec
// dDEC : approximate height
// nring : number of cells generated for this ring
// format : guide to generate the filenames (c-type string format)

// for skycell xxx.yyy, Dcenter = (yyy / 2 - 900), Rcenter = (xxx / 2 / dcos(Dcenter))

SkyRectangle *sky_rectangle_cfis (double dec, int *nring, char *format) {

  int i;

  // Subdivide the DEC center line into an integer number of segments:
  float dRA = CELLSIZE / cos(RAD_DEG*dec); // dRA is a size in RA degrees == \alpha_n
  int nRA = (360.0 + 2*CELLSIZE) / dRA;      // CELLSIZE is projection center spacing (DEC-direction)

  ALLOCATE_PTR (ring, SkyRectangle, nRA);

  for (i = 0; i < nRA; i++) {
    memset (&ring[i], 0, sizeof(SkyRectangle));

    InitCoords (&ring[i].coords, "DEC--TAN");
    ring[i].coords.crval1 = i*dRA;
    ring[i].coords.crval2 = dec;

    ring[i].coords.pc1_1 = +1.0 * X_PARITY;
  
    // we need to make this a user-defined option
    // for the default CFIS cells, PADDING is 0 (hard-wired into NX,NY = 10000)
    // ring[i].NX = NX*(1.0 + PADDING);
    // ring[i].NY = NY*(1.0 + PADDING);
    ring[i].NX = 10000;
    ring[i].NY = 10000;
    ring[i].photcode = 1; // this needs to be set more sensibly

    // crpix1,crpix2 is the projection center
    ring[i].coords.crpix1 = 0.5*ring[i].NX;
    ring[i].coords.crpix2 = 0.5*ring[i].NY;

    // user-supplied pixel scale
    ring[i].coords.cdelt1 = SCALE / 3600.0;
    ring[i].coords.cdelt2 = SCALE / 3600.0;

    // CFIS uses names with centers based on ra,dec
    // format = skycell.%03d.%03d
    snprintf_nowarn (ring[i].name, DVO_IMAGE_NAME_LEN, format, i);
  }

  *nring = nRA;
  return (ring);
}

// define the parameters of a projection centers for this ring 
// dec : ~ center of ring in Dec
// dDEC : approximate height
// nring : number of cells generated for this ring
// format : guide to generate the filenames (c-type string format)
SkyRectangle *sky_rectangle_ring (float dec, float dDEC, int *nring, char *format) {

  static int Nname = 0;
  int i, NX, NY, nRA;
  SkyRectangle *ring;
  float theta, dRA;

  // 'dec' is a guess at the center of the cell; in fact, we need to choose decLower and
  // decUpper to ensure complete overlap of the cells

  // we can determine the 'lower' bound (bound closest to the equator):
  float decLower = (dec > 0.0) ? dec - 0.5*dDEC : dec + 0.5*dDEC;

  // solve for actual cellsize (\theta):  tan(\delta_{n+1} - \theta/2) = tan(\delta_n + \theta/2)cos(\alpha_n / 2)
  float decUpper = (dec > 0.0) ? dec + dDEC : dec - dDEC;

  if (fabs(dec) + 0.5*dDEC > 90.0) {
    // onPole = TRUE;
    theta = dDEC;
    nRA = 1;
    dRA = theta / cos(decLower*RAD_DEG); // make a square at the pole
  } else {
    // onPole = FALSE;
    // Subdivide the 'lower' bound into an integer number of segments:
    nRA = cos(RAD_DEG*decLower) * 360.0 / CELLSIZE; // CELLSIZE is a projection size
    dRA = 360.0 / nRA;                         // dRA is a size in RA degrees == \alpha_n

    // tan(decUpper - theta/2) = tan(dec + theta/2) cos(dRA / 2);

    // we solve this equation for theta (fairly ugly: expand the tangents into sin/cos, expand the 
    // sum-of-angle sine and cosine, multiply through, convert via half-angle formulae and write 
    // as a quadratic expression in sine(theta/2)
  
    float sd1 = sin(RAD_DEG*decUpper);
    float cd1 = cos(RAD_DEG*decUpper);
    float sd2 = sin(RAD_DEG*dec);
    float cd2 = cos(RAD_DEG*dec);
    float   k = cos(RAD_DEG*dRA/2.0);

    float c1 =  (sd1*cd2 + sd2*cd1)*(1.0 - k);
    float c2 =  (sd1*cd2 - sd2*cd1)*(1.0 + k);
    float c3 = -(sd1*sd2 + cd1*cd2)*(1.0 + k); 

    float A = SQ(c3) + SQ(c2);
    float B = 2*c1*c3;
    float C = SQ(c1) - SQ(c2);

    float arg = SQ(B) - 4.0*A*C;

    float root;

    if (dec >= 0.0) {
      root = (-B + sqrt (arg)) / (2.0*A);
      theta = +DEG_RAD*asin(root);
    } else {
      root = (-B - sqrt (arg)) / (2.0*A);
      theta = -DEG_RAD*asin(root);
    }

    // the negative solution yields a negative cellsize 
    // float root2 = (-B - sqrt (arg)) / (2.0*A);
    // float theta2 = DEG_RAD*asin(root2);

    // test lines:
    // float r1 = tan(RAD_DEG*(decUpper - 0.5*theta1));
    // float r2 = tan(RAD_DEG*(dec + 0.5*theta1));
    // fprintf (stdout, "%f %f  %f  %f  %f %f  %f %f  %f %f %f\n", dec, decUpper, dRA, arg, root1, root2, theta1, theta2, r1, r2, k*r2);
  }
  // fprintf (stdout, "%f %f  %f x %f (%d)\n", dec, decUpper, dRA, theta, nRA);

  // I think we need to return the value of dec for the next ring, but I am not sure...

  ALLOCATE (ring, SkyRectangle, nRA);

  for (i = 0; i < nRA; i++) {
    memset (&ring[i], 0, sizeof(SkyRectangle));

    InitCoords (&ring[i].coords, "DEC--TAN");
    ring[i].coords.crval1 = i*dRA;
    ring[i].coords.crval2 = dec;

    ring[i].coords.pc1_1 = +1.0 * X_PARITY;
  
    // range values are in projected degrees
    NX = cos(decLower*RAD_DEG) * dRA   * 3600.0 / SCALE;
    NY =                         theta * 3600.0 / SCALE;

    // crpix1,crpix2 is the projection center
    ring[i].coords.crpix1 = 0.5*NX;
    ring[i].coords.crpix2 = 0.5*NY;

    ring[i].coords.cdelt1 = SCALE / 3600.0;
    ring[i].coords.cdelt2 = SCALE / 3600.0;

    ring[i].NX = NX*(1.0 + PADDING);
    ring[i].NY = NY*(1.0 + PADDING);
    ring[i].photcode = 1; // this needs to be set more sensibly

    snprintf_nowarn (ring[i].name, DVO_IMAGE_NAME_LEN, format, Nname);
    Nname++;

    // fprintf (stderr, "%f %f  : %f %f\n", 
    // ring[i].coords.crval1, ring[i].coords.crval2, 
    // ring[i].coords.crpix1, ring[i].coords.crpix2);
  }

  *nring = nRA;
  return (ring);
}

// define the parameters of a projection centers for this ring 
// dec : ~ center of ring in Dec
// dDEC : approximate height
// nring : number of cells generated for this ring
// format : guide to generate the filenames (c-type string format)
SkyRectangle *sky_rectangle_tamas (double *Dec, double dm, double halfa, double halftheta, int *nring, char *format) {
  OHANA_UNUSED_PARAM(halfa);

  static int Nname = 0;
  int i, j, NX, NY;
  SkyRectangle *ring;

  double d2r = M_PI / 180; // is RAD_DEG
  double dec = *Dec;

  int nRA = (int)ceil(M_PI * cos(dm) / halftheta);  // eq.6        
  double dRA = 2 * M_PI / nRA; // eq.7
  double dp = atan(tan(dec + halftheta) * cos(dRA / 2)); // eq.9

  if (dec == 0.0) {
    ALLOCATE (ring, SkyRectangle, nRA);
  } else {
    ALLOCATE (ring, SkyRectangle, 2*nRA);
  }

  for (i = 0; i < nRA; i++) {
    // R.A. can use different phase per ring 
    double ra = i * dRA; // + phase (watch wraparound) 

    int npass = (dec == 0.0) ? 1 : 2;
    for (j = 0; j < npass; j++) {

      int N = j*nRA + i;

      memset (&ring[N], 0, sizeof(SkyRectangle));
      InitCoords (&ring[N].coords, "DEC--TAN");

      ring[N].coords.crval1 = ra / d2r;
      ring[N].coords.crval2 = (j == 0) ? dec / d2r : -dec / d2r;

      // printf(" \t %d   %25.20f   %25.20f\n", i, ring[N].coords.crval2, ring[N].coords.crval1);

      ring[N].coords.pc1_1 = +1.0 * X_PARITY;
  
      // range values are in projected degrees
      NX = cos(dec - halftheta) * dRA   * 3600.0 / SCALE / d2r;
      NY =    2 * halftheta * 3600.0 / SCALE / d2r;

      // crpix1,crpix2 is the projection center
      ring[N].coords.crpix1 = 0.5*NX;
      ring[N].coords.crpix2 = 0.5*NY;

      ring[N].coords.cdelt1 = SCALE / 3600.0;
      ring[N].coords.cdelt2 = SCALE / 3600.0;

      ring[N].NX = NX*(1.0 + PADDING);
      ring[N].NY = NY*(1.0 + PADDING);
      ring[N].photcode = 1; // this needs to be set more sensibly

      snprintf_nowarn (ring[N].name, DVO_IMAGE_NAME_LEN, format, Nname);
      Nname++;
    }
  }

  // advance to next ring
  *Dec = halftheta + dp;

  *nring = (dec == 0.0) ? nRA : 2*nRA;
  return ring;
}

// an allocated image set is supplied, we fill in the values
int sky_subdivide_image (Image *output, SkyRectangle *input, int Nx, int Ny) {

  int i, j, N, NX, NY, Ndigit;
  char format[80];

  NX = input[0].NX/(double)Nx + 0.5;
  NY = input[0].NY/(double)Ny + 0.5;

  // image[0].NX,NY are unsigned short: abort is we overflow
  if ((NX > 0xffff) || (NY > 0xffff)) {
    fprintf (stderr, "error: NX,NY too big for DVO limits; modify pixel scale\n");
    fprintf (stderr, "NX: %d, NY: %d\n", NX, NY);
    exit (1);
  }

  if (Nx * Ny > 1) {
    Ndigit = (int)(log10(Nx*Ny)) + 1 ;
    snprintf_nowarn (format, 80, "%s.%%0%dd", input[0].name, Ndigit);
  } else {
    snprintf_nowarn (format, 80, "%s", input[0].name);
  }

  // if requested extend, the skycell boundaries so that skycells overlap
  int pad_x = (int) (OVERLAP_RA / SCALE);
  int pad_y = (int) (OVERLAP_DEC / SCALE);

  N = 0;
  for (j = 0; j < Ny; j++) {
    for (i = 0; i < Nx; i++) {

      memset (&output[N], 0, sizeof(Image));
      memcpy (&output[N].coords, &input[0].coords, sizeof(Coords));

      if (Nx + Ny > 1) {
	snprintf_nowarn (output[N].name, DVO_IMAGE_NAME_LEN, format, N);
      } else {
	snprintf_nowarn (output[N].name, DVO_IMAGE_NAME_LEN, "%s", format);
      }

      output[N].NX = NX + 2 * pad_x;
      output[N].NY = NY + 2 * pad_y;
      output[N].photcode = input[0].photcode;

      output[N].coords.crpix1 = input[0].coords.crpix1 - i*NX + pad_x;
      output[N].coords.crpix2 = input[0].coords.crpix2 - j*NY + pad_y;
      N++;
    }
  }
  return (TRUE);
}

int sky_triangle_coords (SkyTriangle *triangle) {

  int i;
  double r;

  // calculate the triangle center
  triangle[0].center.x = (triangle[0].vertex[0].x + triangle[0].vertex[1].x + triangle[0].vertex[2].x)/3.0;
  triangle[0].center.y = (triangle[0].vertex[0].y + triangle[0].vertex[1].y + triangle[0].vertex[2].y)/3.0;
  triangle[0].center.z = (triangle[0].vertex[0].z + triangle[0].vertex[1].z + triangle[0].vertex[2].z)/3.0;

  // renormalize
  r = 1.0 / sqrt (SQ(triangle[0].center.x) + SQ(triangle[0].center.y) + SQ(triangle[0].center.z));

  triangle[0].center.x *= r;
  triangle[0].center.y *= r;
  triangle[0].center.z *= r;

  triangle[0].d = DEG_RAD * asin(triangle[0].center.z);
  triangle[0].r = DEG_RAD * atan2(triangle[0].center.y, triangle[0].center.x);

  for (i = 0; i < 3; i++) {
    triangle[0].dv[i] = DEG_RAD * asin(triangle[0].vertex[i].z);
    triangle[0].rv[i] = DEG_RAD * atan2(triangle[0].vertex[i].y, triangle[0].vertex[i].x);
  }

  return TRUE;
}

// take a list of triangles from one level and return a list of triangles in the next level
// we are doing basic edge division, always yielding 4x as many new triangles as old;
SkyTriangle *sky_divide_triangles (SkyTriangle *in, int *ntriangles) {

  int i, j, Ntriangles, Nt;
  SkyTriangle *out;

  Ntriangles = *ntriangles * 4;
  ALLOCATE (out, SkyTriangle, Ntriangles);

  Nt = 0;
  for (i = 0; i < *ntriangles; i++) {
    for (j = 0; j < 3; j++) {
      out[4*i + j].vertex[0] = in[i].vertex[j];
      out[4*i + j].vertex[1] = sky_divide_edge (in[i].vertex[j], in[i].vertex[(j+1)%3]);
    }
    for (j = 0; j < 3; j++) {
      out[4*i + j].vertex[2] = out[4*i + (j+2)%3].vertex[1];
    }
    for (j = 0; j < 3; j++) {
      out[4*i + 3].vertex[j] = out[4*i + j].vertex[1];
    }
    Nt += 4;
  }
  *ntriangles = Nt;
  return (out);
}

// take a list of triangles from one level and return a list of triangles in the next level
// we are doing basic edge division, always yielding 4x as many new triangles as old;
Point sky_divide_edge (Point v1, Point v2) {

  double r;
  Point out;

  out.x = v1.x + v2.x;
  out.y = v1.y + v2.y;
  out.z = v1.z + v2.z;

  r = 1.0 / sqrt (SQ(out.x) + SQ(out.y) + SQ(out.z));

  out.x *= r;
  out.y *= r;
  out.z *= r;

  return (out);
}

SkyTriangle *sky_base_triangles (int *ntriangles) {

  SkyTriangle *tri;

  tri = NULL;

  switch (SOLID) {
    case TETRAHEDRON:
      fprintf (stderr, "TETRAHEDRON is not yet defined\n");
      exit (2);
      break;
    case CUBE:
      fprintf (stderr, "CUBE is not yet defined\n");
      exit (2);
      break;
    case OCTOHEDRON:
      fprintf (stderr, "OCTOHEDRON is not yet defined\n");
      exit (2);
      break;
    case DODECAHEDRON:
      fprintf (stderr, "DODECAHEDRON is not yet defined\n");
      exit (2);
      break;
    case ICOSAHEDRON:
      tri = sky_base_triangles_icosahedron (ntriangles);
  }

  return tri;

}

# define THETA RAD_DEG*26.565
# define D_PSI RAD_DEG*360.0/5.0

SkyTriangle *sky_base_triangles_icosahedron (int *ntriangles) {

  int i;
  double ctht, stht, psi;
  SkyTriangle *tri;

  // generate 0-level triangles
  ALLOCATE (tri, SkyTriangle, 20);

  for (i = 0; i < 20; i++) {
    memset (&tri[i], 0, sizeof(SkyTriangle));
  }

  ctht = cos(THETA);
  stht = sin(THETA);

  for (i = 0; i < 5; i++) {
    tri[i].vertex[0].x = +0;
    tri[i].vertex[0].y = +0;
    tri[i].vertex[0].z = +1;

    psi = (i + 0.0)*D_PSI;
    tri[i].vertex[1].x = +ctht*cos(psi);
    tri[i].vertex[1].y = +ctht*sin(psi);
    tri[i].vertex[1].z = +stht;
    
    psi = (i + 1.0)*D_PSI;
    tri[i].vertex[2].x = +ctht*cos(psi);
    tri[i].vertex[2].y = +ctht*sin(psi);
    tri[i].vertex[2].z = +stht;
  }    
  
  for (i = 5; i < 10; i++) {
    psi = (i + 0.0)*D_PSI;
    tri[i].vertex[0].x = +ctht*cos(psi);
    tri[i].vertex[0].y = +ctht*sin(psi);
    tri[i].vertex[0].z = +stht;
    
    psi = (i + 0.5)*D_PSI;
    tri[i].vertex[1].x = +ctht*cos(psi);
    tri[i].vertex[1].y = +ctht*sin(psi);
    tri[i].vertex[1].z = -stht;

    psi = (i + 1.0)*D_PSI;
    tri[i].vertex[2].x = +ctht*cos(psi);
    tri[i].vertex[2].y = +ctht*sin(psi);
    tri[i].vertex[2].z = +stht;
  }    
  
  for (i = 10; i < 15; i++) {
    psi = (i + 0.5)*D_PSI;
    tri[i].vertex[0].x = +ctht*cos(psi);
    tri[i].vertex[0].y = +ctht*sin(psi);
    tri[i].vertex[0].z = -stht;

    psi = (i + 1.0)*D_PSI;
    tri[i].vertex[1].x = +ctht*cos(psi);
    tri[i].vertex[1].y = +ctht*sin(psi);
    tri[i].vertex[1].z = +stht;
    
    psi = (i + 1.5)*D_PSI;
    tri[i].vertex[2].x = +ctht*cos(psi);
    tri[i].vertex[2].y = +ctht*sin(psi);
    tri[i].vertex[2].z = -stht;
  }    
  
  for (i = 15; i < 20; i++) {
    psi = (i + 0.5)*D_PSI;
    tri[i].vertex[1].x = +ctht*cos(psi);
    tri[i].vertex[1].y = +ctht*sin(psi);
    tri[i].vertex[1].z = -stht;
    
    psi = (i + 1.5)*D_PSI;
    tri[i].vertex[2].x = +ctht*cos(psi);
    tri[i].vertex[2].y = +ctht*sin(psi);
    tri[i].vertex[2].z = -stht;

    tri[i].vertex[0].x = +0;
    tri[i].vertex[0].y = +0;
    tri[i].vertex[0].z = -1;
  }    
  
  *ntriangles = 20;
  return tri;
}

int sky_base_rotation (SkyTriangle *base, int Nbase) {

  // apply the three euler angles (A, B, C)
  // XXX for now, just apply A and B

  int i, j, ix;
  float rot[3][3], v[3];

  rot[0][0] = +cos(EULER_A)*cos(EULER_B);
  rot[1][0] = +sin(EULER_A)*cos(EULER_B);
  rot[2][0] = +sin(EULER_B);

  rot[0][1] = -sin(EULER_A);
  rot[1][1] = +cos(EULER_A);
  rot[2][1] = +0.0;

  rot[0][2] = -cos(EULER_A)*sin(EULER_B);
  rot[1][2] = -sin(EULER_A)*sin(EULER_B);
  rot[2][2] = +cos(EULER_B);

  for (i = 0; i < Nbase; i++) {
    for (j = 0; j < 3; j++) {
      for (ix = 0; ix < 3; ix++) {
	v[ix] = 0.0;
	v[ix] += base[i].vertex[j].x * rot[0][ix];
	v[ix] += base[i].vertex[j].y * rot[1][ix];
	v[ix] += base[i].vertex[j].z * rot[2][ix];
      }
      base[i].vertex[j].x = v[0];
      base[i].vertex[j].y = v[1];
      base[i].vertex[j].z = v[2];
    }
  }
  return (TRUE);
}

int sky_tessellation_init (double scale) {

  ALLOCATE (refcoords, Coords, 1);
  InitCoords (refcoords, "DEC--TAN");
  refcoords[0].cdelt1 = refcoords[0].cdelt2 = scale / 3600;
  return (TRUE);
}

// free the space used by the current vtable entries
int dvo_image_clear_vtable (FITS_DB *db) {

  int i, nbytes;

  // free memory used by the current vtable rows
  for (i = 0; i < db[0].vtable.Nrow; i++) {
    free (db[0].vtable.buffer[i]);
  }
  REALLOCATE (db[0].vtable.buffer, char *, 1);
  REALLOCATE (db[0].vtable.row, off_t, 1);
  db[0].vtable.Nrow   = 0;

  // reset db[0].theader(NAXIS1) to match Image
  nbytes = sizeof(Image);
  gfits_modify (&db[0].theader, "NAXIS1", "%d", 1,  nbytes);
  db[0].theader.Naxis[0] = sizeof(Image);

  return (TRUE);
}

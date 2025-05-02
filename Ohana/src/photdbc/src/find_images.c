# include "photdbc.h"

/* this function returns a list of all images which overlap the given
   set of region files.  All images in the image catalog are tested
   once, so there is no check that an image already has been included.
   LineNum stores the locations in the Image database of the list of
   images */

Image *find_images (FITS_DB *db, GSCRegion *region, int Nregion, int *Nimage, int **LineNum) {
  
  Image *timage, *image;
  int i, j, k, m, found, nimage, Ntimage, NIMAGE;
  int InRange;
  double Xc[5], Yc[5], Xi[5], Yi[5], r, d, dx, dy;
  int *line_number;
  Coords tcoords;

  if (VERBOSE) fprintf (stderr, "finding images\n");

  /* we make positional comparisons in the projection of catalog */
  InitCoords (&tcoords, "DEC--TAN");
  tcoords.crval1 = 0.5*(region[0].RA[0]  + region[0].RA[1]);
  tcoords.crval2 = 0.5*(region[0].DEC[0] + region[0].DEC[1]);
  tcoords.cdelt1 = tcoords.cdelt2 = 1.0 / 3600.0;

  timage = gfits_table_get_Image (&db[0].ftable, &Ntimage, &db[0].scaledValue, &db[0].nativeOrder);
  if (!timage) {
      fprintf (stderr, "ERROR: failed to read images\n");
      exit (2);
  }

  nimage = 0;
  NIMAGE = 100;
  ALLOCATE (image, Image, NIMAGE);
  ALLOCATE (line_number, int, NIMAGE);
  
  for (i = 0; i < Ntimage; i++) {
      
# if (0)
    /* select images by photcode */
    ecode = GetPhotcodeEquivCodebyCode (timage[i].photcode);
    if (ecode != photcode[0].code) continue;

    /* select images by time */
    if (TimeSelect) {
      if (timage[i].tzero < TSTART) continue;
      if (timage[i].tzero > TSTOP) continue;
    }
# endif

    /* define image corners */
    Xi[0] = 0;            Yi[0] = 0;
    Xi[1] = timage[i].NX; Yi[1] = 0;
    Xi[2] = timage[i].NX; Yi[2] = timage[i].NY;
    Xi[3] = 0;            Yi[3] = timage[i].NY;
    Xi[4] = 0;            Yi[4] = 0;
    found = FALSE;
    /* transform to tcoords */
    for (j = 0; j < 5; j++) {
      XY_to_RD (&r, &d, Xi[j], Yi[j], &timage[i].coords);
      InRange = RD_to_XY (&Xi[j], &Yi[j], r, d, &tcoords);
      if (!InRange) goto imskip;
    }
    /* compare with each region file */
    for (m = 0; (m < Nregion) && !found; m++) { 
      /* define catalog corners */
      Xc[0] = region[m].RA[0]; Yc[0] = region[m].DEC[0];
      Xc[1] = region[m].RA[1]; Yc[1] = region[m].DEC[0];
      Xc[2] = region[m].RA[1]; Yc[2] = region[m].DEC[1];
      Xc[3] = region[m].RA[0]; Yc[3] = region[m].DEC[1];
      Xc[4] = region[m].RA[0]; Yc[4] = region[m].DEC[0];
      for (j = 0; j < 5; j++) {
	r = Xc[j]; d = Yc[j];
	RD_to_XY (&Xc[j], &Yc[j], r, d, &tcoords);
      }
      dx = 0.02*(Xc[2] - Xc[0]);
      dy = 0.02*(Yc[2] - Yc[0]);
      Xc[0] -= dx; Yc[0] -= dy;
      Xc[1] += dx; Yc[1] -= dy;
      Xc[2] += dx; Yc[2] += dy;
      Xc[3] -= dx; Yc[3] += dy;
      Xc[4] -= dx; Yc[4] -= dy;
      
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
      if (found) {
	image[nimage] = timage[i]; 
	if (image[nimage].code == ID_IMAGE_PHOTOM_NOCAL) {
	  image[nimage].code &= ~ID_IMAGE_PHOTOM_NOCAL;
	}	    
	line_number[nimage] = i;
	nimage ++;
	if (nimage == NIMAGE) {
	  NIMAGE += 100;
	  REALLOCATE (image, Image, NIMAGE);
	  REALLOCATE (line_number, int, NIMAGE);
	}
      }
    }
  imskip:
    continue;
  }
      
  if (VERBOSE) fprintf (stderr, "found %d images\n", nimage);

  /* we are going to use image[nimage] to represent images not in the database:
     these are measurements from external sources, like USNO */

  // assignMcal (&image[nimage], (double *) NULL, -1);
  image[nimage].Mcal = 0;
  image[nimage].code = ID_IMAGE_NEW;
  
  REALLOCATE (image, Image, MAX (nimage + 1, 1));
  REALLOCATE (line_number, int, MAX (nimage + 1, 1));
  *Nimage = nimage;
  *LineNum = line_number;
  return (image);
}


# include "uniphot.h"

Group *find_image_sgroups (FITS_DB *db, ImageLink **Imlink, int *Nsgroup) {

  off_t i, j, Nimage;
  int Ngroup, Nentry, NENTRY;
  double r, d, x, y, radius;
  Group *group;
  Coords coords;
  Image *image;
  ImageLink *imlink;

  imlink = *Imlink;
  image = gfits_table_get_Image (&db[0].ftable, &Nimage, &db[0].scaledValue, &db[0].nativeOrder);
  if (!image) {
      fprintf (stderr, "ERROR: failed to read images\n");
      exit (2);
  }

  InitCoords (&coords, "DEC--TAN");
  
  Ngroup = 0;
  ALLOCATE (group, Group, Nimage);

  if (VERBOSE) fprintf (stderr, "finding images\n");
  BuildChipMatch (image, Nimage);
  // MARKTIME("build chip match: %f sec\n", dtime);

  /* set imlink.sgroups = NULL as a marker */
  for (i = 0; i < Nimage; i++) imlink[i].sgroup = NULL;

  for (i = 0; i < Nimage; i++) {
    if (imlink[i].sgroup != NULL) continue;
    if (image[i].flags & ID_IMAGE_PHOTOM_NOCAL) continue;

    // XXX optionally, we should be able to use ONLY the DIS or NOT the DIS images
    // NOCAL above is used to mark images which do not match the photcode (including the DIS)
    // if (!strcmp(&image[i].coords.ctype[4], "-DIS")) continue;

    /* define image center - note the DIS images (mosaic phu) are special */
    if (!strcmp(&image[i].coords.ctype[4], "-DIS")) {
	XY_to_RD (&r, &d, 0.0, 0.0, &image[i].coords);
    } else {
	XY_to_RD (&r, &d, 0.5*image[i].NX, 0.5*image[i].NX, &image[i].coords);
    }

    /* new sgroup, set ref coords */
    coords.crval1 = r;
    coords.crval2 = d;

    /* init sgroup structure */
    Nentry = 0;
    NENTRY = 100;
    ALLOCATE (group[Ngroup].image, Image *, NENTRY);
    ALLOCATE (group[Ngroup].imlink, ImageLink *, NENTRY);
    group[Ngroup].M = 0;
    group[Ngroup].dM = 0;
    snprintf (group[Ngroup].label, 64, "%10.6f - %10.6f", r, d);
    group[Ngroup].v1 = r;
    group[Ngroup].v2 = d;

    /* link this image to sgroup */
    group[Ngroup].image[Nentry] = &image[i];
    group[Ngroup].imlink[Nentry] = &imlink[i];
    imlink[i].sgroup = &group[Ngroup];
    Nentry ++;

    for (j = 0; j < Nimage; j++) {
      if (image[j].flags & ID_IMAGE_PHOTOM_NOCAL) continue;
      if (imlink[j].sgroup != NULL) continue;
      // XXX optionally, we should be able to use ONLY the DIS or NOT the DIS images
      // NOCAL above is used to mark images which do not match the photcode (including the DIS)
      // if (!strcmp(&image[j].coords.ctype[4], "-DIS")) continue;

      /* project image center to local coords, check radius */
      if (!strcmp(&image[j].coords.ctype[4], "-DIS")) {
	  XY_to_RD (&r, &d, 0.0, 0.0, &image[j].coords);
      } else {
	  XY_to_RD (&r, &d, 0.5*image[j].NX, 0.5*image[j].NX, &image[j].coords);
      }
      if (!RD_to_XY (&x, &y, r, d, &coords)) continue; 

      /* RD_to_XY returns FALSE if opposite hemispheres */
      radius = hypot (x, y);

      if (radius > RADIUS) continue;

      /* image in sgroup, add to entry */
      group[Ngroup].image[Nentry] = &image[j];
      group[Ngroup].imlink[Nentry] = &imlink[j];
      imlink[j].sgroup = &group[Ngroup];
      Nentry ++;
      if (Nentry == NENTRY) {
	NENTRY += 100;
	REALLOCATE (group[Ngroup].image, Image *, NENTRY);
	REALLOCATE (group[Ngroup].imlink, ImageLink *, NENTRY);
      }
    }
    group[Ngroup].Nimage = Nentry;
    Ngroup ++;
  }
  *Nsgroup = Ngroup;
  return (group);
}

  /* this is a bit weak: since we use pointers, we can't
     reallocate group after the pointers are assigned.
     therefore, we allocate the max possible groups */

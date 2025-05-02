# include "uniphot.h"
# include <glob.h>

// update images & catalogs for uniphot
void update_dvo_uniphot (FITS_DB *db, Group *sgroup, int Nsgroup) {

  off_t i, Nimage, Nkeep, *keep;
  int j, Nmin;
  char line[256];
  glob_t pglob;
  double Rmin, Rmax, Dmin, Dmax, x, y, radius;
  Image *image;
  Catalog catalog;
  Coords coords;

  image = gfits_table_get_Image (&db[0].ftable, &Nimage, &db[0].scaledValue, &db[0].nativeOrder);
  if (!image) {
      fprintf (stderr, "ERROR: failed to read images\n");
      exit (2);
  }

  // create a subset list so we can make a vtable
  Nkeep = 0;
  ALLOCATE (keep, off_t, Nimage);

  // identify the images used and clear the NOCAL flags on the rest
  for (i = 0; i < Nimage; i++) {
      if (image[i].flags & ID_IMAGE_PHOTOM_NOCAL) {
	  image[i].flags &= ~ID_IMAGE_PHOTOM_NOCAL;
	  continue;
      }
      keep[Nkeep] = i;
      Nkeep ++;
  }

  /* apply calculated space-group offset to image Mcal values */
  for (i = 0; i < Nsgroup; i++) {
    for (j = 0; j < sgroup[i].Nimage; j++) {
      sgroup[i].image[j][0].McalPSF  -= sgroup[i].M;
      sgroup[i].image[j][0].McalAPER -= sgroup[i].M;
    }
  }

  // save the rows in the image table which were used in this analysis
  gfits_vtable_from_ftable (&db[0].ftable, &db[0].vtable, keep, Nkeep);

  // write image table
  dvo_image_update (db, VERBOSE);

  // XXX need to fix the update for the catalog (or make it optional)
  return;

  // XXX this process uses the existence of the file to perform the update
  // XXX convert this to an examination of the SkyTable
  /** update catalog tables **/
  pglob.gl_offs = 0;
  sprintf (line, "%s/*/*.cpt", CATDIR);
  glob (line, 0, NULL, &pglob);

  InitCoords (&coords, "DEC--TAN");
  
  for (i = 0; i < pglob.gl_pathc; i++) {
    dvo_catalog_init (&catalog, TRUE);
    catalog.filename = pglob.gl_pathv[i];
    catalog.catflags = DVO_LOAD_AVERAGE | DVO_LOAD_MEASURE | DVO_LOAD_MISSING | DVO_LOAD_SECFILT;
    catalog.Nsecfilt  = GetPhotcodeNsecfilt ();

    // an error exit status here is a significant error
    if (!dvo_catalog_open (&catalog, NULL, VERBOSE, "a")) {
      fprintf (stderr, "ERROR: failure to open/create catalog file %s\n", catalog.filename);
      exit (2);
    }
    if (!catalog.Naverage_disk) {
      dvo_catalog_unlock (&catalog);
      dvo_catalog_free (&catalog);
      continue;
    }

    gfits_scan (&catalog.header, "RA0", "%lf",  1, &Rmin);
    gfits_scan (&catalog.header, "RA1", "%lf",  1, &Rmax);
    gfits_scan (&catalog.header, "DEC0", "%lf", 1, &Dmin);
    gfits_scan (&catalog.header, "DEC1", "%lf", 1, &Dmax);

    Rmin = ohana_normalize_angle (Rmin);
    Rmax = ohana_normalize_angle (Rmax);

    coords.crval1 = 0.5*(Rmin + Rmax);
    coords.crval2 = 0.5*(Dmin + Dmax);

    Nmin = 0;
    Rmin = 1000;
    /* primitive version: match catalog with closest sgroup */
    for (j = 0; j < Nsgroup; j++) {
      if (!RD_to_XY (&x, &y, sgroup[j].v1, sgroup[j].v2, &coords)) continue;
      radius = hypot (x, y);
      if ((j == 0) || (radius < Rmin)) {
	Rmin = radius;
	Nmin = j;
      }
    }

    fprintf (stderr, "catalog: %s sgroup: %d %s %f\n", catalog.filename, Nmin, sgroup[Nmin].label, Rmin);
    update_catalog_uniphot (&catalog, &sgroup[Nmin], (Rmin > 2*RADIUS)); 
    SetProtect (TRUE);
    if (!dvo_catalog_save (&catalog, VERBOSE)) { fprintf (stderr, "ERROR: failed to save %s\n", catalog.filename); exit (1); }
    if (!dvo_catalog_unlock (&catalog)) { fprintf (stderr, "ERROR: failed to unlock %s\n", catalog.filename); exit (1); }
    SetProtect (FALSE);
    dvo_catalog_free (&catalog);
  }
}      

/* loop over all average 

   if (source equiv photcode)

   loop over sgroups

   if (radius < RADIUS) 
      
   average.M (+/-) source.M
*/

/* loop over all measure 

   if (source equiv photcode)

   loop over sgroups

   if (radius < RADIUS) 
      
   measure.Mcal (+/-) source.M
*/

/* alternative (slower, more robust)

   loop over all average
   loop over all average.Nm
   select measure with source equiv photcode
   match measure to image, get Mcal & offset
   if any offset for average[i] is different, give error
   apply offset to average.M
*/

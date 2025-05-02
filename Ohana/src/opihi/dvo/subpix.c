# include "dvoshell.h"

int subpix (int argc, char **argv) {
  
  off_t Nlo, Nhi, *entry, Nentry;
  off_t j, i, I, *index, Nstars, Nimage, Nmeasure;
  off_t Nmin, Nsub, NSUB;
  int status, TimeFormat;
  time_t Timage, TimeReference;
  double X, Y, Mabs, t;
  double Ra, Dec, Radius, Radius2, r, Rmin;
  double *RA, *DEC;
  
  SkyTable *sky;
  SkyList *skylist;
  Measure *measure;
  Image *image;
  Catalog catalog;

  if (!InitPhotcodes ()) return (FALSE);

  GetTimeFormat (&TimeReference, &TimeFormat);

  if (argc != 4) {
    gprint (GP_ERR, "USAGE: subpix ra dec radius\n");
    return (FALSE);
  }
  if (!ohana_str_to_radec (&Ra, &Dec, argv[1], argv[2])) return (FALSE);
  Ra = ohana_normalize_angle (Ra);

  Radius = atof (argv[3]);

  /* load star nearest position */
  sky = GetSkyTable ();
  skylist = SkyListByRadius (sky, -1, Ra, Dec, Radius);
  if (skylist[0].Nregions > 1) {
    gprint (GP_ERR, "warning, radius overlaps region boundary, not yet implemented\n");
  }

  /* lock, load, unlock catalog */
  dvo_catalog_init (&catalog, TRUE);
  catalog.filename = skylist[0].filename[0];
  catalog.catflags = DVO_LOAD_AVERAGE | DVO_LOAD_MEASURE;
  catalog.Nsecfilt = 0;

  // an error exit status here is a significant error
  if (!dvo_catalog_open (&catalog, NULL, FALSE, "r")) {
      fprintf (stderr, "ERROR: failure to open catalog file %s\n", catalog.filename);
      exit (2);
  }
  dvo_catalog_unlock (&catalog);

  /* quick search of star list for Ra, Dec */
  Nstars = catalog.Naverage;
  ALLOCATE (RA, double, Nstars);
  ALLOCATE (DEC, double, Nstars);
  ALLOCATE (index, off_t, Nstars);
  for (i = 0; i < Nstars; i++) {
    RA[i] = catalog.average[i].R;
    DEC[i] = catalog.average[i].D;
    index[i] = i;
  }
  if (Nstars > 1) sort_coords_index (DEC, RA, index, Nstars);

  /* bracket the DEC range of interest */
  Nlo = bracket (DEC, Nstars, FALSE, Dec - Radius);
  Nhi = bracket (DEC, Nstars, TRUE,  Dec + Radius);
  ALLOCATE (entry, off_t, MAX (Nhi - Nlo, 1));
  Nentry = 0;

  /* find the list of stars */
  Radius2 = Radius*Radius;
  for (i = Nlo; i < Nhi; i++) {
    r = SQ(Dec - DEC[i]) + SQ(Ra - RA[i]);
    if (r < Radius2) {
      entry[Nentry] = i;
      Nentry ++;
    }
  }
  if (!Nentry) {
    gprint (GP_ERR, "no stars found\n");
    free (RA);
    free (DEC);
    free (entry);
    free (index);
    dvo_catalog_free (&catalog);
    SkyListFree (skylist);
    return (TRUE);
  }

  /* find the closest star */
  Nmin = 0;
  Rmin = SQ(Dec - DEC[entry[0]]) + SQ(Ra - RA[entry[0]]);
  for (i = 1; i < Nentry; i++) {
    r = SQ(Dec - DEC[entry[i]]) + SQ(Ra - RA[entry[i]]);
    if (r < Rmin) {
      Rmin = r;
      Nmin = i;
    }
  }
  Nentry = index[entry[Nmin]];
  Ra = RA[entry[Nmin]];
  Dec = DEC[entry[Nmin]];
  gprint (GP_ERR, "finding subpix values for star @ %f %f\n", Ra, Dec);

  free (RA);
  free (DEC);
  free (entry);
  free (index);

  /* storage for the image references */
  Nsub = 0;
  NSUB = 100;
  ALLOCATE (index, off_t, NSUB);

  /* load all images, extract those touching Ra, Dec */
  if ((image = LoadImagesDVO (&Nimage)) == NULL) return (FALSE);
  // BuildChipMatch (image, Nimage);

  for (i = 0; i < Nimage; i++) {
    status = RD_to_XY (&X, &Y, Ra, Dec, &image[i].coords);
    if (!status || (X < 0) || (X > image[i].NX) || (Y < 0) || (Y > image[i].NY)) continue;
    index[Nsub] = i;
    Nsub ++;
    if (Nsub == NSUB - 1) {
      NSUB += 100;
      REALLOCATE (index, off_t, NSUB);
    }
  }

  /* only print the entries for existing measurements of this star */ 
  measure = &catalog.measure[catalog.average[Nentry].measureOffset];
  Nmeasure = catalog.average[Nentry].Nmeasure;
  for (i = 0; i < Nsub; i++) {
    I = index[i];
    Timage = image[I].tzero;
    for (j = 0; j < Nmeasure; j++) {
      if (measure[j].t == Timage) { 
	Mabs = PhotCat (&measure[j], MAG_CLASS_PSF);
	RD_to_XY (&X, &Y, Ra, Dec, &image[I].coords);
	t = TimeValue (measure[j].t, TimeReference, TimeFormat);
	gprint (GP_LOG, "%f %6.3f %7.2f %7.2f %5.3f\n", t, Mabs, X, Y, image[I].secz);
      } 
    }
  }

  dvo_catalog_free (&catalog);
  free (index);
  SkyListFree (skylist);

  FreeImagesDVO(image);
  return (TRUE);
}

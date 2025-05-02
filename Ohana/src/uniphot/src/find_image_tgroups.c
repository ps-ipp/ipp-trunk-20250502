# include "uniphot.h"

Group *find_image_tgroups (FITS_DB *db, ImageLink **Imlink, int *Ntgroup) {

  char *start, *stop;
  int j, Ngroup, NGROUP, Nentry, NENTRY;
  off_t i, Nimage, Ntime;
  unsigned int *time, *tmin, *tmax;
  Group *group;
  Image *image;
  ImageLink *imlink;

  image = gfits_table_get_Image (&db[0].ftable, &Nimage, &db[0].scaledValue, &db[0].nativeOrder);
  if (!image) {
      fprintf (stderr, "ERROR: failed to read images\n");
      exit (2);
  }

  ALLOCATE (imlink, ImageLink, Nimage);

  /* sort time list (use only valid images?) */
  ALLOCATE (time, unsigned int, Nimage);
  Ntime = 0;
  for (i = 0; i < Nimage; i++) {
    if (image[i].flags & ID_IMAGE_PHOTOM_NOCAL) continue;
    if (!strcmp(&image[i].coords.ctype[4], "-DIS")) continue;
    time[Ntime] = image[i].tzero;
    Ntime ++;
  }
  sort_time (time, Ntime);

  /* find groups with dt < TRANGE */
  Ngroup = 0;
  NGROUP = 100;
  ALLOCATE (tmin, unsigned int, NGROUP);
  ALLOCATE (tmax, unsigned int, NGROUP);
  tmin[Ngroup] = time[0];

  /* generate tgroups */
  for (i = 0; i < Ntime - 1; i++) {
    if (time[i+1] - time[i] < TRANGE) continue;
    
    tmax[Ngroup] = time[i];
    
    Ngroup ++;
    if (Ngroup == NGROUP) {
      NGROUP += 100;
      REALLOCATE (tmin, unsigned int, NGROUP);
      REALLOCATE (tmax, unsigned int, NGROUP);
    }
    tmin[Ngroup] = time[i + 1];
  }
  tmax[Ngroup] = time[Ntime - 1];
  Ngroup ++;
  ALLOCATE (group, Group, Ngroup);

  /* assign images to groups */
  for (i = 0; i < Ngroup; i++) {
    Nentry = 0;
    NENTRY = 100;
    ALLOCATE (group[i].image, Image *, NENTRY);
    ALLOCATE (group[i].imlink, ImageLink *, NENTRY);
    group[i].M = 0;
    group[i].dM = 0;

    start = ohana_sec_to_date (tmin[i]);
    stop = ohana_sec_to_date (tmax[i]);
    snprintf (group[i].label, 64, "%s - %s", start, stop);
    strcpy(group[i].tstart, start);
    strcpy(group[i].tstop, stop);
    free (start);
    free (stop);

    for (j = 0; j < Nimage; j++) {
      if (image[j].tzero < tmin[i]) continue;
      if (image[j].tzero > tmax[i]) continue;
      if (image[j].flags & ID_IMAGE_PHOTOM_NOCAL) continue;
      if (!strcmp(&image[j].coords.ctype[4], "-DIS")) continue;
      
      group[i].image[Nentry] = &image[j];
      group[i].imlink[Nentry] = &imlink[j];
      imlink[j].tgroup = &group[i];
      Nentry ++;
      if (Nentry == NENTRY) {
	NENTRY += 100;
	REALLOCATE (group[i].image, Image *, NENTRY);
	REALLOCATE (group[i].imlink, ImageLink *, NENTRY);
      }
    }
    group[i].Nimage = Nentry;
  }
  *Imlink = imlink;
  *Ntgroup = Ngroup;
  return (group);
}



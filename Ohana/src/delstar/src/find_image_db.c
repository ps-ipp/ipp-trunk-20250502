# include "delstar.h"

off_t *find_images_name (FITS_DB *db, char *filename, off_t *nlist) {

  off_t i, Nimage, Nlist, NLIST;
  off_t *list;
  char *p, *name;
  Image *image;

  /* strip off all but the filename */
  p = strrchr (filename, '/');
  if (p == NULL) {
    name = filename;
  } else {
    name = p + 1;
  }

  image = gfits_table_get_Image (&db[0].ftable, &Nimage, &db[0].scaledValue, &db[0].nativeOrder);
  if (!image) {
    fprintf (stderr, "ERROR: failed to read images\n");
    exit (2);
  }

  Nlist = 0;
  NLIST = 100;
  ALLOCATE (list, off_t, NLIST);

  for (i = 0; i < Nimage; i++) {
    if (strcmp (image[i].name, name)) continue;
    list[Nlist] = i;
    Nlist ++;
    CHECK_REALLOCATE (list, off_t, NLIST, Nlist, 100);
  }

  *nlist = Nlist;
  return (list);
}

/* find images in db by image data (time/photcode) */
off_t *find_images_data (FITS_DB *db, Image *timage, off_t *nlist) {

  off_t i, Nimage, Nlist, NLIST; 
  off_t *list;
  Image *image;
  time_t start, stop;
  int code;

  image = gfits_table_get_Image (&db[0].ftable, &Nimage, &db[0].scaledValue, &db[0].nativeOrder);

  start = timage[0].tzero - MAX(0.01*timage[0].trate*timage[0].NY, 1);
  stop  = timage[0].tzero + MAX(1.01*timage[0].trate*timage[0].NY, 1);
  code  = timage[0].photcode;

  Nlist = 0;
  NLIST = 100;
  ALLOCATE (list, off_t, NLIST);

  for (i = 0; i < Nimage; i++) {
    if (image[i].tzero < start) continue;
    if (image[i].tzero > stop) continue;
    if (image[i].photcode != code) continue;
    list[Nlist] = i;
    Nlist ++;
    CHECK_REALLOCATE (list, off_t, NLIST, Nlist, 100);
  }

  *nlist = Nlist;
  return (list);
}

/* find images in db by image data (time/photcode) */
off_t *find_images_time (FITS_DB *db, e_time start, e_time end, PhotCode *code, off_t *nlist) {

  off_t i, Nimage, Nlist, NLIST;
  off_t *list;
  Image *image;

  image = gfits_table_get_Image (&db[0].ftable, &Nimage, &db[0].scaledValue, &db[0].nativeOrder);

  Nlist = 0;
  NLIST = 100;
  ALLOCATE (list, off_t, NLIST);

  for (i = 0; i < Nimage; i++) {
    if (image[i].tzero < start) continue;
    if (image[i].tzero > end) continue;
    if (code != NULL) {
      if (image[i].photcode != code[0].code) continue;
    }
    list[Nlist] = i;
    Nlist ++;
    CHECK_REALLOCATE (list, off_t, NLIST, Nlist, 100);
  }

  *nlist = Nlist;
  return (list);
}


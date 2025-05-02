# include "imregister.h"
# include "imreg.h"

void ModifySubset (FITS_DB *db, RegImage *image, off_t Nimage, off_t *match, off_t Nmatch) {

  off_t i, j, Nold;
  char *tmppath;
  char *ext, *root, *path;

  Nold = 0;
  tmppath = NULL;

  /* create some necessary variables */
  if (output.modify_path) { 
    Nold = strlen (output.oldpath);
    ALLOCATE (tmppath, char, 128);
  }
  // XXX this code is not used, why was it here?
  // if (output.modify_dist) {
  //   dist = (output.dist) ? 0xff : ~IMREG_DIST;
  // }

  /* modify the selected entries */
  for (j = 0; j < Nmatch; j++) {

    i = match[j];

    if (output.modify_path) {
      if (!strncmp (image[i].pathname, output.oldpath, Nold)) {
	strcpy (tmppath, &image[i].pathname[Nold]);
	snprintf (image[i].pathname, 128, "%s%s", output.newpath, tmppath);
      }
    }

    if (output.mef2split) {
      if (image[i].mode == M_MEF) {
	root = filerootname (image[i].filename);
	ext = fileextname (image[i].filename);
	path = strcreate (image[i].pathname);

	snprintf (image[i].pathname, 128, "%s/%s", path, root);
	snprintf (image[i].filename, 64,  "%s%02d.%s", root, image[i].ccd, ext);
	image[i].mode = M_SPLIT;
	free (root);
	free (ext);
	free (path);
      }
    }

    if (output.split2mef) {
      if (image[i].mode == M_SPLIT) {
	ext  = fileextname (image[i].filename);
	root = filebasename (image[i].pathname);
	path = pathname (image[i].pathname);

	snprintf (image[i].pathname, 128, "%s", path);
	snprintf (image[i].filename, 64,  "%s.%s", root, ext);
	image[i].mode = M_MEF;
	free (root);
	free (ext);
	free (path);
      }
    }

    if (output.modify_dist) {
      if (output.dist)  image[i].flag |=  IMREG_DIST;
      if (!output.dist) image[i].flag &= ~IMREG_DIST;
    }

    if (output.modify_filter) {
      strncpy_nowarn (image[i].filter, output.filter, 31);
    }

    if (output.modify_type) {
      image[i].type = output.type;
    }

  }

  /** we may later want to pull this out and put it elsewhere **/
  gfits_vtable_from_ftable (&db[0].ftable, &db[0].vtable, match, Nmatch);
  for (i = 0; i < Nmatch; i++) {
    gfits_convert_RegImage ((RegImage *) db[0].vtable.buffer[i], sizeof (RegImage), 1);
  }
  gfits_db_update (db);
  gfits_db_close (db);
  gfits_db_free (db);

  fprintf (stderr, "SUCCESS\n");
  exit (0);

}


/* 
   MEF                      SPLIT
   /path/filename.fits <--> /path/filename/filenameNN.fits
*/


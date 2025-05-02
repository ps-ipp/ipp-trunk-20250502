# include "imregister.h"
# include "detrend.h"

int SetAltpath (FITS_DB *db, DetReg *image, off_t Nimage, Match *match, off_t Nmatch) {
  
  int status, found;
  off_t i, j, n, Nlist;
  off_t *list;
  char *dBPath, infile[512], outfile[512], line[1100];
  struct stat statbuf;

  ALLOCATE (list, off_t, Nimage);
  Nlist = 0;

  dBPath = get_dBPath ();

  /* set altpath for 'add' images */
  if (output.Altpath == ADD) {
    /* find images to add */
    for (i = 0; i < Nmatch; i++) {
      if (image[match[i].image].altpath == FALSE) {
	list[Nlist] = match[i].image;
	Nlist ++;
      }
    }
    /* copy the masters to the altpath locations */
    for (j = 0; j < Nlist; j++) {
      i = list[j];
      for (n = 0; n < NDetrendAltDB; n++) {
	snprintf (infile, 512, "%s/%s", dBPath, image[i].filename);
	snprintf (outfile, 512, "%s/%s", DetrendAltDB[n], image[i].filename);
	status = ckpathname (outfile);
	if (!status) {
	  fprintf (stderr, "warning: can't make outfile directory for %s\n", outfile);
	  continue;
	}
	snprintf (line, 1100, "cp %s %s", infile, outfile);
	fprintf (stderr, "%s\n", line);
	status = system (line);
	if (status) {
	  fprintf (stderr, "warning: can't make outfile directory for %s\n", outfile);
	  continue;
	}
      }
      image[i].altpath = TRUE;
    }

    /** we may later want to pull this out and put it elsewhere **/
    gfits_vtable_from_ftable (&db[0].ftable, &db[0].vtable, list, Nlist);
    for (i = 0; i < Nmatch; i++) {
      gfits_convert_DetReg ((DetReg *) db[0].vtable.buffer[i], sizeof (DetReg), 1);
    }
    gfits_db_update (db);
    gfits_db_close (db);
    gfits_db_free (db);

    fprintf (stderr, "SUCCESS\n");
    exit (0);
  }

  /* unset altpath for 'delete' images */
  if (output.Altpath == DELETE) {
    /* find images to delete */
    for (i = 0; i < Nmatch; i++) {
      if (image[match[i].image].altpath == TRUE) {
	list[Nlist] = match[i].image;
	Nlist ++;
      }
    }
    /* remove the copies from the altpath locations */
    for (j = 0; j < Nlist; j++) {
      i = list[j];
      image[i].altpath = FALSE;
      for (n = 0; n < NDetrendAltDB; n++) {
	snprintf (outfile, 512, "%s/%s", DetrendAltDB[n], image[i].filename);
	snprintf (line, 1100, "rm %s", outfile);
	fprintf (stderr, "%s\n", line);
	status = system (line);
	if (status) {
	  fprintf (stderr, "warning: can't delete %s\n", outfile);
	}
      }
    }

    /** we may later want to pull this out and put it elsewhere **/
    gfits_vtable_from_ftable (&db[0].ftable, &db[0].vtable, list, Nlist);
    for (i = 0; i < Nmatch; i++) {
      gfits_convert_DetReg ((DetReg *) db[0].vtable.buffer[i], sizeof (DetReg), 1);
    }
    gfits_db_update (db);
    gfits_db_close (db);
    gfits_db_free (db);

    fprintf (stderr, "SUCCESS\n");
    exit (0);
  }

  /* check for existence in altpath, set flag as appropriate */
  if (output.Altpath == UPDATE) {
    for (j = 0; j < Nmatch; j++) {
      i = match[j].image;
      list[Nlist] = i;
      Nlist ++;
      found = TRUE;
      for (n = 0; found && (n < NDetrendAltDB); n++) {
	snprintf (outfile, 512, "%s/%s", DetrendAltDB[n], image[i].filename);
	status = stat (outfile, &statbuf);
	fprintf (stderr, "checking for %s, status is %d\n", outfile, status);
	if (status == -1) found = FALSE;
      }
      image[i].altpath = found;
    }

    /** we may later want to pull this out and put it elsewhere **/
    gfits_vtable_from_ftable (&db[0].ftable, &db[0].vtable, list, Nlist);
    for (i = 0; i < Nmatch; i++) {
      gfits_convert_DetReg ((DetReg *) db[0].vtable.buffer[i], sizeof (DetReg), 1);
    }
    gfits_db_update (db);
    gfits_db_close (db);
    gfits_db_free (db);

    fprintf (stderr, "SUCCESS\n");
    exit (0);
  }

  fprintf (stderr, "unknown altpath mode: %d\n", output.Altpath);
  return (TRUE);
}


int ckpathname (char *newpath) {
  
  int status;
  char *path;
  struct stat statbuf;
  
  path = pathname (newpath);
  
  status = stat (path, &statbuf);
  if (status == -1) {
    if (errno == ENOENT) {
      if (mkdirhier (path, S_IRWXU | S_IRWXG | S_IRWXO) == -1) {
	fprintf (stderr, "ERROR: can't create path %s\n", path);
	return (FALSE);
      }
    } else {
      fprintf (stderr, "ERROR: problem with path %s\n", path);
      return (FALSE);
    }
  }
  free (path);
  return (TRUE);
}


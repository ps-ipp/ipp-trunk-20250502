# include "imregister.h"
# include "detrend.h"

DetReg DefineEntry (Descriptor descriptor) {
  
  struct timeval now;
  DetReg newdata;

  gettimeofday (&now, (void *) NULL);
  
  /* now we have all of the data (except filename), set the new entry */
  newdata.tstart    = descriptor.tstart;
  newdata.tstop     = descriptor.tstop;
  newdata.treg      = now.tv_sec;
  newdata.type      = descriptor.type;
  newdata.filter    = descriptor.filter;
  newdata.ccd       = descriptor.CCD;
  newdata.exptime   = descriptor.Exptime;
  newdata.Norder    = descriptor.order;
  newdata.mode      = descriptor.mode;
  newdata.altpath   = FALSE;
  bzero (newdata.dummy, 58);
  snprintf (newdata.label, 64, "%s", descriptor.label);
  
  return (newdata);
}

/* we are generating a name based on type, etc.  determine valid path */
int SaveEntry (char *input, DetReg *newdata, char *ID) {

  int status;
  int found, Ntry, filestate;
  char path[MY_MAX_PATH], fullpath[MY_MAX_PATH], filename[MY_MAX_PATH];
  char rootname[MY_MAX_PATH], fullname[MY_MAX_PATH], line[512];
  char *dBPath;
  char *filter_basename;
  struct stat statbuf;
  FILE *f;

  filter_basename = filterhash[newdata[0].filter];
  dBPath = get_dBPath ();

  if (newdata[0].type == T_FLAT) {
    snprintf_nowarn (path, MY_MAX_PATH, "%s/%s", get_type_name(newdata[0].type), filter_basename);
  } else {
    snprintf_nowarn (path, MY_MAX_PATH, "%s", get_type_name(newdata[0].type));
  }    
  snprintf_nowarn (fullpath, MY_MAX_PATH, "%s/%s", dBPath, path);

  status = stat (fullpath, &statbuf);
  if (status == -1) {
    if (errno == ENOENT) {
      if (mkdirhier (fullpath, S_IRWXU | S_IRWXG | S_IRWXO) == -1) {
	fprintf (stderr, "ERROR: can't create path %s\n", fullpath);
	exit (1);
      }
    } else {
      fprintf (stderr, "ERROR: problem with path %s\n", fullpath);
      exit (1);
    }
  }

  /* base filename constructed from detrend type information */
  switch (newdata[0].type) {
  case T_DARK:
  case T_BIAS:
  case T_MASK:
    snprintf_nowarn (rootname, MY_MAX_PATH, "%s.%s.%d.%02d", ID, get_type_name(newdata[0].type), (int)newdata[0].exptime, newdata[0].ccd);
    break;
  default:
    snprintf_nowarn (rootname, MY_MAX_PATH, "%s.%s.%s.%02d", ID, get_type_name(newdata[0].type), filter_basename, newdata[0].ccd);
  }

  /* find & lock first file of the format name that does not exist */
  found = FALSE;
  for (Ntry = 0; !found && (Ntry < 100); Ntry++) {
    snprintf_nowarn (filename, MY_MAX_PATH, "%s/%s.%02d.fits", path, rootname, Ntry);
    snprintf_nowarn (fullname, MY_MAX_PATH, "%s/%s", dBPath, filename);
    f = fsetlockfile (fullname, 2.0, LCK_XCLD, &filestate);
    /* if there is an error, it may just mean file is locked. */
    if (filestate == LCK_EMPTY) {
	found = TRUE;
	newdata[0].Nentry = Ntry;
    } 
  }
  if (!found) { 
    fprintf (stderr, "ERROR: no available target files for %s/%s?\n", path, rootname);
    exit (1);
  }
  strcpy (newdata[0].filename, filename);
    
  if (DEBUG) {
    fprintf (stderr, "path: %s\n", path);
    fprintf (stderr, "fullpath: %s\n", fullpath);
    fprintf (stderr, "filename: %s\n", filename);
    fprintf (stderr, "dBPath: %s\n", dBPath);
    fprintf (stderr, "line: %s\n", line);
  }

  /* copy the file to the new name, add entry to database */
  /* we need some error checking here - complain if filename > 255 */
  /* the copy replaces the locked file, lock remains valid */

  sprintf (line, "/bin/cp -f %s %s", input, fullname);
  status = system (line);
  if (status) {
    fprintf (stderr, "ERROR: failure in image copy %s to %s\n", input, fullname);
    exit (1);
  }
  fclearlockfile (fullname, f, LCK_XCLD, &filestate);

  return (TRUE);
}

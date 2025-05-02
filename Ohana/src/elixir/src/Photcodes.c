# include "elixir.h"

char *GetPhotcode (char *file) {

  char *code;
  Header header;
  char detector[80], filter[80];
  int i, imageid;

  /* read in image header, open image data region */
  if (!gfits_read_header (file, &header)) {
    fprintf (stderr, "ERROR: can't find image file %s (1)\n", file);
    exit(0);
  }

  /** WARNING: this should use the abstracted keyword names **/
  gfits_scan (&header, "DETECTOR", "%s", 1, detector);
  gfits_scan (&header, "FILTER",   "%s", 1, filter);
  gfits_scan (&header, "IMAGEID",  "%d", 1, &imageid);

  gfits_free_header (&header);

  for (i = 0; i < strlen(detector); i++) { detector[i] = toupper (detector[i]); }
  for (i = 0; i < strlen(filter); i++) { filter[i] = toupper (filter[i]); }

  ALLOCATE (code, char, 256);
  sprintf (code, "%s.%s.%02d", detector, filter, imageid);
  return (code);

}

/** WARNING: this is pretty weak **/
char *GetPhotcodeMef (char *file) {

  char *code;
  Header header;
  char detector[80], filter[80];
  int i, imageid;
  char *filename, *p;

  /* file contains file and extend number */
  ALLOCATE (filename, char, strlen(file) + 1);
  
  p = file;
  while ((p = strchr (p, ',')) != (char *) NULL) { *p = ' '; }
  sscanf (file, "%s %d", filename, &imageid);

  /* read in image header, open image data region */
  if (!gfits_read_header (filename, &header)) {
    fprintf (stderr, "ERROR: can't find image file %s (1)\n", file);
    exit(0);
  }
  free (filename);

  gfits_scan (&header, "DETECTOR", "%s", 1, detector);
  gfits_scan (&header, "FILTER",   "%s", 1, filter);
  gfits_free_header (&header);

  for (i = 0; i < strlen(detector); i++) { detector[i] = toupper (detector[i]); }
  for (i = 0; i < strlen(filter); i++) { filter[i] = toupper (filter[i]); }

  ALLOCATE (code, char, 256);  sprintf (code, "%s.%s.%02d", detector, filter, imageid);
  return (code);

}

char *BuildCode (char *line) {

  char *code, *p;
  Header header;
  char fullname[256], detector[80], filter[80];
  char name[256], path[256], ccd[80], mode[80];
  int i;

  p = line;
  while ((p = strchr (p, ',')) != (char *) NULL) { *p = ' '; }
  sscanf (line, "%s %s %s %s", path, name, ccd, mode);

  if (!strcasecmp (mode, "MEF")) {
    snprintf_nowarn (fullname, 256, "%s/%s.fits", path, name);
  } else {
    snprintf_nowarn (fullname, 256, "%s/%s/%s%s.fits", path, name, name, ccd);
  }

  if (!gfits_read_header (fullname, &header)) {
    fprintf (stderr, "ERROR: can't find image file %s (1)\n", fullname);
    exit(0);
  }
  gfits_scan (&header, "DETECTOR", "%s", 1, detector);
  gfits_scan (&header, "FILTER",   "%s", 1, filter);
  gfits_free_header (&header);
  
  for (i = 0; i < strlen(detector); i++) { detector[i] = toupper (detector[i]); }
  for (i = 0; i < strlen(filter); i++) { filter[i] = toupper (filter[i]); }

  ALLOCATE (code, char, 256);
  sprintf (code, "%s.%s.%s", detector, filter, ccd);
  return (code);

}

char *BuildName (char *line) {

  char *fullname;
  char name[256], path[256], ccd[80], mode[80];
  char *p;

  ALLOCATE (fullname, char, 256);

  p = line;
  while ((p = strchr (p, ',')) != (char *) NULL) { *p = ' '; }
  sscanf (line, "%s %s %s %s", path, name, ccd, mode);

  if (!strcasecmp (mode, "MEF")) {
    sprintf (fullname, "%s/%s.fits", path, name);
  } else {
    sprintf (fullname, "%s/%s/%s%s.fits", path, name, name, ccd);
  }

  return (fullname);

}

char *GetPhotcodeExt (char *file) {

  char *code;
  Header header;
  char detector[80], filter[80];
  int i, imageid, extend;
  char *filename, *p;

  /* file contains file and extend number */
  ALLOCATE (filename, char, strlen(file) + 1);
  
  p = file;
  while ((p = strchr (p, ',')) != (char *) NULL) { *p = ' '; }
  sscanf (file, "%s %d", filename, &extend);

  /* read in image header, open image data region */
  if (!gfits_read_Xheader (filename, &header, extend)) {
    fprintf (stderr, "ERROR: can't find image file %s (1)\n", file);
    exit(0);
  }
  free (filename);

  gfits_scan (&header, "DETECTOR", "%s", 1, detector);
  gfits_scan (&header, "FILTER",   "%s", 1, filter);
  gfits_scan (&header, "IMAGEID",  "%d", 1, &imageid);
  gfits_free_header (&header);

  for (i = 0; i < strlen(detector); i++) { detector[i] = toupper (detector[i]); }
  for (i = 0; i < strlen(filter); i++) { filter[i] = toupper (filter[i]); }

  ALLOCATE (code, char, 256);
  sprintf (code, "%s.%s.%02d", detector, filter, imageid);
  return (code);

}

/* given path/filename.ext return filename */
char *RootFilename (char *file) {

  int Nbyte;
  char *root, *p1, *p2;

  p1 = strrchr (file, '/');
  if (p1 == (char *) NULL) p1 = file;

  p2 = strrchr (file, '.');
  if (p2 == (char *) NULL) p2 = p1 + strlen(p1);
  Nbyte = p2-p1;

  ALLOCATE (root, char, Nbyte + 1);
  strncpy_nowarn (root, p1, Nbyte);
  
  return (root);

}  

/* given: path/filename
   return: path or ./ if none */
char *PathFilename (char *file) {

  int Nbyte;
  char *path, *p1;

  p1 = strrchr (file, '/');
  if (p1 == (char *) NULL) {
    ALLOCATE (p1, char, 2);
    strcpy (p1, ".");
    return (p1);
  }

  Nbyte = p1-file;
  ALLOCATE (path, char, Nbyte + 1);
  strncpy_nowarn (path, file, Nbyte);
  
  return (path);

}  

/* given: A/B/D/filename
   return: D or ./ if none 
   if /filename, return '/' 
   if filename, return ./
*/
char *BaseFilename (char *file) {

  int Nbyte;
  char *path, *p1, *p2;

  p1 = strrchr (file, '/');
  if (p1 == (char *) NULL) {
    ALLOCATE (p1, char, 2);
    strcpy (p1, ".");
    return (p1);
  }
  if (p1 == file) {
    ALLOCATE (p1, char, 2);
    strcpy (p1, "/");
    return (p1);
  }

  p2 = p1 - 1;
  while ((p2 > file) && (*p2 != '/')) p2--;
  if (p2 != file) {
    p2 ++;
  }

  Nbyte = (p1 - p2);
  ALLOCATE (path, char, Nbyte + 1);
  strncpy_nowarn (path, p2, Nbyte);
  
  return (path);

}  


/*

There are two methods to implement the Photcode determination for MEF files. 
The difference is in whether the IMAGEID is used to refer to the ccd, or the EXTNUM.
Originally, EXTNUM was used, which meant we needed to determine the IMAGEID for a given EXTNUM.
The old implementation of PhotCodeMef returned the photcode based on the input MEF filename
and the EXTNUM.  This is now maintained as PhotCodeExt, but not used in elixir.

As of 13/2/00, we've converted to only using the IMAGEID.  this means that PhotCodeMef returns 
the photcode, based on the MEF filename and the IMAGEID.  To go along with this change, the 
function flatten.mef operates on the given IMAGEID not EXTNUM as well.                             

*/



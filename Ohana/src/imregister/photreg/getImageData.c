# include "imregister.h"
# include "photreg.h"

void getImageData (char *Image, char *ImageCCD, char *ImageMode) {

  Header header;
  char detector[64], filter[64], photcode[64];
  int i, ccd, Nfilter;
  
  /* extract time & photcode from header */

  /* load options from the image header */
  if (!gfits_read_header (Image, &header)) {
    if (output.verbose) fprintf (stderr, "ERR: trouble reading image header\n");
    exit (1);
  }

  /* get time from image header */
  ALLOCATE (criteria.tstart, time_t, 1);
  ALLOCATE (criteria.tstart, time_t, 1);
  criteria.tstart[0] = parse_time (&header);
  criteria.tstop[0] = criteria.tstart[0] + 1;
  criteria.Ntimes = 1;

  /** determine photcode from header **/
  /* get camera */
  gfits_scan (&header, CameraKeyword, "%s", 1, detector);
  for (i = 0; i < strlen(detector); i++) { detector[i] = toupper (detector[i]); }
  for (i = 0; i < strlen(detector); i++) { if (isspace (detector[i])) detector[i] = '.'; }
    
  /* get filter */
  Nfilter = FILTER_NONE;
  gfits_scan (&header, FilterKeyword,   "%s", 1, filter);
  for (i = 0; i < strlen (filter); i++) { if (isspace (filter[i])) filter[i] = '.'; }
  for (i = 0; (i < NFILTER) && (Nfilter == FILTER_NONE); i++) {
    if (!strcasecmp (filter, filtername[i])) {
      Nfilter = filternum[i];
    }
  }      
  if (Nfilter == FILTER_NONE) {
    fprintf (stderr, "ERR: invalid filter %s\n", filter);
    exit (1);
  }
  strcpy (filter, filtername[Nfilter]);

  /* get ccd number */
  if (!strcasecmp (ImageCCD, "phu")) {
    ccd = 0;
  } else {
   if (!strcasecmp (ImageMode, "mef")) {
     ccd = MatchCCDName (ImageCCD);
   } else {
     ccd = -1;
     gfits_scan (&header, CCDnumKeyword,  "%d", 1, &ccd);
   }  
   if (ccd == -1) {
     fprintf (stderr, "ERR: invalid ccd %s %s\n", ImageCCD, ImageMode);
     exit (1);
   }
  }

  snprintf_nowarn (photcode, 64, "%s.%s.%02d", detector, filter, ccd);
  if (!(criteria.photcode = GetPhotcodeCodebyName (photcode))) {
    fprintf (stderr, "ERR: photcode not found table\n");
    exit (1);
  }
  criteria.PhotCodeSelect = TRUE;
  return;
}

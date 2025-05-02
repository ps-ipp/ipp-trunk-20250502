# include "imregister.h"
# include "spreg.h"

Spectrum *spinfo (char *filename) {

  Spectrum *spectrum;
  Header header;
  char line[80];
  struct timeval now;
  char *name, *cwd, *tempname;
  double tmp;

  ALLOCATE (spectrum, Spectrum, 1);
  bzero (spectrum, sizeof (Spectrum));

  /* load in FITS header from image */
  if (!gfits_read_header (filename, &header)) {
    fprintf (stderr, "ERROR: can't find image file %s\n", filename);
    exit (1);
  }

  /* get filename */
  name = filebasename (filename);
  snprintf (spectrum[0].filename, 32, "%s", name);
  free (name);

  /* get pathname (add cwd if not absolute) */
  name = pathname (filename);
  if (name[0] != '/') {
    cwd = getcwd (NULL, 64 - strlen(name) - 2);
    if (cwd == (char *) NULL) {
      cwd = strcreate ("longpath");
      /* pathname is limited to 64 chars.  if it is toolong, 
	 we put in the word 'longpath'.  no other useful solution. */
    }
    ALLOCATE (tempname, char, strlen (cwd) + strlen (name) + 2);
    if (!strcmp (name, ".")) {
      sprintf (tempname, "%s", cwd);
    } else {
      sprintf (tempname, "%s/%s", cwd, name);
    }      
    free (name);
    free (cwd);
    name = tempname;
  }    
  snprintf (spectrum[0].pathname, 64, "%s", name);
  free (name);

  /* find the important header keyword values */
  spectrum[0].obstime = parse_time (&header);

  gettimeofday (&now, (void *) NULL);
  spectrum[0].regtime = now.tv_sec;

  warn_scan_nchar (&header, 16, ObjectKeyword,    1, spectrum[0].objname);
  warn_scan_nchar (&header, 16, CameraKeyword,    1, spectrum[0].instrument);
  warn_scan_nchar (&header, 16, TelescopeKeyword, 1, spectrum[0].telescope);

  clean_spaces (spectrum[0].objname);
  clean_spaces (spectrum[0].instrument);
  clean_spaces (spectrum[0].telescope);

  warn_scan (&header, ExptimeKeyword,    "%f", 1, &spectrum[0].exptime);
  warn_scan (&header, AirmassKeyword,    "%f", 1, &spectrum[0].airmass);

  /* grab coordinates from header */
  if (RADecDegKeyword[0] & DECDecDegKeyword[0]) {
    /* expect RA & DEC in decimal degrees */
    warn_scan (&header, RADecDegKeyword,  "%f", 1, &spectrum[0].ra);
    warn_scan (&header, DECDecDegKeyword, "%f", 1, &spectrum[0].dec);
  } else {
    if (RASexigKeyword[0] & DECSexigKeyword[0]) {
      /* expect RA & DEC in hh:mm:ss, dd:mm:ss */
      warn_scan (&header, RASexigKeyword,  "%s", 1, line);
      ohana_dms_to_ddd (&tmp, line);
      spectrum[0].ra = 15*tmp;
      warn_scan (&header, DECSexigKeyword, "%s", 1, &line);
      ohana_dms_to_ddd (&tmp, line);
      spectrum[0].dec = tmp;
    }
  }

  /* for now we will not try to determine the mode, state, or 
     wavelength ranges from the header.  set these with -modify
     or on the command line on insert.  later on, we can add the logic */
  
  spectrum[0].mode  = SPMODE_UKN;
  spectrum[0].state = SPSTATE_UKN;
  spectrum[0].Nspec = 1;
  spectrum[0].flag  = 0;
  
  { 
    int Nx;
    float crpix, crval, cd11;

    warn_scan (&header, "CRPIX1", "%f", 1, &crpix);
    warn_scan (&header, "CRVAL1", "%f", 1, &crval);
    if (!gfits_scan (&header, "CDELT1", "%f", 1, &cd11))
      warn_scan (&header, "CD1_1", "%f", 1, &cd11);
    
    gfits_scan (&header, "NAXIS1", "%d", 1, &Nx);

    spectrum[0].Ws = (1  - crpix)*cd11 + crval;
    spectrum[0].We = (Nx - crpix)*cd11 + crval;
    spectrum[0].dW = cd11;
  }
  
  strcpy (spectrum[0].extname, "PHU");
  
  return (spectrum);

}


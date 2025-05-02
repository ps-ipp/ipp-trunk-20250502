# include "lightcurve.h"

void  get_info (images)
Image *images;
{

  char head[500], line[500];
  Header header;
  int i, status;
  double jd, days, hrs, min, sec, atof();

  status = TRUE;

  strcpy (head, images[0].name);
  strcpy (strchr(head, '.'), ".head");

  status = gfits_read_header (head, &header);
  if (!status) {
    fprintf (stderr, "could not open header file %s\n", head);
    exit (0);
  }
  status = TRUE;

  switch (PIXELS) {
  case 0:
    status &= gfits_scan (&header, "RA_O",    "%lf", 1, &images[0].RA_O);
    status &= gfits_scan (&header, "RA_X",    "%lf", 1, &images[0].RA_X);
    status &= gfits_scan (&header, "RA_Y",    "%lf", 1, &images[0].RA_Y);
    status &= gfits_scan (&header, "DEC_O",   "%lf", 1, &images[0].DEC_O);
    status &= gfits_scan (&header, "DEC_X",   "%lf", 1, &images[0].DEC_X);
    status &= gfits_scan (&header, "DEC_Y",   "%lf", 1, &images[0].DEC_Y);
    COS = cos (RAD_DEG * images[0].DEC_O);
    break;

  case 1:
    status &= gfits_scan (&header, "X_O",    "%lf", 1, &images[0].RA_O);
    status &= gfits_scan (&header, "X_X",    "%lf", 1, &images[0].RA_X);
    status &= gfits_scan (&header, "X_Y",    "%lf", 1, &images[0].RA_Y);
    status &= gfits_scan (&header, "Y_O",    "%lf", 1, &images[0].DEC_O);
    status &= gfits_scan (&header, "Y_X",    "%lf", 1, &images[0].DEC_X);
    status &= gfits_scan (&header, "Y_Y",    "%lf", 1, &images[0].DEC_Y);
    COS = 1;
    break;
  }
  
  gfits_scan (&header, "ORIGIN", "%s", 1, line);
  /* interpret the silly way ESO / La Palma stores exposure time and duration   */
  if (!strcmp (line, "ESO-MIDAS")) { 
    status &= gfits_scan (&header, "DATE-OBS", "%s", 1, line);
    stripwhite (line);
    fprintf (stderr, "date line: %s\n", line);
    line[2] = 0;
    jd = atof(line);
    line[0] = 0;
    status = gfits_scan (&header, "TM-START", "%lf", 1, &sec);
    fprintf (stderr, "date: %f, sec: %f\n", jd, sec);
    jd += (sec/86400.0) + 0.5;
    images[0].JD =  jd;
    gfits_scan (&header, "EXPTIME", "%lf", 1, &images[0].exptime);
  }
  else {
    status &= gfits_scan (&header, "JD", "%lf", 1, &images[0].JD);
    images[0].JD -= 2400000.5;   /* convert to MJD */
    gfits_scan (&header, "EXPTIME", "%lf", 1, &images[0].exptime);
  }

  status &= gfits_scan (&header, "Mcal", "%lf", 1, &images[0].Mcal);
  status &= gfits_scan (&header, "McalR", "%lf", 1, &images[0].McalR);
  status &= gfits_scan (&header, "McalD", "%lf", 1, &images[0].McalD);
  status &= gfits_scan (&header, "McalR2", "%lf", 1, &images[0].McalR2);
  status &= gfits_scan (&header, "McalD2", "%lf", 1, &images[0].McalD2);
  status &= gfits_scan (&header, "McalRD", "%lf", 1, &images[0].McalRD);
  status &= gfits_scan (&header, "dMcal", "%lf", 1, &images[0].dMcal);

  images[0].airmass = 1000;  
  gfits_scan (&header, "SECZ", "%lf", 1, &images[0].airmass);
  gfits_scan (&header, "AIRMASS", "%lf", 1, &images[0].airmass);
  /* a stupid value as a flag (i hate flags!) but gfits_scan will not alter the
     value if it fails to find the entry.  try a couple possibilities */
 
  fprintf (stderr, "%s: %10.6f %10.6f  %lf  %6.3f %8.2f %5.2f\n", 
	   head, images[0].RA_O, images[0].DEC_O, images[0].JD, 
	   images[0].Mcal, images[0].exptime, images[0].airmass);
   
  if (!status) {
    fprintf (stderr, "error getting header info from %s\n", head);
    exit(0);
  }

  if (images[0].airmass > 10) {
    images[0].airmass = 1.1;
    fprintf (stderr, "warning: no airmass info\n");
  }
  
  images[0].Nstars  = 0;
  images[0].fixed   = TRUE;
  images[0].empty   = FALSE;
  images[0].Mtime   = 2.5*log10(images[0].exptime);
  images[0].clouds  = 0.0;

  gfits_free_header (&header);
  
}



/* airmass formula (only used if needed) */
/* page 264 of Kitchin */

/*    z = 1 / ( .5294258 * sin(pi * d / 180.0) + .84835625 * cos(pi * d / 180.0) *
 cos(pi * h / 12.0));

n
p status

*/


/*
   code for using RA, DEC, ST info, if needed. 
    status &= gfits_scan (&header, "ST", "%lf", 1, &LST);
    ra  =  info[0].RA_O  + info[0].RA_X *CCD_X/2.0 + info[0].RA_Y *CCD_Y/2.0;
    dec =  info[0].DEC_O + info[0].DEC_X*CCD_X/2.0 + info[0].DEC_Y*CCD_Y/2.0;
    temp1 = sin(OBS_LAT*DEG_RAD)*sin(dec*DEG_RAD);
    temp2 = cos(OBS_LAT*DEG_RAD)*cos(dec*DEG_RAD);
    temp3 = cos(((360./24.)*LST - ra)*DEG_RAD);
    (info[0].airmass) = 1.0 / (temp1 + temp2*temp3);
*/

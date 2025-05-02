# include "delstar.h"

e_time parse_time (Header *header) {

  double jd;
  int Ny, Nf, mode;
  int Nsec, hour, min, sec, year, month, day;
  char *py, *pm, *pd, *c;
  char line[256];

  /* we want to find JD or MJD to get Nsec (seconds since 01/01/1970) */

  /* try JD first */
  if (strcasecmp (JDKeyword, "NONE")) {
    uppercase (JDKeyword);
    gfits_scan (header, JDKeyword, "%lf", 1, &jd);
    Nsec = (jd - 2440587.5)*86400;
    return (Nsec);
  }

  /* try MJD next */
  if (strcasecmp (MJDKeyword, "NONE")) {
    uppercase (MJDKeyword);
    gfits_scan (header, MJDKeyword, "%lf", 1, &jd);
    Nsec = (jd - 40587.0)*86400;
    return (Nsec);
  }
    
  /* get UT and DATE */
  uppercase (UTKeyword);
  gfits_scan (header, UTKeyword, "%s", 1, line);
  /* remove ':' characters */
  for (c = strchr (line, 0x3a); c != NULL; c = strchr (line, 0x3a)) { *c = ' '; }
  sscanf (line, "%d %d %d", &hour, &min, &sec);

  /* parse mode line */
  uppercase (DateMode);
  for (Ny = 0, c = strchr (DateMode, 'Y'); c != NULL; c = strchr (c + 1, 'Y'), Ny++);
  if ((Ny != 2) && (Ny != 4)) {
    Shutdown ("error in DATE-MODE format: %s", DateMode);
  }
  py = strchr (DateMode, 'Y');
  pm = strchr (DateMode, 'M');
  pd = strchr (DateMode, 'D');
  if ((py == NULL) || (pm == NULL) || (pd == NULL)) {
    Shutdown ("error in DATE-MODE format: %s", DateMode);
  }
  if ((py > pm) && (py < pd)) {
    Shutdown ("error in DATE-MODE format: %s", DateMode);
  }
  if ((py > pd) && (py < pm)) {
    Shutdown ("error in DATE-MODE format: %s", DateMode);
  }
  mode = 0;
  if ((py < pm) && (pm < pd)) { mode = 1; }  /* yyyy-mm-dd */
  if ((py < pm) && (pm > pd)) { mode = 2; }  /* yyyy-dd-mm */
  if ((py > pm) && (pm < pd)) { mode = 3; }  /* mm-dd-yyyy */
  if ((py > pm) && (pm > pd)) { mode = 4; }  /* dd-mm-yyyy */
  if (!mode) {
    Shutdown ("error in DATE-MODE format: %s", DateMode);
  }

  /* parse date entry */
  uppercase (DateKeyword);
  gfits_scan (header, DateKeyword, "%s",  1, line);
  /* remove possible separators: ':', '/' '.', '-' */
  for (c = strchr (line, 0x3a); c != NULL; c = strchr (line, 0x3a)) { *c = ' '; }
  for (c = strchr (line, 0x2f); c != NULL; c = strchr (line, 0x2f)) { *c = ' '; }
  for (c = strchr (line, 0x2e); c != NULL; c = strchr (line, 0x2e)) { *c = ' '; }
  for (c = strchr (line, 0x2d); c != NULL; c = strchr (line, 0x2d)) { *c = ' '; }

  Nf = 0;
  switch (mode) {
  case 1:
    Nf = sscanf (line, "%d %d %d", &year, &month, &day);
    break;
  case 2:
    Nf = sscanf (line, "%d %d %d", &year, &day, &month);
    break;
  case 3:
    Nf = sscanf (line, "%d %d %d", &month, &day, &year);
    break;
  case 4:
    Nf = sscanf (line, "%d %d %d", &day, &month, &year);
    break;
  }
  if (Nf != 3) {
    Shutdown ("error in date entry (%s) or DATE-MODE format (%s)", line, DateMode);
  }

  if (year > 1000) {
    if (Ny == 2) {
      fprintf (stderr, "warning: mode line claims 2 digit year, but 4 digit year found\n");
    }
  } else {
    if (Ny == 4) {
      fprintf (stderr, "warning: mode line claims 4 digit year, but 2 digit year found\n");
    }
    if (year < 50) year += 100;
    year += 1900;
  }    

  /* convert yy.mm.dd hh.mm.ss to Nsec since 1970 (jd = 2440587.5) */
  /* note that in this section, tm_mon has range 1-12, unlike for gmtime () */
  jd = day - 32075 + (int)(1461*(year + 4800 + (int)(((month)-14)/12))/4)
    + (int)(367*((month) - 2 - (int)(((month) - 14)/12)*12)/12)
    - (int)(3*(int)((year + 4900 + (int)(((month) - 14)/12))/100)/4) - 0.5;
  /* jd is the julian day of the whole day only not the time */
  Nsec = (jd - 2440587.5)*86400 + 3600.0*hour + min*60.0 + sec;
  
  return (Nsec);

}

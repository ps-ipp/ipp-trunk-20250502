# include "dvoshell.h"
# include "hstgsc.h"

/* find region file which contains ra, dec */
void aregion (GSCRegion *region, FILE *f, double ra, double dec, char *path) {
  
  char buffer[28800], temp[50], file[50];
  double RA0, RA1, DEC0, DEC1;
  int i, NBigDec, NLINES, done;
  
  ra = ohana_normalize_angle (ra);

  if (dec >= 86.25) {
    sprintf (file, "%s/n8230/pole.cpt", path);
    region[0].DEC[0] = 86.25;
    region[0].DEC[1] = 93.75;
    region[0].RA[0] =  0.0;
    region[0].RA[1] =  360.0;
    strcpy (region[0].filename, file);
    return;
  }
    
  NBigDec = -1;
  for (i = 0; i < 12; i++) {
    if ((dec >= BigDecBounds[i]) && (dec < BigDecBounds[i+1])) {
      NBigDec = i;
      break;
    }
  }
  if (NBigDec < 0) {
    for (i = 13; i < 24; i++) {
      if ((dec < BigDecBounds[i]) && (dec >= BigDecBounds[i+1])) {
	NBigDec = i;
	break;
      }
    }
  }
  if (NBigDec < 0) {
    gprint (GP_ERR, "dec out of range: %f\n", dec);
  }
    
  NLINES = 0;
  for (i = 0; i < NBigDec; i++) {
    NLINES += NDecLines[i];
  }
  fseeko (f, 5*2880 + 48*NLINES, SEEK_SET);
      
  done = FALSE;
  fread (buffer, 1, 48*NDecLines[NBigDec], f);
  for (i = 0; !done && (i < NDecLines[NBigDec]); i++) {
    strncpy_nowarn (temp, &buffer[i*48], 48);
    hstgsc_hms_to_deg (&RA0, &RA1, &DEC0, &DEC1, &temp[7]);
    if (RA1 < RA0) RA1 += 360.0;
    if ((dec >= 0) && (dec >= DEC0) && (dec < DEC1) && (ra >= RA0) && (ra < RA1)) {
      done = TRUE;
    }
    if ((dec < 0) && (dec < DEC0) && (dec >= DEC1) && (ra >= RA0) && (ra < RA1)) {
      done = TRUE;
    }
  }

  if (!done) {
    gprint (GP_ERR, "error in search: %f %f\n", ra, dec);
    exit (0);
  }
  temp[5] = 0;
  sprintf (file, "%s/%s/%s.cpt", path, Dec2Sections[NBigDec],&temp[1]);
  if (DEC0 < DEC1) {
    region[0].DEC[0] = DEC0;
    region[0].DEC[1] = DEC1;
  } else {
    region[0].DEC[0] = DEC1;
    region[0].DEC[1] = DEC0;
  }     
  region[0].RA[0] = RA0;
  region[0].RA[1] = RA1;
  strcpy (region[0].filename, file);
  return;
}

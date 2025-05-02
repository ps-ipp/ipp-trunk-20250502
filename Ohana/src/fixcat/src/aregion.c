# include "markstar.h"

double BigDecBounds[] = {0.0, 7.5, 15.0, 22.5, 30.0, 37.5, 45.0, 
			 52.5, 60.0, 67.5, 75.0, 82.5, 90.0,
			 0.0, -7.5, -15.0, -22.5, -30.0, -37.5, -45.0, 
			 -52.5, -60.0, -67.5, -75.0, -82.5, -90.0};
char *DecSections[] = {"N0000", "N0730", "N1500", "N2230", "N3000", "N3730", "N4500", 
		       "N5230", "N6000", "N6730", "N7500", "N8230", "weirdness", 
		       "S0000", "S0730", "S1500", "S2230", "S3000", "S3730", "S4500", 
		       "S5230", "S6000", "S6730", "S7500", "S8230", "weirdness"};

char *Dec2Sections[] = {"n0000", "n0730", "n1500", "n2230", "n3000", "n3730", "n4500", 
			"n5230", "n6000", "n6730", "n7500", "n8230", "weirdness", 
			"s0000", "s0730", "s1500", "s2230", "s3000", "s3730", "s4500", 
			"s5230", "s6000", "s6730", "s7500", "s8230", "weirdness"};

char *disk[] = {"disk 1", "disk 1", "disk 1", "disk 1", "disk 1", "disk 1", "disk 1", 
		"disk 1", "disk 1", "disk 1", "disk 1", "disk 1", "weirdness", 
		"disk 1", "disk 2", "disk 2", "disk 2", "disk 2", "disk 2", "disk 2", 
		"disk 2", "disk 2", "disk 2", "disk 2", "disk 2", "weirdness"};

int NBigRASections [] = {48, 47, 45, 43, 40, 36, 32, 28, 21, 15, 9, 3, 3, 48, 47, 45, 43, 40, 36, 32, 28, 21, 15, 9, 3, 3};

int NDecLines[] = {593, 584, 551, 530, 522, 465, 406, 362, 280, 198, 123, 24, 
                   0, 597, 578, 574, 577, 534, 499, 442, 376, 294, 212, 144, 48};

/* find region file which contains ra, dec */
aregion (region, f, ra, dec) 
GSCRegion region[];
FILE *f;
double ra, dec;
{
  
  
  char buffer[28800], temp[50], file[50];
  double dr, dd;
  double RA0, RA1, DEC0, DEC1;
  int i, NBigDec, NLINES, done, nregion;
  
  ra = ohana_normalize_angle (ra);
    
  if (dec >= 86.25) {
    sprintf (file, "%s/n8230/pole.cpt\0", GSCDIR);
    region[0].DEC[0] = 86.25;
    region[0].DEC[1] = 93.75;
    region[0].RA[0] = -180.0;
    region[0].RA[1] =  540.0;
    strcpy (region[0].filename, file);
    return;
  }
    
  NBigDec = -1;
  for (i = 0; i < 12; i++) {
# ifdef DEBUG
    fprintf (stderr, "%d %f %f %f\n", i, dec, BigDecBounds[i], BigDecBounds[i+1]);
# endif
    if ((dec >= BigDecBounds[i]) && (dec < BigDecBounds[i+1])) {
      NBigDec = i;
      break;
    }
  }
  if (NBigDec < 0) {
    for (i = 13; i < 24; i++) {
# ifdef DEBUG
      fprintf (stderr, "%d %f %f %f\n", i, dec, BigDecBounds[i], BigDecBounds[i+1]);
# endif
      if ((dec < BigDecBounds[i]) && (dec >= BigDecBounds[i+1])) {
	NBigDec = i;
	break;
      }
    }
  }
  if (NBigDec < 0) {
    fprintf (stderr, "dec out of range: %f\n", dec);
  }
    
  NLINES = 0;
  for (i = 0; i < NBigDec; i++) {
    NLINES += NDecLines[i];
  }
  fseeko (f, 5*2880 + 48*NLINES, SEEK_SET);
      
  done = FALSE;
  Fread (buffer, 1, 48*NDecLines[NBigDec], f, "char");
  for (i = 0; !done && (i < NDecLines[NBigDec]); i++) {
    strncpy_nowarn (temp, &buffer[i*48], 48);
    hstgsc_hms_to_deg (&RA0, &RA1, &DEC0, &DEC1, &temp[7]);
    if (RA1 < RA0) RA1 += 360.0;
# ifdef DEBUG
    fprintf (stderr, "%f %f %f  %f %f %f  %s\n", DEC0, dec, DEC1, RA0, ra, RA1, temp);
# endif
    if ((dec >= 0) && (dec >= DEC0) && (dec < DEC1) && (ra >= RA0) && (ra < RA1)) {
      done = TRUE;
    }
    if ((dec < 0) && (dec < DEC0) && (dec >= DEC1) && (ra >= RA0) && (ra < RA1)) {
      done = TRUE;
    }
  }
  if (!done) {
    fprintf (stderr, "error in search: %f %f\n", ra, dec);
    exit (0);
  }
  temp[5] = 0;
  sprintf (file, "%s/%s/%s.cpt\0", GSCDIR, Dec2Sections[NBigDec],&temp[1]);
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

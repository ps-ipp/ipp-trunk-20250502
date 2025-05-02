# include "dvoshell.h"
# include "hstgsc.h"

/* returns a list of region files within the desired RA, DEC region */
RegionFile *find_regions (double Ra, double Dec, double radius, int *Nregions) {
  
  char filename[256];
  char buffer[28800], temp[50];
  RegionFile *regions;
  FILE *f;
  double minRa, maxRa, minDec, maxDec, rad;

  double RA0, RA1, DEC0, DEC1;
  int i, j, NBigDec;
  int done, NREGIONS, nregion;
  off_t NLINES;
  
  VarConfig ("GSCFILE", "%s", filename);
  f = fopen (filename, "r");
  if (f == NULL) {
    gprint (GP_ERR, "ERROR: can't find regions file %s\n", filename);
    *Nregions = 0;
    return ((RegionFile *) NULL);
  }
  
  NREGIONS = 50;
  ALLOCATE (regions, RegionFile, NREGIONS);
  nregion = 0;

  Ra = ohana_normalize_angle (Ra);

  minDec = Dec - radius;
  maxDec = Dec + radius;

  if ((minDec <= -90) || (maxDec >= 90)) {
    minRa = 0;
    maxRa = 360;
  } else {
    rad = MAX (radius / (cos(minDec*RAD_DEG)), radius / (cos(maxDec*RAD_DEG)));
    minRa = Ra - rad;
    maxRa = Ra + rad;
  }
  
  /* use the pole regions, if near pole */
  if (maxDec > 86.25) {
    sprintf (regions[nregion].name, "n8230/pole.cpt");
    regions[nregion].RA0 = 0;
    regions[nregion].RA1 = 360;
    regions[nregion].DEC0 = 86.25;
    regions[nregion].DEC1 = 90.0;
    nregion ++;
    if (nregion == NREGIONS) {
      NREGIONS += 50;
      REALLOCATE (regions, RegionFile, NREGIONS);
    }
  }

  if (minDec > 86.25) {
    return (regions);
  }
    
  if ((minDec < 0) && (maxDec > 0)) {
    /* Search Both Sides */
    NBigDec = 0;
  } else {
    /* find large DEC region (directory) */
    NBigDec = -1;
    for (i = 0; i < 12; i++) {
      if ((minDec >= BigDecBounds[i]) && (minDec < BigDecBounds[i+1])) {
	NBigDec = i;
	break;
      }
    }
    if (NBigDec < 0) {
      for (i = 13; i < 24; i++) {
	if ((maxDec < BigDecBounds[i]) && (maxDec >= BigDecBounds[i+1])) {
	  NBigDec = i;
	  break;
	}
      }
    }
  }
  if (NBigDec < 0) {
    gprint (GP_ERR, "ERROR: Dec out of range: %f\n", minDec);
    *Nregions = 0;
    return ((RegionFile *) NULL);
  }
  
  /* count lines before section */
  NLINES = 0;
  for (i = 0; i < NBigDec; i++) {
    NLINES += NDecLines[i];
  }
  fseeko (f, 5*2880 + 48*NLINES, SEEK_SET);
  
  /* should be in this section.  if not, there is a problem counting... */
  /* careful with the 0,360.0 boundary **/
  done = FALSE;
  for (j = 0; !done && (NBigDec + j < 25); j++) {
    fread (buffer, 48*NDecLines[NBigDec + j], 1, f);
    for (i = 0; (i < NDecLines[NBigDec + j]); i++) {
      strncpy_nowarn (temp, &buffer[i*48], 48);
      hstgsc_hms_to_deg (&RA0, &RA1, &DEC0, &DEC1, &temp[7]);
      if (RA1 < RA0) RA1 += 360.0;
      if ((DEC1 > 0) && (minDec < DEC1) && (maxDec > DEC0) && (minRa < RA1) && (maxRa > RA0)) {
	temp[5] = 0;
	sprintf (regions[nregion].name, "%s/%s.cpt", Dec2Sections[NBigDec + j], &temp[1]);
	regions[nregion].RA0 = RA0;
	regions[nregion].RA1 = RA1;
	regions[nregion].DEC0 = DEC0;
	regions[nregion].DEC1 = DEC1;
	nregion ++;
	if (nregion == NREGIONS) {
	  NREGIONS += 50;
	  REALLOCATE (regions, RegionFile, NREGIONS);
	}
      }
      if ((DEC1 < 0) && (minDec < DEC0) && (maxDec > DEC1) && (minRa < RA1) && (maxRa > RA0)) {
	temp[5] = 0;
	sprintf (regions[nregion].name, "%s/%s.cpt", Dec2Sections[NBigDec + j], &temp[1]);
	regions[nregion].RA0 = RA0;
	regions[nregion].RA1 = RA1;
	regions[nregion].DEC0 = DEC0;
	regions[nregion].DEC1 = DEC1;
	nregion ++;
	if (nregion == NREGIONS) {
	  NREGIONS += 50;
	  REALLOCATE (regions, RegionFile, NREGIONS);
	}
      }
      if (((DEC1 > 0) && (maxDec <= DEC1)) || ((DEC1 < 0) && (minDec >= DEC1))) {
	done = TRUE;
      }
    }
    if (done && (minDec < 0) && (maxDec > 0) && (BigDecBounds[NBigDec + j + 1] > 0)) {
      /* skip remaining north sections, try south sections */
      /* count lines before section */
      NLINES = 0;
      for (i = 0; i < 13; i++) { 
	NLINES += NDecLines[i];
      }
      fseeko (f, 5*2880 + 48*NLINES, SEEK_SET);
      done = FALSE;
      j = 12;
    }
  }

  REALLOCATE (regions, RegionFile, MAX(1,nregion));
  *Nregions = nregion;
  
  fclose (f);
  return (regions);
  
}


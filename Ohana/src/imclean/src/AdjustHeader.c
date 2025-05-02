# include "imclean.h"

void AdjustHeader (Header *header) {

  int i;
  double value;
  char line[256];

  if (FIX_KEYWORD) {
    for (i = 0; i < FIX_KEYWORD; i++) {
      if (!strcmp (KEYFMT[i], "%f")) {
	value = atof (KEYVALU[i]);
	gfits_modify (header, KEYWORD[i], "%le", 1, value);
      } else {
	gfits_modify (header, KEYWORD[i], "%s", 1, KEYVALU[i]);
      }
    }
  }

  /* validating the photcode name should be the job of DVO/addstar */
  /* here we are only writing the selected photcode name to the header */
  if (NEWPHOTCODE) {
    gfits_modify (header, "PHOTCODE", "%s", 1, PHOTCODE);
  }    

  if (!HEADER_COORDS) {
    int rh, rm, dd, dm;
    float rs, ds;
    RA = RA / 15.0;
    rh = RA; /* rh is int */
    rm = 60.0 * (RA - rh);
    rs = 3600 * (RA - rh - rm / 60.0);
    sprintf (line, "%02d:%02d:%05.2f", rh, rm, rs);
    gfits_modify (header, "RA", "%s", 1, line);
    dd = DEC;
    dm = 60.0 * (DEC - dd);
    ds = 3600 * (DEC - dd - dm / 60.0);
    sprintf (line, "%02d:%02d:%05.2f", dd, dm, ds);
    gfits_modify (header, "DEC", "%s", 1, line);
  }
 
  if (PROVIDE_ASTROM) {

    Header astrom_header;
    Coords coords;

    if (!gfits_read_header (AstromFile, &astrom_header)) {
      fprintf (stderr, "ERROR: can't get astrometry from %s\n", AstromFile);
      exit (1);
    }
    if (!GetCoords (&coords, &astrom_header)) {
      fprintf (stderr, "ERROR: no astrometric solution in header\n");
      exit (1);
    }
    /*** use PutCoords to update header ***/
    if (coords.Npolyterms > 1) {
      gfits_modify (header, "CTYPE1",   "%s",  1, "RA---PLY");
      gfits_modify (header, "CTYPE2",   "%s",  1, "DEC--PLY");
    } else {
      gfits_modify (header, "CTYPE1",   "%s",  1, "RA---TAN");
      gfits_modify (header, "CTYPE2",   "%s",  1, "DEC--TAN");
    }    
    gfits_modify (header, "NASTRO",   "%d", 1, 1); 

    gfits_modify (header, "CDELT1",   "%le", 1, coords.cdelt1); 
    gfits_modify (header, "CDELT2",   "%le", 1, coords.cdelt2);
    gfits_modify (header, "CRVAL1",   "%lf", 1, coords.crval1);
    gfits_modify (header, "CRVAL2",   "%lf", 1, coords.crval2);  
    gfits_modify (header, "CRPIX1",   "%lf", 1, coords.crpix1);
    gfits_modify (header, "CRPIX2",   "%lf", 1, coords.crpix2);
    gfits_modify (header, "PC001001", "%le", 1, coords.pc1_1);
    gfits_modify (header, "PC001002", "%le", 1, coords.pc1_2);
    gfits_modify (header, "PC002001", "%le", 1, coords.pc2_1);
    gfits_modify (header, "PC002002", "%le", 1, coords.pc2_2);
    gfits_modify (header, "NPLYTERM", "%d", 1, coords.Npolyterms);
    if (coords.Npolyterms > 1) {
      /* RA Terms */
      gfits_modify (header, "PCA1X2Y0", "%le", 1, coords.polyterms[0][0]);   /* polyterms[0]); */
      gfits_modify (header, "PCA1X1Y1", "%le", 1, coords.polyterms[1][0]);   /* polyterms[1]); */
      gfits_modify (header, "PCA1X0Y2", "%le", 1, coords.polyterms[2][0]);   /* polyterms[2]); */
      
      if (coords.Npolyterms > 2) {
	gfits_modify (header, "PCA1X3Y0", "%le", 1, coords.polyterms[3][0]);   /* polyterms[3]); */
	gfits_modify (header, "PCA1X2Y1", "%le", 1, coords.polyterms[4][0]);   /* polyterms[4]); */
	gfits_modify (header, "PCA1X1Y2", "%le", 1, coords.polyterms[5][0]);   /* polyterms[5]); */
	gfits_modify (header, "PCA1X0Y3", "%le", 1, coords.polyterms[6][0]);   /* polyterms[6]); */
      }
      /* Dec Terms */
      gfits_modify (header, "PCA2X2Y0", "%le", 1, coords.polyterms[0][1]);   /* polyterms[7]); */
      gfits_modify (header, "PCA2X1Y1", "%le", 1, coords.polyterms[1][1]);   /* polyterms[8]); */
      gfits_modify (header, "PCA2X0Y2", "%le", 1, coords.polyterms[2][1]);   /* polyterms[9]); */
      
      if (coords.Npolyterms > 2) {
	gfits_modify (header, "PCA2X3Y0", "%le", 1, coords.polyterms[3][1]);   /* polyterms[10]); */
	gfits_modify (header, "PCA2X2Y1", "%le", 1, coords.polyterms[4][1]);   /* polyterms[11]); */
	gfits_modify (header, "PCA2X1Y2", "%le", 1, coords.polyterms[5][1]);   /* polyterms[12]); */
	gfits_modify (header, "PCA2X0Y3", "%le", 1, coords.polyterms[6][1]);   /* polyterms[13]); */
      }
    }
  }
}

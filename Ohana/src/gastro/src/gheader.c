# include "gastro.h"

void gheader (char *file, Coords coords, double dR, int Nmatch) {

  Header header;
  FILE *f, *g;
  off_t i, oldsize, nbytes, status;
  char line[1024];

  if (!gfits_read_header (file, &header)) {
    fprintf (stderr, "ERROR: can't find image file %s (3)\n", file);
    exit(0);
  }
  oldsize = header.datasize;

  /* validating the photcode name should be the job of DVO/addstar */
  /* here we are only writing the selected photcode name to the header */
  if (NEWPHOTCODE) {
    gfits_modify (&header, "PHOTCODE", "%s", 1, PHOTCODE);
  }    
  
  if (Nmatch < 2) {
    gfits_modify (&header, "NASTRO", "%d", 1, 0);
    gfits_modify_alt (&header, "NASTRO", "%C", 1, "number of stars used for astrometry");
    goto skipstuff;
  }
  
  gfits_modify (&header, "NASTRO", "%d", 1, Nmatch);
  gfits_modify_alt (&header, "NASTRO", "%C", 1, "number of stars used for astrometry");

  /*** use PutCoords to update header ***/
  if (coords.Npolyterms > 1) {
    gfits_modify (&header, "CTYPE1",   "%s",  1, "RA---PLY");
    gfits_modify (&header, "CTYPE2",   "%s",  1, "DEC--PLY");
  } else {
    gfits_modify (&header, "CTYPE1",   "%s",  1, "RA---TAN");
    gfits_modify (&header, "CTYPE2",   "%s",  1, "DEC--TAN");
  }    
  gfits_modify (&header, "CDELT1",   "%le", 1, coords.cdelt1); 
  gfits_modify (&header, "CDELT2",   "%le", 1, coords.cdelt2);
  gfits_modify (&header, "CRVAL1",   "%lf", 1, coords.crval1);
  gfits_modify (&header, "CRVAL2",   "%lf", 1, coords.crval2);  
  gfits_modify (&header, "CRPIX1",   "%lf", 1, coords.crpix1);
  gfits_modify (&header, "CRPIX2",   "%lf", 1, coords.crpix2);
  gfits_modify (&header, "PC001001", "%le", 1, coords.pc1_1);
  gfits_modify (&header, "PC001002", "%le", 1, coords.pc1_2);
  gfits_modify (&header, "PC002001", "%le", 1, coords.pc2_1);
  gfits_modify (&header, "PC002002", "%le", 1, coords.pc2_2);
  gfits_modify (&header, "NPLYTERM", "%d", 1, coords.Npolyterms);
  if (coords.Npolyterms > 1) {
    /* RA Terms */
    gfits_modify (&header, "PCA1X2Y0", "%le", 1, coords.polyterms[0][0]);   /* polyterms[0]); */
    gfits_modify (&header, "PCA1X1Y1", "%le", 1, coords.polyterms[1][0]);   /* polyterms[1]); */
    gfits_modify (&header, "PCA1X0Y2", "%le", 1, coords.polyterms[2][0]);   /* polyterms[2]); */

    if (coords.Npolyterms > 2) {
      gfits_modify (&header, "PCA1X3Y0", "%le", 1, coords.polyterms[3][0]);   /* polyterms[3]); */
      gfits_modify (&header, "PCA1X2Y1", "%le", 1, coords.polyterms[4][0]);   /* polyterms[4]); */
      gfits_modify (&header, "PCA1X1Y2", "%le", 1, coords.polyterms[5][0]);   /* polyterms[5]); */
      gfits_modify (&header, "PCA1X0Y3", "%le", 1, coords.polyterms[6][0]);   /* polyterms[6]); */
    }
    /* Dec Terms */
    gfits_modify (&header, "PCA2X2Y0", "%le", 1, coords.polyterms[0][1]);   /* polyterms[7]); */
    gfits_modify (&header, "PCA2X1Y1", "%le", 1, coords.polyterms[1][1]);   /* polyterms[8]); */
    gfits_modify (&header, "PCA2X0Y2", "%le", 1, coords.polyterms[2][1]);   /* polyterms[9]); */

    if (coords.Npolyterms > 2) {
      gfits_modify (&header, "PCA2X3Y0", "%le", 1, coords.polyterms[3][1]);   /* polyterms[10]); */
      gfits_modify (&header, "PCA2X2Y1", "%le", 1, coords.polyterms[4][1]);   /* polyterms[11]); */
      gfits_modify (&header, "PCA2X1Y2", "%le", 1, coords.polyterms[5][1]);   /* polyterms[12]); */
      gfits_modify (&header, "PCA2X0Y3", "%le", 1, coords.polyterms[6][1]);   /* polyterms[13]); */
    }
  }
  gfits_modify (&header, "CERROR", "%lf", 1, dR);
  gfits_modify_alt (&header, "CERROR", "%C", 1, "scatter in astrometry soln (arcsec)");
  gfits_modify (&header, "CPRECISE", "%lf", 1, dR / sqrt(1.0*Nmatch));
  gfits_modify_alt (&header, "CPRECISE", "%C", 1, "precision of astrometry soln (arcsec)");
  gfits_modify (&header, "EQUINOX", "%lf", 1, 2000.0);
  /* we force equinox to be 2000.0 for all images */

skipstuff:
  if (header.datasize > oldsize) {
    if (VERBOSE) fprintf (stderr, "header expanded, creating new copy\n");
    sprintf (line, "mv %s %s~", file, file);
    status = system (line);
    if (status) {
      fprintf (stderr, "ERROR: unable to create %s~, exiting\n", file);
      exit (0);
    }
    sprintf (line, "%s~", file);
    f = fopen (line, "r");
    g = fopen (file, "w");
    if (f == NULL) {
      fprintf (stderr, "ERROR: can't find image file %s (4)\n", line);
      exit(0);
    }
    if (g == NULL) {
      fprintf (stderr, "ERROR: can't open output image file %s (4)\n", file);
      exit(0);
    }
    nbytes = fwrite (header.buffer, 1, header.datasize, g);
    fseeko (f, oldsize, SEEK_SET);
    for (i = 0; (nbytes = fread (header.buffer, 1, header.datasize, f)) > 0; i++) {
      if (nbytes != fwrite (header.buffer, 1, nbytes, g)) {
	fprintf (stderr, "ERROR: failure writing output data file\n");
	exit (0);
      }
    }
    fclose (f);
    fclose (g);
  } else {
    f = fopen (file, "r+");
    if (f == NULL) {
      fprintf (stderr, "ERROR: can't find image file %s (4)\n", file);
      exit(0);
    }
    
    fseeko (f, 0, SEEK_SET);
    nbytes = fwrite (header.buffer, 1, header.datasize, f);
    
    fclose (f);
  }
  free (header.buffer);

}



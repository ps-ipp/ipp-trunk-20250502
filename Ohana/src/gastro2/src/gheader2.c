# include "gastro2.h"

void gheader (char *file, CmpCatalog *Target) {

  double dR;
  Header header;
  FILE *f, *g;
  off_t i, oldsize, nbytes, status;
  char line[1024];

  if (!gfits_read_header (file, &header)) {
    fprintf (stderr, "ERROR: can't find image file %s (3)\n", file);
    exit(0);
  }
  oldsize = header.datasize;

  /* check for insufficient number of stars */
  switch (Target[0].coords.Npolyterms) {
    case 0:
    case 1:
      if (Target[0].answer.N < 6) {
	gfits_modify (&header, "NASTRO", "%d", 1, 0);
	gfits_modify_alt (&header, "NASTRO", "%C", 1, "number of stars used for astrometry");
	goto skipstuff;
      }
      break;
    case 2:
      if (Target[0].answer.N < 12) {
	gfits_modify (&header, "NASTRO", "%d", 1, 0);
	gfits_modify_alt (&header, "NASTRO", "%C", 1, "number of stars used for astrometry");
	goto skipstuff;
      }
      break;
    case 3:
      if (Target[0].answer.N < 20) {
	gfits_modify (&header, "NASTRO", "%d", 1, 0);
	gfits_modify_alt (&header, "NASTRO", "%C", 1, "number of stars used for astrometry");
	goto skipstuff;
      }
      break;
    default:
      fprintf (stderr, "invalid order\n");
      exit (2);
  }
  
  gfits_modify (&header, "NASTRO", "%d", 1, Target[0].answer.N);
  gfits_modify_alt (&header, "NASTRO", "%C", 1, "number of stars used for astrometry");

  /*** use PutCoords to update header ***/
  PutCoords (&Target[0].coords, &header);

  dR = fabs (Target[0].answer.dR*Target[0].coords.cdelt1*3600.0);
  gfits_modify (&header, "CERROR", "%lf", 1, dR);
  gfits_modify_alt (&header, "CERROR", "%C", 1, "scatter in astrometry soln (arcsec)");
  gfits_modify (&header, "CPRECISE", "%lf", 1, dR / sqrt(1.0*Target[0].answer.N));
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



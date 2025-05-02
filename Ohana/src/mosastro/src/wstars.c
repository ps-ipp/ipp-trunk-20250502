# include "mosastro.h"
# define NCHAR 66 /* 65 char EXCLUDING return */

void wchip (char *filename, Chip *data) {
  
  FILE *g;

  if (data[0].FITS) {
    wfits (filename, (SMPData *) data[0].buffer, data[0].Nstars, &data[0].header);
  } else {

    g = fopen (filename, "w");
    if (g == (FILE *) NULL) {
      fprintf (stderr, "ERROR: can't create output file %s\n", filename);
      exit (1);
    }

    fwrite (data[0].header.buffer, 1, data[0].header.datasize, g);
    fwrite (data[0].buffer, 1, data[0].Nbuffer, g);
    fclose (g);
  }  
}

void wstars (char *filename, SMPData *stars, int Nstars, Header *header) {
  
  int i, Nchar;
  FILE *g;
  char line[NCHAR + 3];

  g = fopen (filename, "w");
  if (g == (FILE *) NULL) {
    fprintf (stderr, "ERROR: can't create output file %s\n", filename);
    exit (1);
  }

  fwrite (header[0].buffer, 1, header[0].datasize, g);

  for (i = 0; i < Nstars; i++) {
    if (stars[i].dophot == 0) continue;
    Nchar = snprintf (line, NCHAR, "%6.1f %6.1f %6.3f %03d %2d %3.1f %6.3f %6.3f %6.2f %6.2f %5.1f", 
		      stars[i].X, stars[i].Y, stars[i].M, 
		      (int)(1000*stars[i].dM), stars[i].dophot, stars[i].sky, 
		      stars[i].Mgal, stars[i].Map, stars[i].fx, stars[i].fy, stars[i].df);
    
    /* this is just a little funny.  NCHAR (in) includes the trailing NULL, Nchar (out) excludes it */
    if (Nchar != NCHAR - 1) {
      fprintf (stderr, "funny line %d (%d)\n%s\n", i, Nchar, line);
    } else {
      fprintf (g, "%s\n", line);
    }
  }

  fclose (g);

}

/*

  63.6 2869.5 17.568 157 17.568 17.568 25.01 25.00 360.0 7 2.9

  63.6 2869.5 17.568 157 7 2.9

*/

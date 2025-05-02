# include "mosastro.h"

void testcoords () {

  int i;
  FILE *f;
  double R, D, P, Q, L, M, X, Y;
  
  fprintf (stderr, "starting test.dat\n");

  /* version 1: transform with three separate stages */
  /* X,Y -> L,M -> P,Q -> R,D */
  f = fopen ("test.1.dat", "w");
  if (f == (FILE *) NULL) {
    fprintf (stderr, "ERROR: can't create output file \n");
    exit (1);
  }
  for (i = 0; i < chip[0].Nmatch; i++) {
    X = chip[0].raw[i].X;
    Y = chip[0].raw[i].Y;
    XY_to_RD (&L, &M, X, Y, &chip[0].map);
    XY_to_RD (&P, &Q, L, M, &field.distort);
    XY_to_RD (&R, &D, P, Q, &field.project);
    fprintf (f, "%4d  %10.6f %10.6f  %8.2f %8.2f %8.2f %8.2f  %8.2f %8.2f\n", 
	     i, R, D, P, Q, L, M, X, Y);
  }
  fclose (f);

  /* version 2: transform with two separate stages */
  /* X,Y -> L,M -> R,D */
  f = fopen ("test.2.dat", "w");
  if (f == (FILE *) NULL) {
    fprintf (stderr, "ERROR: can't create output file \n");
    exit (1);
  }
  field_combine ();
  for (i = 0; i < chip[0].Nmatch; i++) {
    X = chip[0].raw[i].X;
    Y = chip[0].raw[i].Y;
    XY_to_RD (&L, &M, X, Y, &chip[0].map);
    XY_to_RD (&R, &D, L, M, &field.project);
    fprintf (f, "%4d  %10.6f %10.6f   %8.2f %8.2f  %8.2f %8.2f\n", 
	     i, R, D, L, M, X, Y);
  }
  fclose (f);


  /* version 3: transform with automatic two stages in coordops */
  /* X,Y -> R,D */
  f = fopen ("test.3.dat", "w");
  if (f == (FILE *) NULL) {
    fprintf (stderr, "ERROR: can't create output file \n");
    exit (1);
  }

  chip[0].map.mosaic = &field.project;
  strcpy (chip[0].map.ctype, "DEC--WRP");
  for (i = 0; i < chip[0].Nmatch; i++) {
    X = chip[0].raw[i].X;
    Y = chip[0].raw[i].Y;
    XY_to_RD (&R, &D, X, Y, &chip[0].map);
    fprintf (f, "%4d  %10.6f %10.6f   %8.2f %8.2f\n", 
	     i, R, D, X, Y);
  }
  fclose (f);
  exit (1);
}

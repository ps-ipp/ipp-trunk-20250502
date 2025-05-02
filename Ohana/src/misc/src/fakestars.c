# include <ohana.h>
# define NCHAR 66 /* 65 char EXCLUDING return */
# define D_NSTARS 1000
# define BYTES_STAR 66
# define BLOCK 1000

typedef struct {
  double X;
  double Y;
  double R;
  double D;
  double M, dM;
  char   dophot;
  double sky;
  double fx, fy, df;
  double Mgal, Map;
  int found;
} Stars;

  /* 
     load in a list of stars in cmp format
     generate a new list of stars in cmp format
     new measurements should have the requested noise characteristics */

int main (int argc, char **argv) {

  int Nstar, N, Nbytes, nbytes, i;
  FILE *f;
  char *input, *output, *buffer, line[NCHAR];
  Header header;
  Stars *stars;
  double tmp, dMs, dMr, dMo, dM, offset;

  if (argc != 5) {
    fprintf (stderr, "USAGE: fakestars (input) (output) (syserr) (offset)\n");
    exit (2);
  }
  input  = argv[1];
  output = argv[2];
  dMs    = atof (argv[3]);
  offset = atof (argv[4]);

  ohana_gaussdev_init ();

  /* load header, open file */
  if (!gfits_read_header (input, &header)) {
    fprintf (stderr, "ERROR: can't read header for %s\n", input);
    exit (1);
  }
  f = fopen (input, "r");
  if (f == NULL) {
    fprintf (stderr, "ERROR: can't read data from %s\n", input);
    exit (1);
  }
  fseeko (f, header.datasize, SEEK_SET); 

  /* find expected number of stars */
  gfits_scan (&header, "NSTARS", "%d", 1, &Nstar);
  if (Nstar == 0) {
    fprintf (stderr, "ERROR: can't get NSTARS from header\n");
    exit (1);
  }
  ALLOCATE (stars, Stars, Nstar);

  /* load in stars */
  Nbytes = Nstar*BYTES_STAR;
  ALLOCATE (buffer, char, Nbytes + 1);
  nbytes = fread (buffer, 1, Nbytes, f);
  if (nbytes != Nbytes) {
    fprintf (stderr, "ERROR: failed to read in %d stars\n", Nstar);
    exit (1);
  }
  buffer[Nbytes] = 0;

  for (i = 0; i < Nstar; i++) {
    Nbytes = i*BYTES_STAR;
    dparse (&stars[i].X,    1, &buffer[Nbytes]);
    dparse (&stars[i].Y,    2, &buffer[Nbytes]);
    dparse (&stars[i].M,    3, &buffer[Nbytes]);
    dparse (&stars[i].dM,   4, &buffer[Nbytes]);
    dparse (&stars[i].Mgal, 7, &buffer[Nbytes]);
    dparse (&stars[i].Map,  8, &buffer[Nbytes]);
    dparse (&stars[i].fx,   9, &buffer[Nbytes]);
    dparse (&stars[i].fy,  10, &buffer[Nbytes]);
    dparse (&stars[i].df,  11, &buffer[Nbytes]);
    
    dparse (&tmp,           5, &buffer[Nbytes]);
    stars[i].dophot = tmp;
  }
  fclose (f);

  /* error per star is hypot of systematic & poisson */
  for (i = 0; i < Nstar; i++) {
    dMr = 0.001 * stars[i].dM;
    dMo = hypot (dMs, dMr);
    dM  = ohana_gaussdev_rnd (0.0, dMo);
    stars[i].M += dM + offset;
  }

  gfits_write_header (output, &header);
  f = fopen (output, "a");
  if (f == NULL) {
    fprintf (stderr, "ERROR: can't open file for output: %s\n", output);
    exit (1);
  }
  fseeko (f, header.datasize, SEEK_SET); 

  for (i = 0; i < Nstar; i++) {
    snprintf (line, NCHAR, "%6.1f %6.1f %6.3f %03d %2d %3.1f %6.3f %6.3f %6.2f %6.2f %5.1f", 
	      stars[i].X, stars[i].Y, stars[i].M, 
	      (int)stars[i].dM, stars[i].dophot, stars[i].sky, 
	      stars[i].Mgal, stars[i].Map, stars[i].fx, stars[i].fy, stars[i].df);
    fprintf (f, "%s\n", line);
  }
  fclose (f);
}


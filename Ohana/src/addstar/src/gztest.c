# include "ohana.h"
# include "zlib.h"
# define DTIME(A,B) ((A.tv_sec - B.tv_sec) + 1e-6*(A.tv_usec - B.tv_usec))

main (int argc, char **argv) {

  off_t Nseek, Nback;
  int N, Nread;
  char *filename, *buffer;
  gzFile gf;
  FILE *f;
  struct timeval start, stop;

  if (argc != 6) {
    fprintf (stderr, "USAGE: gztest (mode) (file) (Nseek) (Nread) (Nback)\n");
    exit (2);
  }

  filename = argv[2];
  Nseek = atoi (argv[3]);
  Nread = atoi (argv[4]);
  Nback = -1 * atoi (argv[5]);
  ALLOCATE (buffer, char, Nread);

  if (!strcmp (argv[1], "gz")) {
    gf = gzopen (filename, "rb");
    if (gf == NULL) {
      fprintf (stderr, "can't read data file: %s", filename);
      exit (1);
    }

    gettimeofday (&start, NULL);
    N = gzseek (gf, Nseek, SEEK_SET);
    gettimeofday (&stop, NULL);
    if (N != Nseek) {
      fprintf (stderr, "error seeking\n");
      exit (1);
    }
    fprintf (stdout, "seek: %f\n", DTIME (stop, start));

    gettimeofday (&start, NULL);
    N = gzread (gf, buffer, Nread);
    if (N != Nread) {
      fprintf (stderr, "error reading\n");
      exit (1);
    }
    gettimeofday (&stop, NULL);
    fprintf (stdout, "read: %f\n", DTIME (stop, start));

    gettimeofday (&start, NULL);
    N = gzseek (gf, Nback, SEEK_CUR);
    gettimeofday (&stop, NULL);
    if (N == -1) {
      fprintf (stderr, "error seeking\n");
      exit (1);
    }
    fprintf (stdout, "back: %f\n", DTIME (stop, start));
    exit (0);
  } 

  if (!strcmp (argv[1], "raw")) {
    f = fopen (filename, "r");
    if (f == NULL) {
      fprintf (stderr, "can't read data file: %s", filename);
      exit (1);
    }

    gettimeofday (&start, NULL);
    N = fseeko (f, Nseek, SEEK_SET);
    gettimeofday (&stop, NULL);
    if (N) {
      fprintf (stderr, "error seeking\n");
      exit (1);
    }
    fprintf (stdout, "seek: %f\n", DTIME (stop, start));

    gettimeofday (&start, NULL);
    N = fread (buffer, 1, Nread, f);
    gettimeofday (&stop, NULL);
    if (N != Nread) {
      fprintf (stderr, "error reading\n");
      exit (1);
    }
    fprintf (stdout, "read: %f\n", DTIME (stop, start));

    gettimeofday (&start, NULL);
    N = fseeko (f, Nback, SEEK_CUR);
    gettimeofday (&stop, NULL);
    if (N) {
      fprintf (stderr, "error seeking\n");
      exit (1);
    }
    fprintf (stdout, "back: %f\n", DTIME (stop, start));
    exit (0);
  }
    
  
  fprintf (stderr, "unknown mode\n");
  exit (1);
}

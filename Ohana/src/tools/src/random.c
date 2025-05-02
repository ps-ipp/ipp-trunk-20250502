# include <ohana.h>
# define NBUFFER 8000000

int main (int argc, char **argv) {

  char c;
  int i, j, N;

  int FAST = FALSE;
  if ((N = get_argument (argc, argv, "-fast"))) {
    remove_argument (N, &argc, argv);
    FAST = TRUE;
  }

  if (argc != 2) {
    fprintf (stderr, "USAGE: random (Nbytes) [-fast]\n");
    fprintf (stderr, "  generates Nbytes of random characters, to stdout\n");
    fprintf (stderr, "  [-fast generates non random bytes, but ~2x faster]\n");
    exit (1);
  }

  long A = time(NULL);
  srand48(A);

  int Nbytes = atoi (argv[1]);
  char buffer[NBUFFER];

  int Nblock = Nbytes / NBUFFER;
  int Nextra = Nbytes - (Nblock * NBUFFER);

  for (j = 0; j < Nblock; j++) {
    for (i = 0; i < NBUFFER; i++) {
      if (i % 80 == 0) {
	buffer[i] = '\n';
	continue;
      }
      if (FAST) {
	c = (i % 90) + 33;
      } else {
	c = (char) 90*drand48() + 33;
      }
      buffer[i] = c;
    }
    fwrite (buffer, 1, NBUFFER, stdout);
  }
  for (i = 0; i < Nextra - 1; i++) {
    if (i % 80 == 0) {
      buffer[i] = '\n';
      continue;
    }
    if (FAST) {
      c = (i % 90) + 33;
    } else {
      c = (char) 90*drand48() + 33;
    }
    buffer[i] = c;
  }
  buffer[i] = '\n';
  fwrite (buffer, 1, Nextra, stdout);
  exit (0);
}



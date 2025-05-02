# include <ohana.h>

void main (argc, argv)
int argc;
char **argv;
{

  Header in_h, out_h;
  Matrix in_m, out_m;
  int i, j;
  int Nage, Nalpha, Ndist, NAv;
  float *in, *out;

  if (argc < 7) {
    fprintf (stderr, "USAGE fakedump infile outfile Nage Nalpha Ndist NAv\n");
    exit (0);
  }
  Nage = atof(argv[3]);
  Nalpha = atof(argv[4]);
  Ndist = atof(argv[5]);
  NAv = atof(argv[6]);

  gfits_read_header (argv[1], &in_h);
  gfits_read_matrix (argv[1], &in_m);

  if (Nage > in_h.Naxis[2] - 1) {
    fprintf (stderr, "Nage too big (%d, %d)\n", Nage, in_h.Naxis[2] - 1);
    exit (0);
  }

  if (Nalpha > in_h.Naxis[3] - 1) {
    fprintf (stderr, "Nalpha too big (%d, %d)\n", Nalpha, in_h.Naxis[3] - 1);
    exit (0);
  }

  if (Ndist > in_h.Naxis[4] - 1) {
    fprintf (stderr, "Ndist too big (%d, %d)\n", Ndist, in_h.Naxis[4] - 1);
    exit (0);
  }

  if (NAv > in_h.Naxis[5] - 1) {
    fprintf (stderr, "NAv too big (%d, %d)\n", NAv, in_h.Naxis[5] - 1);
    exit (0);
  }

  gfits_init_header (&out_h);
  out_h.bitpix = -32;
  out_h.Naxes = 2;
  out_h.Naxis[0] = in_h.Naxis[0];
  out_h.Naxis[1] = in_h.Naxis[1] - 1;
  out_h.bzero = 0.0;
  out_h.bscale = 1.0;
  out_h.unsign = FALSE;
  out_h.extend = FALSE;
  gfits_create_header (&out_h);
  gfits_create_matrix (&out_h, &out_m);
  
  in = (float *)in_m.buffer;
  out = (float *)out_m.buffer;
  for (i = 0; i < out_h.Naxis[0]; i++) {
    for (j = 0; j < out_h.Naxis[1]; j++) {
      out[i + out_h.Naxis[0]*j] = in[i + in_h.Naxis[0]*(j + in_h.Naxis[1]*(Nage + in_h.Naxis[2]*(Nalpha + in_h.Naxis[3]*(Ndist + in_h.Naxis[4]*(NAv + in_h.Naxis[5] * 0)))))];
      fprintf (stderr, "%f ", out[i + out_h.Naxis[0]*j]);
    }
    fprintf (stderr, "\n");
  }
  
  gfits_write_header (argv[2], &out_h);
  gfits_write_matrix (argv[2], &out_m);
}

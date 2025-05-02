# include <ohana.h>
# include <gfitsio.h>

int main (int argc, char **argv) {

  char *p;
  int i, N;
  Matrix matrix;
  Header header;

  if (argc != 3) {
    fprintf (stderr, "USAGE: fiximg (input) (output)\n");
    exit (1);
  }

  /* read catalog header */
  if (!gfits_read_header (argv[1], &header)) {
    fprintf (stderr, "error loading file %s\n", argv[1]);
    exit (1);
  }
  if (!gfits_read_matrix (argv[1], &matrix)) {
    fprintf (stderr, "error loading file %s\n", argv[1]);
    exit (1);
  }

  N = 0;
  while (gfits_header_field (&header, "COMMENT", 1) != (char *) NULL) {
    N++;
    gfits_delete (&header, "COMMENT", 1);
  }
  fprintf (stderr, "deleted %d comments\n", N);
  p = gfits_header_field (&header, "END", 1);
  for (i = 3; i < header.datasize - (int) (p - header.buffer); i++) { 
    p[i] = ' '; 
  }

  /* read catalog header */
  if (!gfits_write_header (argv[2], &header)) {
    fprintf (stderr, "error writing file %s\n", argv[2]);
    exit (1);
  }
  if (!gfits_write_matrix (argv[2], &matrix)) {
    fprintf (stderr, "error writing file %s\n", argv[2]);
    exit (1);
  }
  exit (0);

}

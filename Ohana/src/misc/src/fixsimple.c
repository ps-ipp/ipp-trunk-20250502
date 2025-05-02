# include <ohana.h>

main (int argc, char **argv) {

  char *p;
  int i, N;
  Matrix matrix;
  Header header;

  if (argc != 3) {
    fprintf (stderr, "USAGE: fixsimple (input) (output)\n");
    exit (1);
  }

  /* read header */
  if (!gfits_read_header (argv[1], &header)) {
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

  f = fopen (argv[1], "r+");
  if (f == NULL) {
    fprintf (stderr, "can't open file for update %s\n", argv[1]);
    exit (1);
  }

  fseeko (f, skip, SEEK_SET);
  status = fwrite (header.buffer, 1, header.datasize, f);
  if (status != header.datasize) {
    fprintf (stderr, "failed to write data to image header\n");
    exit (1);
  }
  if (fclose (f)) {
    fprintf (stderr, "error writing data to disk\n");
    exit (1);
  }

}

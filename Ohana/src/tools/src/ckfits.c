# include <ohana.h>
# include <gfitsio.h>

int main (int argc, char **argv) {

  off_t i, Nbytes, nbytes, Ndata, Ntotal, nskip;
  int status;
  Header header;
  FILE *f;
  char *buffer;

  if (argc != 2) {
    fprintf (stdout, "USAGE: ckfits (filename)\n");
    exit (2);
  }

  status = gfits_read_header (argv[1], &header);
  if (!status) { 
    fprintf (stdout, "%s: header\n", argv[1]);
    exit (1);
  }

  Ntotal = gfits_data_size (&header);

  Ndata = abs(header.bitpix / 8);
  for (i = 0; i < header.Naxes; i++) Ndata *= header.Naxis[i];

  nbytes = Ntotal - Ndata;
  nskip = Ndata + header.datasize;

  f = fopen (argv[1], "r");
  if (f == (FILE *) NULL) {
    fprintf (stdout, "%s: open\n", argv[1]);
    exit (1);
  }

  ALLOCATE (buffer, char, MAX (nbytes, 1));
  fseeko (f, nskip, SEEK_SET);
  Nbytes = fread (buffer, 1, nbytes, f);
  fclose (f);

  if (Nbytes != nbytes) {
    fprintf (stdout, "%s: short\n", argv[1]);
    exit (1);
  }

  for (i = 0; i < nbytes; i++) {
    if (buffer[i]) {
      fprintf (stdout, "%s: padding\n", argv[1]);
      exit (1);
    }
  }

  fprintf (stdout, "%s: ok\n", argv[1]);
  exit (0);

}



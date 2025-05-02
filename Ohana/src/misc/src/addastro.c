# include <ohana.h>
# define BLOCK 0x1000

main (int argc, char **argv) {

  /* USAGE: addastro (cmp) (ref) */

  off_t nbytes, oldsize;
  int i, j, itmp;
  double tmp;
  Header header, refhead;
  FILE *f, *g;
  char filename[1024];
  char buffer[0x1000];

  if (argc != 3) {
    fprintf (stderr, "USAGE: addastro (cmp) (ref)\n");
    exit (1);
  }

  if (!gfits_read_header (argv[1], &header)) {
    fprintf (stderr, "ERROR: can't read header for %s\n", argv[1]);
    exit (1);
  }
  oldsize = header.datasize;

  if (!gfits_read_header (argv[2], &refhead)) {
    fprintf (stderr, "ERROR: can't read header for %s\n", argv[2]);
    exit (1);
  }

  gfits_scan   (&refhead, "CDELT1",   "%lf", 1, &tmp);
  gfits_modify (&header,  "CDELT1",   "%le", 1, tmp);
  gfits_scan   (&refhead, "CDELT2",   "%lf", 1, &tmp);
  gfits_modify (&header,  "CDELT2",   "%le", 1, tmp);
  gfits_scan   (&refhead, "CRVAL1",   "%lf", 1, &tmp);
  gfits_modify (&header,  "CRVAL1",   "%lf", 1, tmp);
  gfits_scan   (&refhead, "CRVAL2",   "%lf", 1, &tmp);
  gfits_modify (&header,  "CRVAL2",   "%lf", 1, tmp);
  gfits_scan   (&refhead, "CRPIX1",   "%lf", 1, &tmp);
  gfits_modify (&header,  "CRPIX1",   "%lf", 1, tmp);
  gfits_scan   (&refhead, "CRPIX2",   "%lf", 1, &tmp);
  gfits_modify (&header,  "CRPIX2",   "%lf", 1, tmp);
  gfits_scan   (&refhead, "PC001001", "%lf", 1, &tmp);
  gfits_modify (&header,  "PC001001", "%le", 1, tmp);
  gfits_scan   (&refhead, "PC001002", "%lf", 1, &tmp);
  gfits_modify (&header,  "PC001002", "%le", 1, tmp);
  gfits_scan   (&refhead, "PC002001", "%lf", 1, &tmp);
  gfits_modify (&header,  "PC002001", "%le", 1, tmp);
  gfits_scan   (&refhead, "PC002002", "%lf", 1, &tmp);
  gfits_modify (&header,  "PC002002", "%le", 1, tmp);
  gfits_scan   (&refhead, "NPLYTERM", "%d",  1, &itmp);
  gfits_modify (&header,  "NPLYTERM", "%d",  1, itmp);
  gfits_scan   (&refhead, "NASTRO",   "%d",  1, &itmp);
  gfits_modify (&header,  "NASTRO",   "%d",  1, itmp);
  gfits_scan   (&refhead, "CERROR",   "%lf", 1, &tmp);
  gfits_modify (&header,  "CERROR",   "%lf", 1, tmp);
  gfits_scan   (&refhead, "CPRECISE", "%lf", 1, &tmp);
  gfits_modify (&header,  "CPRECISE", "%lf", 1, tmp);

  gfits_modify_alt (&header, "CERROR", "%C", 1, "scatter in astrometry soln (arcsec)");
  gfits_modify_alt (&header, "CPRECISE", "%C", 1, "precision of astrometry soln (arcsec)");

  /* write to file */
  sprintf (filename, "%s.tmp", argv[1]);
  if (!gfits_write_header (filename, &header)) {
    fprintf (stderr, "ERROR: can't write header for %s\n", filename);
    exit (1);
  }
  g = fopen (filename, "w");
  if (g == NULL) {
    fprintf (stderr, "ERROR: can't read write to %s\n", filename);
    exit (1);
  }
  nbytes = fwrite (header.buffer, 1, header.datasize, g);
  fseeko (g, header.datasize, SEEK_SET); 

  /* read date from file */
  f = fopen (argv[1], "r");
  if (f == NULL) {
    fprintf (stderr, "ERROR: can't read data from %s\n", argv[1]);
    exit (1);
  }
  fseek (f, oldsize, SEEK_SET); 

  while ((nbytes = fread (buffer, 1, BLOCK, f)) > 0) {
    if (nbytes != fwrite (buffer, 1, nbytes, g)) {
      fprintf (stderr, "ERROR: failure writing output data file\n");
      exit (0);
    }
  }
  
  fclose (f);
  fclose (g);

  sprintf (buffer, "mv %s %s~\n", argv[1], argv[1]);
  system (buffer);

  sprintf (buffer, "mv %s %s\n", filename, argv[1]);
  system (buffer);

  fprintf (stdout, "SUCCESS\n");


}


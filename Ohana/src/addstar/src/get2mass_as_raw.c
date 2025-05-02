# include "addstar.h"
# include "2mass.h"

/* unlike the DR2 data, the AS data is NOT fixed bytes/row 
 * we need to handle fractional lines at the end of each read block
 */

/* read in chunks of ~16MB */
# define NBYTE 0x1000000
# define NBREC 330

Stars *get2mass_AS_rawdata (SkyRegion *region, char *filename, SkyRegion *patch, int photcode, int *nstars) {
  
  int Nstars, NSTARS, Nbyte, Nextra;
  Stars *stars;
  FILE *f;
  char *buffer;
  char *p, *q, *tmp;
  double RA, DEC;
  double RA0, RA1, DEC0, DEC1;

  ALLOCATE (buffer, char, NBYTE);

  RA0  = MAX (patch[0].Rmin, UserPatch.Rmin);
  RA1  = MIN (patch[0].Rmax, UserPatch.Rmax);
  DEC0 = MAX (patch[0].Dmin, UserPatch.Dmin);
  DEC1 = MIN (patch[0].Dmax, UserPatch.Dmax);

  get2mass_setup (photcode);

  f = fopen (filename, "r");
  if (f == NULL) Shutdown ("can't read 2mass data file: %s", filename);
  // test if this is a raw datafile or gzipped...

  Nstars = 0;
  NSTARS = 10000;
  ALLOCATE (stars, Stars, NSTARS);

  Nextra = 0;
  while ((Nbyte = fread (&buffer[Nextra], 1, NBYTE-Nextra, f)) != 0) {
    if (Nbyte == -1) Shutdown ("error reading from raw file %s", filename);
    Nbyte += Nextra;

    if (VERBOSE) fprintf (stderr, ".");

    /* find bounds on first complete line */
    p = buffer;
    q = memchr (p, '\n', Nbyte);
    if (q == NULL) Shutdown ("incomplete line at end of file\n");

    while (1) {

      get2mass_coords (p, &RA, &DEC, Nbyte - (p - buffer));

      /* skip stars which are outside desired region */
      if (DEC > DEC1) goto skip_star;
      if (DEC < DEC0) goto skip_star;
      if (RA <  RA0)  goto skip_star;
      if (RA >  RA1)  goto skip_star;

      get2mass_star (&stars[Nstars], p, Nbyte - (p - buffer));

      Nstars ++;
      CHECK_REALLOCATE (stars, Stars, NSTARS, Nstars, 5000);

    skip_star:
      /* start of the next line */
      tmp = p;
      p = q + 1;
      if (p - buffer == Nbyte) {
	Nextra = 0;
	break;
      }
      /* end of the next line */
      q = memchr (p, '\n', Nbyte - (p - buffer));
      if (q == NULL) {
	Nextra = Nbyte - (p - buffer);
	memmove (buffer, p, Nextra);
	break;
      } 
    }
  }
  if (VERBOSE) fprintf (stderr, "\n");
  
  fclose (f);
  free (buffer);
  *nstars = Nstars;
  return (stars);
}

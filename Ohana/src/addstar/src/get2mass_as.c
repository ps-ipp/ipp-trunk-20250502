# include "addstar.h"
# include "2mass.h"

/* unlike the DR2 data, the AS data is NOT fixed bytes/row 
 * we need to handle fractional lines at the end of each read block
 */

/* read in chunks of ~16MB */
# define NBYTE 0x1000000

Stars *get2mass_AS_data (SkyRegion *region, char *filename, SkyRegion *patch, int photcode, int *nstars) {
  
  int Nstars, NSTARS, Nbyte, Nextra;
  Stars *stars;
  gzFile gf;
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

  gf = gzopen (filename, "rb");
  if (gf == NULL) Shutdown ("can't read 2mass data file: %s", filename);

  Nstars = 0;
  NSTARS = 10000;
  ALLOCATE (stars, Stars, NSTARS);

  /* I want to add a seek-ahead test to find a good starting position in the file
     this is very expensive using gzseek / gzread.  */

  Nextra = 0;
  while ((Nbyte = gzread (gf, &buffer[Nextra], NBYTE-Nextra)) != 0) {
    if (Nbyte == -1) Shutdown ("error reading from gzipped file %s", filename);
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
  
  gzclose (gf);
  free (buffer);
  *nstars = Nstars;
  return (stars);
}

/* this just scans along in the file.  file is sorted by dec, so we 
   should be skipping large chunks - but we would need to have
   the size from the accel file (won't fit in SkyRegion) and need
   to use gzseek, if it exists.
*/
/* don't bother to seek ahead : position is not sufficiently predictable 
   and gzseek is as expensive as gzread */
/* 
   Noffset = region[0].Nrec * (patch[0].DEC[0] + 90) / 180.0;
   gzseek (gf, Noffset * NBYTE, SEEK_SET);
   Nbyte = gzread (gf, buffer, NLINE*NBYTE);
*/

# if (0)
    /** need to re-think this test **/
    if (0) {
      /* search for end of last complete line */
      p = memrchr (buffer, '\n', Nbyte);
      if (p == NULL) Shutdown ("incomplete line in at end of file\n");

      /* search for start of last complete line */
      /* last block may be only one line */
      q = memrchr (buffer, '\n', (p - buffer - 1));
      if (q == NULL) {
	q = buffer;
      } else {
	q ++;
      }

      /* skip past block not yet in range */
      RA = strtod (q, NULL);
      ptr = skipNbounds (q, '|', 1, Nbyte - (q - buffer));
      DEC = strtod (ptr, NULL);
      if (DEC < DEC0) continue;
    }

# endif

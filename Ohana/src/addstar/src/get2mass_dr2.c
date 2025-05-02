# include "addstar.h"
# include "2mass.h"

# define NBYTE 302
# define NLINE 30000

Stars *get2mass_2DR_data (SkyRegion *region, char *filename, SkyRegion *patch, int photcode, int *nstars) {
  
  int i, Nstars, NSTARS, Nbyte, Nline;
  Stars *stars;
  gzFile gf;
  char *buffer;
  char line[303];
  double RA, DEC, J, H, K, dJ, dH, dK;
  double RA0, RA1, DEC0, DEC1;

  ALLOCATE (buffer, char, (NBYTE*NLINE));

  if (region == NULL) exit (2);
  if (patch == NULL) exit (3);

  RA0  = MAX (patch[0].Rmin, UserPatch.Rmin);
  RA1  = MIN (patch[0].Rmax, UserPatch.Rmax);
  DEC0 = MAX (patch[0].Dmin, UserPatch.Dmin);
  DEC1 = MIN (patch[0].Dmax, UserPatch.Dmax);

  fprintf (stderr, "overlap: %f - %f, %f - %f\n", RA0, RA1, DEC0, DEC1);

  gf = gzopen (filename, "rb");
  if (gf == NULL) Shutdown ("can't read 2mass data file: %s", filename);

  Nstars = 0;
  NSTARS = 10000;
  ALLOCATE (stars, Stars, NSTARS);

  while ((Nbyte = gzread (gf, buffer, NLINE*NBYTE)) != 0) {
    if (Nbyte ==  0) Shutdown ("error reading from gzipped file %s", filename);
    if (Nbyte == -1) Shutdown ("error reading from gzipped file %s", filename);
    if (Nbyte % NBYTE) Shutdown ("error reading complete line from gzipped file %s", filename);
    Nline = Nbyte / NBYTE;

    /* skip past block not yet in range */
    sscanf (&buffer[NBYTE*(Nline - 1)], "%lf %lf", &RA, &DEC);
    if (DEC < DEC0) continue;

    memcpy (line, &buffer[NBYTE*(Nline-1)], NBYTE);
    line[302] = 0;

    for (i = 0; i < Nline; i++) {
      
      dparse (&RA,  1, &buffer[NBYTE*i+  0]);
      dparse (&DEC, 2, &buffer[NBYTE*i+  0]);

      /* dr2 is nicely sorted in dec order */
      if (DEC > DEC1) goto finished;
      if (DEC < DEC0) continue;
      if (RA <  RA0) continue;
      if (RA >  RA1) continue;

      stars[Nstars].R  	  = RA;
      stars[Nstars].D  	  = DEC;
      stars[Nstars].t  	  = short_date_to_sec (&buffer[NBYTE*i + 164]);
      stars[Nstars].found = -1;
      stars[Nstars].detID   = 0;
      stars[Nstars].imageID = 0;

      if (photcode == TM_J) {
	dparse (&J,  1, &buffer[NBYTE*i + 53]);
	dparse (&dJ, 2, &buffer[NBYTE*i + 53]);
	stars[Nstars].M        = J;
	stars[Nstars].dM       = dJ;
	stars[Nstars].photcode = TM_J;
      }
      if (photcode == TM_H) {
	dparse (&H,  1, &buffer[NBYTE*i + 72]);
	dparse (&dH, 2, &buffer[NBYTE*i + 72]);
	stars[Nstars].M        = H;
	stars[Nstars].dM       = dH;
	stars[Nstars].photcode = TM_H;
      }
      if (photcode == TM_K) {
	dparse (&K,  1, &buffer[NBYTE*i + 91]);
	dparse (&dK, 2, &buffer[NBYTE*i + 91]);
	stars[Nstars].M        = K;
	stars[Nstars].dM       = dK;
	stars[Nstars].photcode = TM_K;
      }
      Nstars ++;
      CHECK_REALLOCATE (stars, Stars, NSTARS, Nstars, 5000);
    }
  }  
finished:
  gzclose (gf);
  free (buffer);

  *nstars = Nstars;
  return (stars);
}

/* this just scans along in the file.  file is sorted by dec, so we should be
   skipping large chunks - but we would need to have the size from the accel
   file in (and need to use gzseek, if the data is compressed)
*/
/* don't bother to seek ahead : position is not sufficiently predictable 
   and gzseek is as expensive as gzread */
/* 
   Noffset = region[0].Nrec * (patch[0].Dmin + 90) / 180.0;
   gzseek (gf, Noffset * NBYTE, SEEK_SET);
   Nbyte = gzread (gf, buffer, NLINE*NBYTE);
*/


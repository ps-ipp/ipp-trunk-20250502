# include "dvoshell.h"
# define D_NSTARS 1000
# define BYTES_STAR 66
# define BLOCK 1000

CMPstars *cmpReadFits (FILE *f, off_t *nstars) {

  off_t i, Nstars;
  Header theader;
  FTable table;
  CMPstars *stars;
  SMPData *smpdata;

  /* if no stars, no table */
  if (*nstars == 0) return (NULL);

  /* init & load in table data */
  table.header   = &theader;
  if (!gfits_fread_ftable (f, &table, "SMPFILE")) goto escape;

  smpdata = gfits_table_get_SMPData (&table, &Nstars, NULL, NULL);
  if (!smpdata) {
    fprintf (stderr, "ERROR: failed to read stars\n");
    exit (2);
  }

  ALLOCATE (stars, CMPstars, Nstars);
  for (i = 0; i < Nstars; i++) {
    stars[i].X      = smpdata[i].X;
    stars[i].Y      = smpdata[i].Y;
    stars[i].M      = smpdata[i].M;
    stars[i].dM     = smpdata[i].dM;
    stars[i].dophot = smpdata[i].dophot;

    stars[i].Mgal   = smpdata[i].M;
    stars[i].Map    = smpdata[i].dM;
    stars[i].fx     = smpdata[i].fx;
    stars[i].fy     = smpdata[i].fy;
    stars[i].df     = smpdata[i].df;
  }    
  *nstars = Nstars;
  return (stars);

escape:
  gprint (GP_ERR, "error reading file\n");
  *nstars = 0;
  return (NULL);
}

CMPstars *cmpReadText (FILE *f, off_t *nstars) {

  off_t N;
  int j, Nextra, Ninstar, Nskip, Nbytes, nbytes;
  int done;
  char *buffer, *c, *c2;
  double tmp;
  CMPstars *stars;
  
  /* load in stars by blocks of 1000 */
  N = 0;
  ALLOCATE (buffer, char, (BLOCK*BYTES_STAR) + 1);
  buffer[BLOCK*BYTES_STAR] = 0;
  Nextra = 0;

  ALLOCATE (stars, CMPstars, *nstars);

  while (N < *nstars) {
    /* load next data block */
    Nbytes = BYTES_STAR * BLOCK - Nextra;
    nbytes = fread (&buffer[Nextra], 1, Nbytes, f);
    if (nbytes == 0) {
      *nstars = N;
      return (stars);
    }
    nbytes += Nextra;

    /* check line-by-line integrity */
    c = buffer;
    done = FALSE;
    while ((c < buffer + nbytes) && (!done)) { 
      for (c2 = c; *c2 == '\n'; c2++);
      if (c2 > c) { /* extra return chars */
	memmove (c, c2, (int)(buffer + nbytes - c2));
	Nskip = c2 - c;
	nbytes -= Nskip;
	bzero (buffer + nbytes, Nskip);
	/* if (VERBOSE) gprint (GP_ERR, "deleted %d extra return chars\n", Nskip); */
      }
      c2 = strchr (c, '\n');
      if (c2 == (char *) NULL) {
	done = TRUE;	
	continue;
      }
      c2++;
      if ((c2 - c) != BYTES_STAR) { /* bad line, delete it */
	memmove (c, c2, (int)(buffer + nbytes - c2));
	Nskip = c2 - c;
	nbytes -= Nskip;
	bzero (buffer + nbytes, Nskip);
	/* if (VERBOSE) gprint (GP_ERR, "deleted line, %d extra chars\n", Nskip); */
      } else {
	c = c2;
      }
    }

    /* extract data for stars */
    Ninstar = nbytes / BYTES_STAR;
    Nextra = nbytes % BYTES_STAR;
    for (j = 0; (j < Ninstar) && (N < *nstars); j++, N++) {
      dparse (&stars[N].X,  1, &buffer[j*BYTES_STAR]);
      dparse (&stars[N].Y,  2, &buffer[j*BYTES_STAR]);
      dparse (&stars[N].M,  3, &buffer[j*BYTES_STAR]);

      /* cmp files carry dM in millimags */
      dparse (&tmp, 4, &buffer[j*BYTES_STAR]);
      stars[N].dM = 0.001*tmp;

      dparse (&tmp,         5, &buffer[j*BYTES_STAR]);
      stars[N].dophot = tmp;

      dparse (&stars[N].Mgal, 7, &buffer[j*BYTES_STAR]);
      dparse (&stars[N].Map,  8, &buffer[j*BYTES_STAR]);
      dparse (&stars[N].fx,   9, &buffer[j*BYTES_STAR]);
      dparse (&stars[N].fy,  10, &buffer[j*BYTES_STAR]);
      dparse (&stars[N].df,  11, &buffer[j*BYTES_STAR]);
    }
  }
  *nstars = N;
  return (stars);
}

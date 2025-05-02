
# define TIMESTAMP(TIME) \
    gettimeofday (&stop, (void *) NULL);	\
    dtime = DTIME (stop, start);		\
    TIME += dtime;				\
    gettimeofday (&start, (void *) NULL);


  // XXX quick and stupid test:
  if (1) {
    int i, j; 
    off_t *Nlist, *NLIST, **mlist;
    off_t *NlistI, *NLISTI, **mlistI;
    int **clist;
    int **clistI;
    float dtime;
    float time1 = 0.0;
    float time2 = 0.0;

    int Nmosaic = 10000;
    int Nimage = 600000;

    for (j = 0; j < 19000; j++) {

      // mosaic indexes
      ALLOCATE (Nlist, off_t,   Nmosaic);
      ALLOCATE (NLIST, off_t,   Nmosaic);
      ALLOCATE (clist, int *,   Nmosaic);
      ALLOCATE (mlist, off_t *, Nmosaic);
    
      for (i = 0; i < Nmosaic; i++) {
	Nlist[i] = 0;
	NLIST[i] = 100;
	ALLOCATE (clist[i], int,   NLIST[i]);
	ALLOCATE (mlist[i], off_t, NLIST[i]);
      }

      // image indexes
      ALLOCATE (NlistI, off_t,   Nimage);
      ALLOCATE (NLISTI, off_t,   Nimage);
      ALLOCATE (clistI, int *,   Nimage);
      ALLOCATE (mlistI, off_t *, Nimage);
    
      for (i = 0; i < Nimage; i++) {
	NlistI[i] = 0;
	NLISTI[i] = 100;
	ALLOCATE (clistI[i], int,   NLISTI[i]);
	ALLOCATE (mlistI[i], off_t, NLISTI[i]);
      }

      TIMESTAMP(time1);

      // free mosaic indexes
      for (i = 0; i < Nmosaic; i++) {
	free (clist[i]);
	free (mlist[i]);
      }
      free (Nlist);
      free (NLIST);
      free (clist);
      free (mlist);

      // free image indexes
      for (i = 0; i < Nimage; i++) {
	free (clistI[i]);
	free (mlistI[i]);
      }
      free (NlistI);
      free (NLISTI);
      free (clistI);
      free (mlistI);

      TIMESTAMP(time2);

      if (j % 10 == 0) {
	fprintf (stderr, "done with %d\n", j);
	fprintf (stderr, "time1  %f : init mosaic\n", time1);
	fprintf (stderr, "time2  %f : free mosaic\n", time2);
      }
    }
    
    fprintf (stderr, "time1  %f : init mosaic\n", time1);
    fprintf (stderr, "time2  %f : free mosaic\n", time2);
    exit (1);
  }


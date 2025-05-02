# include "gophot.h"
float set_thresholds (float, float, float *, int *);
void large_features (float, float);
int fix_mediansky  (float);
float get_mediansky (int, int);

int dophot (void) {

  bool first, lastround;
  float factor, sky, dsky;
  int nstar, Nit, i, j;
  char c;
  int xtest, ytest;
  struct timeval now, then;  
  
  makenoise ();
  make_mediansky ();
  get_skystats (&sky, &dsky);
  fix_mediansky (sky);

  /*
  gettimeofday (&then, (void *) NULL);
  for (i = 0; i < nfast; i++) {
    for (j = 0; j < nslow; j++) {
      sky = get_mediansky (i, j);
      big[j*nfast + i] -= sky;
    }
  }
  gettimeofday (&now, (void *) NULL);
  fprintf (stderr, "elapsed time = %.2f sec\n", 
	   (now.tv_sec - then.tv_sec) + 1e-6*(now.tv_usec - then.tv_usec));

  gfits_write_header ("test.sub", &header);
  gfits_write_matrix ("test.sub", &matrix);
  exit (0);
  */

  thresh = set_thresholds (sky, dsky, &factor, &Nit);

  /* large_features (sky, dsky); */

  first = TRUE;
  lastround = FALSE;

  for (i = 0; i < Nit + 1; i++) {
    mprint (0, "starting loop at threshold level %f\n", thresh);
	   
    if (i == Nit) lastround = TRUE;

    makemask ();

    /* fix pos needs to be defined correctly */
    if (fixpos && first) improve (FALSE);
	   
    nstar = isearch (first);
    shape ();
    paravg ();

    improve (lastround);
	   
    mprint (1, " ending loop at threshold level %f\n", thresh); 
    mprint (1, " number of new objects found on this threshold = %f\n", nstar); 
    mprint (1, " total number of objects found so far = %d\n", nstot); 
    
    thresh /= factor;

    first = FALSE;

# if (0)
    gfits_write_header ("test.sub", &header);
    gfits_write_matrix ("test.sub", &matrix);

    completeout (nstot);
    fprintf (stderr, "type return to continue:  ");
    fscanf (stdin, "%c", &c);
# endif

  }

  feature_fluxes ();

  if (lastround) {
    if (flags[3] == COMPLETE) completeout (nstot);
  }
	   
    /* 
    gfits_write_header ("test.sub", &header);
    gfits_write_matrix ("test.sub", &matrix);
    completeout (nstot);
    fprintf (stderr, "type return to continue:  ");
    fscanf (stdin, "%c", &c);
    */

}

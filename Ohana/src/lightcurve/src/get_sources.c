# include "lightcurve.h"
# define D_NSOURCES 200

void get_sources (sources, Nsources)
Star  **sources;
int    *Nsources;
{
  
  int N, i;

  ALLOCATE (sources[0], Star, D_NSOURCES);

  for (i = 0; (fscanf (stdin, "%lf %lf", &sources[0][i].RA, &sources[0][i].Dec) != EOF); i++) { 
    sources[0][i].unique_number = EMPTY;
    if (i == D_NSOURCES - 1) {
      fprintf (stderr, "No More Sources: %d\n", i);
      break;
    }
  }
  *Nsources = i;
  fprintf (stderr, "Nsources: %d\n", *Nsources);
}


  

# include "addstar.h"
# include "ukirt_uhs.h"

UKIRT_Stars *loadukirt_uhs_readstars (FILE *f, char *buffer, int *nstart, AddstarClientOptions *options, int *nstars) {

  switch (UKIRT_MODE) {
    case UKIRT_MODE_UHS: {
      UKIRT_Stars *stars = loadukirt_uhs_readstars_uhs (f, buffer, nstart, options, nstars);
      return stars;
    }
      
    case UKIRT_MODE_UGCS: {
      UKIRT_Stars *stars = loadukirt_uhs_readstars_ugcs (f, buffer, nstart, options, nstars);
      return stars;
    }
      
    case UKIRT_MODE_UGPS: {
      UKIRT_Stars *stars = loadukirt_uhs_readstars_ugps (f, buffer, nstart, options, nstars);
      return stars;
    }
      
    case UKIRT_MODE_ULAS: {
      UKIRT_Stars *stars = loadukirt_uhs_readstars_ulas (f, buffer, nstart, options, nstars);
      return stars;
    }
      
    case UKIRT_MODE_UHS2022: {
      UKIRT_Stars *stars = loadukirt_uhs_readstars_uhs2022 (f, buffer, nstart, options, nstars);
      return stars;
    }
      
    default:
      fprintf (stderr, "programming error: invalid mode\n");
      exit (1);
  }
  return NULL;
}

int loadukirt_uhs_sortStars (UKIRT_Stars *stars, int Nstars) {

# define SWAPFUNC(A,B){ UKIRT_Stars temp = stars[A]; stars[A] = stars[B]; stars[B] = temp; }
# define COMPARE(A,B)(stars[A].average.R < stars[B].average.R)

  OHANA_SORT (Nstars, COMPARE, SWAPFUNC);

# undef SWAPFUNC
# undef COMPARE
  
  return TRUE;
}

float psfQFfromXClass (int xClass) {

  float value = NAN;
  switch (xClass) {
    case -1: value = 0.99; break; // star
    case -2: value = 0.90; break; // probable star
    case -3: value = 0.40; break; // probable galaxy
    case +1: value = 0.20; break; // galaxy
    case  0: value = 0.10; break; // noise
    default: break;
  }

  return value;
}


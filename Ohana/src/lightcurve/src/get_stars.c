# include "lightcurve.h"
# define D_NSTARS 1000

void get_stars (stars, Nstars, images, Nimages)
Star  **stars;
int    *Nstars;
Image   images[];
int     Nimages;
{

  double type, mag, dmag, X, Y, ap, dap, AmF, A, A2, S;
  int i, j, image_number;
  int NSTARS, status, n, nperf;
  FILE *f;
  char line[200];

  j = 0;
  NSTARS = D_NSTARS;
  ALLOCATE (stars[0], Star, NSTARS);

  for (i = 0; i < Nimages; i++) { 
    
    get_info (&images[i]);
    f = fopen (images[i].name, "r"); 
    if (f == NULL) { 
      fprintf (stderr, "failed to open %s\n", images[i].name); 
      exit (0); 
    }
    nperf = A = A2 = S = 0;
    for (n = 0; scan_line (f, line) != EOF; n++) {
      status = TRUE;
      status &= dparse (&type, 2, line);
      status &= dparse (&X,    3, line);
      status &= dparse (&Y,    4, line);
      status &= dparse (&mag,  5, line);
      status &= dparse (&dmag, 6, line);
      status &= dparse (&ap,  12, line);
      status &= dparse (&dap, 13, line);
      status &= dparse (&AmF, 15, line);
      if (!status) {
	fprintf (stderr, "error on line %d in file %s\n", n, images[i].name);
	continue;  /* go on to the next line */
      }
      stars[0][j].ap = 0;
      if (ap < 99) {
	nperf ++;
	A  += AmF/SQ(dap);
	A2 += SQ(AmF)/SQ(dap);
	S  += 1.0/SQ(dap);
	stars[0][j].ap = ap;    /* if the ap mag is good, use it, not the <ap-fit> adjusted value */
      }
      if ((dmag < MCUTOFF) && ((type == 1) || (EXTRASTARS && ((type == 2) || (type == 3) || (type == 4) || (type == 5) || (type == 7))))) {  /* for now, this uses only type 1s */
	stars[0][j].RA  = images[i].RA_O  + X*images[i].RA_X  + Y*images[i].RA_Y;
	stars[0][j].Dec = images[i].DEC_O + X*images[i].DEC_X + Y*images[i].DEC_Y;
	stars[0][j].m   = mag + images[i].Mtime;
	stars[0][j].dm  = sqrt (SQ(dmag) + SQ(images[i].dMcal));
	
	stars[0][j].next_this_unique = NULL;
	stars[0][j].image_number     = i;
	stars[0][j].star_number      = n;
	stars[0][j].unique_number    = EMPTY;
	
	if (j == NSTARS - 1) {
	  NSTARS += D_NSTARS;
	  fprintf (stderr, "!");
	  REALLOCATE (stars[0], Star, NSTARS);
	}
	j ++;
	images[i].Nstars ++;   /* this is set to 0 in "get_info" */
      }
    }
    if (nperf > 2) {
      images[i].AmF = A / S;
      images[i].dAmF = sqrt(A2/S - A*A/SQ(S));
    }
    else {
      images[i].AmF = 0.0;
      images[i].dAmF = 0.0;
    }     
    fclose (f);
/*    images[i].Mcal    = - C_LAMBDA + A_LAMBDA*images[i].airmass - images[i].AmF; */
    stars[0][j - 1].next_this_image = NULL;
  }
  
  image_number = 0;
  for (i = 0; i < j; ) {
    images[image_number].first_this_image = &stars[0][i];
    for (i++; ((stars[0][i].image_number == image_number) && (i < j)); i++)  {
      stars[0][i - 1].next_this_image = &stars[0][i];
    }
    stars[0][i - 1].next_this_image = NULL;
    image_number = stars[0][i].image_number;
  }

  *Nstars = j;
  fprintf (stderr, "Nstars: %d\n", *Nstars);
}

  

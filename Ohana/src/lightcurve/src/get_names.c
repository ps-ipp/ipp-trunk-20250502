# include "lightcurve.h"
# define D_NIMAGE 200
  
void get_names (images, Nimages)
Image **images;
int   *Nimages;
{
  
  int N, i;
  char line[500];
  FILE *f;

  f = fopen (IMAGES, "r"); 
  if (f == NULL) { 
    fprintf (stderr, "failed to open %s\n", IMAGES); 
    exit (0); 
  }
  N = D_NIMAGE;
  ALLOCATE (images[0], Image, N);

  for (i = 0; (fscanf (f, "%s", line) != EOF); i++) { 
    strcpy (images[0][i].name, line);
    strcpy (strchr(images[0][i].name, '.'), ".obj_out");
    if (i == N - 1) {
      N += D_NIMAGE;
      REALLOCATE (images[0], Image, N);
    }
  }
  *Nimages = i;
  fprintf (stderr, "Nimages: %d\n", *Nimages);
  fclose (f);
}



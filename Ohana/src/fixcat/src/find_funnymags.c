# include "markstar.h"

find_funnymags (catalog, images, Nimage)
Catalog catalog[];
Image images[];
int Nimage;
{

  int i;
  int *bad;

  ALLOCATE (bad, int, Nimage);
  bzero (bad, Nimage*sizeof(int));

  for (i = 0; i < catalog[0].Nmeasure; i++) {
    if (catalog[0].measure[i].M < 9000) {
      bad[catalog[0].image[i]] ++;
    }
  }

  for (i = 0; i < Nimage; i++) {
    if (bad[i] > 0) {
      fprintf (stdout, "%s %d %d\n", images[i].name, images[i].tzero, bad[i]);
    }
  }

}

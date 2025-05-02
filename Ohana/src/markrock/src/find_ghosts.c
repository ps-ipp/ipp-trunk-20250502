# include "markrock.h"

find_ghosts (catalog, catstats, filename, image, Nimage)
Catalog catalog[];
CatStats catstats[];
char *filename;
Image image[];
int Nimage;
{

  int i, j, first_j, Nave, Ngsc, Nregions;
  Catalog GSCdata, Ghostdata;
  double MinRA, MinDEC, MaxRA, MaxDEC, RaCenter, DecCenter;
  Coords *tcoords;
  float *X1, *Y1, *X2, *Y2;
  int *N1, *N2, *match;
  char *mark;
  double dX, dY, dR, RADIUS2, BRIGHT_RADIUS, radius, X, Y, R, D;
  Image timage;
  GSCRegion *region, *gregions();

  for (i = 0; i < Nimage; i++) {
    
    /* coords structure for ghost image */
    timage = image[i];
    timage.coords.cdelt1 = -1*image[i].coords.cdelt1;
    timage.coords.cdelt2 = -1*image[i].coords.cdelt2;
    timage.coords.crpix1 = 2*OPTICAL_AXIS1 - image[i].coords.crpix1;
    timage.coords.crpix2 = 2*OPTICAL_AXIS2 - image[i].coords.crpix2;

    region = gregions2 (&timage, &Nregions);
    
    /* find ghost stars */
    load_gsc_data_ghost (&GSCdata, region, Nregions, &timage);

    /* refect ghost stars to find locations of ghosts */
    ALLOCATE (Ghostdata.average, Average, MAX (GSCdata.Naverage, 1));
    for (j = 0; j < GSCdata.Naverage; j++) {
      RD_to_XY (&X, &Y, GSCdata.average[j].R, GSCdata.average[j].D, &timage);
      fXY_to_RD (&Ghostdata.average[j].R, &Ghostdata.average[j].D, X, Y, &image[i]);
      Ghostdata.average[j].M = GSCdata.average[j].M;
    }    
    Ghostdata.Naverage = GSCdata.Naverage;

    /* match ghosts and image stars, mark ghosts */
    find_matches (catalog, catstats, &Ghostdata, i);

    free (Ghostdata.average);
    free (GSCdata.average);
  }

}

/* 

   just to be clear on some of the terms:

   the "ghost" is the fuzzy patch of light on an image caused by
     a reflected star

   the "ghost star" is the star in the sky which causes a ghost

   the "ghost image" is the image location on the sky 
     where ghost stars may come from.

   the "image stars" are observed stars at the location of the
     ghost - these are detections of the ghost


*/

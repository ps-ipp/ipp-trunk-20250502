# include "dvomerge.h"

Image *MatchImage (Image *image, off_t Nimage, unsigned int time, short int source, unsigned int imageID) { 

  int m = -1;

  if ((imageID != 0) && (imageID < Nimage)) {
    // imageID is in range for the array of images. If the table is still in order and
    // no images have been deleted the index of the image we are looking for will be imageID - 1
    // If this is the case, we have it. Otherwise we'll have to go search for it below
    int guess = (int) imageID - 1;
    if (image[guess].imageID == imageID) {
        m = guess;
    }
  } 
  if (m == -1) {
    m = match_image (image, Nimage, time, source);
  }
  if (m == -1) return (NULL);
  return (&image[m]);
}

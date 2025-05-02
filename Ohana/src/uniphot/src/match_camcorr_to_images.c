# include "setphot.h"

// the date/time of the image is used to find the 'season'
// the photcode is used to find the actual flat correction

int match_camcorr_to_images (Image *image, off_t Nimage, CamPhotomCorrection *camcorr) {

  int i, j;

  // we have an array of CamPhotomCorrection->matrix values, where each matrix is a flat-field image
  // the sequence is seq = ix + Nx_chip*iy + filter*Nchips + season*(Nchips*Nfilter)

  // we derive ix,iy,filter from photcode : 10143 -> ix = 4, iy = 3, filter = 1
  // season comes from the date/time

  int minCodeGPC1 = 10000;
  int maxCodeGPC1 = 10576;

  int minCodeHSC  = 20000;
  int maxCodeHSC  = 26111;

  int Nmissed = 0;

  int isGPC1 = FALSE;
  int isHSC  = FALSE;
  
  for (i = 0; i < Nimage; i++) {
    if (!image[i].photcode) continue; // skip PHU images

    if ((image[i].photcode >= minCodeGPC1)&&(image[i].photcode <= maxCodeGPC1)) {
      isGPC1 = TRUE;
      isHSC  = FALSE;
    }
    else if ((image[i].photcode >= minCodeHSC)&&(image[i].photcode <= maxCodeHSC)) {
      isGPC1 = FALSE;
      isHSC  = TRUE;
    }

    if (!(isGPC1) && !(isHSC)) { continue; }
    
    int found = FALSE;
    for (j = 0; !found && (j < camcorr->Nseason); j++) {
      if (image[i].tzero < camcorr->tstart[j]) continue;
      if (image[i].tzero > camcorr->tstop[j]) continue;

      int seq = 0;
      int photcode = image[i].photcode;

      if (isGPC1) { 
	int iy =  photcode % 10;
	int ix = (int)(photcode / 10) % 10;
	int filter = (int)(photcode / 100) % 10;

	seq = ix + iy * camcorr->Nx + filter * camcorr->Nchips + j * camcorr->Nflats;
      }
      else if (isHSC) {
	int filter = (int)(photcode / 1000) % 10;
	int ix = photcode - 20000 - filter * 1000;
	
	seq = ix + filter * camcorr->Nchips + j * camcorr->Nflats;
      }

      // we add one so photom_map_id = 0 can mean no map
      image[i].photom_map_id = seq + 1;
      found = TRUE;
    }
    if (!found) {
      Nmissed ++;
      fprintf (stderr, "image does not match flatcorr Table ranges: %s\n", image[i].name);
    }

    if (Nmissed > 100)  {
      fprintf (stderr, "something is wrong: too many images do not match\n");
      exit (2);
    }

  }
  return TRUE;
}

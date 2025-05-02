# include "addstar.h"

/* set specific options based on the image collection */
int ImageOptions (AddstarClientOptions *options, Image *images, off_t Nimages) {

  off_t i;
  int equivPhotcode, consistent;
  float maxError;
  PhotCode *photcode;

  // set the radius to the maximum error circle
  if (options[0].radius == 0) {
    maxError = 0;
    for (i = 0; i < Nimages; i++) {
      maxError = MAX (maxError, 0.02 * images[i].cerror);
    }
    options[0].radius = options[0].Nsigma * maxError;
  }     

  // check that all images have the same equiv photcode and save it
  // XXX this is only used to allow use to calculate an average mag
  // if we have mis-matched photcodes, leave this as 0;
  options[0].photcode = 0;
  equivPhotcode = 0;
  consistent = TRUE;

  for (i = 0; i < Nimages; i++) {
    /* MOSAIC_PHU images do not have a photcode */
    if (!strcmp (&images[0].coords.ctype[4], "-DIS")) continue;

    photcode = GetPhotcodebyCode (images[i].photcode);

    if (equivPhotcode) {
      if (equivPhotcode != photcode[0].equiv) {
	consistent = FALSE;
	break;
      }
    } else {
      equivPhotcode = photcode[0].equiv;
    }
  }
  if (consistent) {
    options[0].photcode = equivPhotcode;
  }

  options[0].imageID = 0;
  return (TRUE);
}

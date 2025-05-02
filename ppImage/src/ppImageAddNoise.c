# include "ppImage.h"

/* This function degrades MD exposures to 3Pi exposures, by adding appropriate noise
   and scaling the values in the image - Daniel Farrow.

   EAM : Currently, this function is totally hard-wired to use values for GPC1 MD fields.
   The sigmas should be included in a recipe, and probably calculated based on a target
   exptime.

 */

bool ppImageAddNoise(pmConfig *config, ppImageOptions *options, pmFPAview *view, pmFPA *fpa) 
{

  
  // this step is optional.
  if (!options->addNoise) {
    return true;
  }

  // psTimerStart("add.noise");

  // grizy variances to add to turn MD exposure -> 3pi, calculated from the DRM 
  // static float add_sigmas[] = {NAN, 13.39, 23.79, 75.25, 110.19, 128.84};
  
  // Target Exposure times for 3pi in grizy
  static float expTimes3Pi[] = {NAN, 43.0, 40.0, 45.0, 30.0, 30.0}; 
  float expTime = psMetadataLookupF32(NULL, fpa->concepts, "FPA.EXPOSURE"); // Exposure time for image

  // Something to choose the band, g,r,i,z,y = 0,1,2,3,4 respectively
  char *filter = psMetadataLookupStr (NULL, fpa->concepts, "FPA.FILTERID");
  
  int band = 0;
  if (!strcmp(filter, "g")) {
    band = 1;
  }
  if (!strcmp(filter, "r")) {
    band = 2;
  }
  if (!strcmp(filter, "i")) {
    band = 3;
  }
  if (!strcmp(filter, "z")) {
    band = 4;
  }
  if (!strcmp(filter, "y")) {
    band = 5;
  }
  if (!band) {
    psError(PS_ERR_UNKNOWN, true, "ppImageAddNoise doesn't recognise the filter %s, aborting", filter);
    return false;
  }

  pmReadout *inReadout = pmFPAfileThisReadout(config->files, view, "PPIMAGE.INPUT");
  
  // find the currently selected readout
  psImage *image = inReadout->image;
  psImage *variance = inReadout->variance;
  
  // Warning, just in case this is left on accidently!
  psWarning("addNoise is set; adding noise to the images.");

  // Add in appropriate variance, and scale the image 
  float rho  =  expTime/expTimes3Pi[band];
  float rho2 = PS_SQR(rho);

  psRandom *rng = psRandomAlloc(PS_RANDOM_TAUS); // Random number generator

  psStats *stats = psStatsAlloc (PS_STAT_SAMPLE_MEDIAN);
  stats->nSubsample = 10000;

  psImageBackground(stats, NULL, inReadout->image, inReadout->mask, 0xffff, rng);
  double MSKY_MN = stats->sampleMedian;

  // set GAIN and RDNOISE to nominal values:
  double GAIN = 1.0; // electrons / DN
  double RDNOISE = 6.0; // electrons (== DN)

  double add_sigma = sqrt((MSKY_MN/GAIN)*(rho - 1.0) + PS_SQR(RDNOISE)*(rho2 - 1.0));

  fprintf (stderr, "mean sky: %f, scaling by %f, adding %f before re-scaling\n", MSKY_MN, rho, add_sigma);

  for (int iy = 0; iy < image->numRows; iy++){
    for (int ix = 0; ix < image->numCols; ix++){
      image->data.F32[iy][ix] += ppImageRandomGaussian(rng, 0.0, add_sigma);
      image->data.F32[iy][ix] /= rho;
      variance->data.F32[iy][ix] += PS_SQR(add_sigma);
      variance->data.F32[iy][ix] /= rho2;
    }
  }
	  
  // Update the metadata about exposure time
  psMetadataAddF32(inReadout->parent->concepts, PS_LIST_TAIL, "CELL.EXPOSURE", PS_META_REPLACE, "the modified exposure time", expTimes3Pi[band]);
  psMetadataAddF32(inReadout->parent->concepts, PS_LIST_TAIL, "FPA.EXPOSURE", PS_META_REPLACE, "the modified exposure time", expTimes3Pi[band]);
  psMetadataAddF32(fpa->concepts, PS_LIST_TAIL, "EXPTIME", PS_META_REPLACE, "the modified exposure time", expTimes3Pi[band]);
  ppImageRandomGaussianFree();
  psFree(rng);
  psFree(stats);
  
  // psLogMsg ("ppImage", 5, "add noise: %f sec\n", psTimerMark ("add.noise"));

  return true;
}

# include "relphot.h"

// we've just reloaded the data from disk; we now need to apply the Image/Mosaic/Grid
// calibrations determined by the rest of the program.  We also need to set the final
// output dbFlags values

void setMrelFinal (Catalog *catalog, int simpleAverage) {

  off_t i;

  int Nsecfilt = GetPhotcodeNsecfilt ();

  // this resets values only if -reset is used
  ResetAverageAndMeasure (catalog);
  
  // This sets ID_MEAS flags in the measureT element, not the measure element
  // These bits are thus ephemeral and not saved.  
  setExclusions (catalog, 1, VERBOSE);  /* mark by area */

  /* set catalog[0].found[i] = FALSE */
  ALLOCATE (catalog[0].found_t, off_t, MAX (1, Nsecfilt*catalog[0].Naverage));
  ALLOCATE (catalog[0].foundWarp_t, off_t, MAX (1, Nsecfilt*catalog[0].Naverage));
  for (i = 0; i < Nsecfilt*catalog[0].Naverage; i++) {
    catalog[0].found_t[i] = FALSE;
    catalog[0].foundWarp_t[i] = FALSE;
  }

  ALLOCATE (catalog[0].measureRank, char, catalog[0].Nmeasure);
  setMeasureRank (catalog);

  setMflatFromGrid (catalog); // Mgrid is used to set Mflat; Mgrid is the ignored in setMrelOutput / setMrelCatalog
  setMrelOutput (catalog, 1); // sets the values secfilt.MpsfChp = <measure.M - image.Mcal - measure.Mflat>
  setMcalOutput (catalog, 1); // sets measure.Mcal = image.Mcal
}

// int print_measure_set (Average *average, SecFilt *secfilt, Measure *measure) {
// 
//   off_t k;
// 
//   int Nsecfilt = GetPhotcodeNsecfilt ();
// 
//   off_t m = average[0].measureOffset;
// 
//   for (k = 0; k < average[0].Nmeasure; k++, m++) {
//     fprintf (stderr, "meas: %08x\n", measure[m].dbFlags);
//   }
// 
//   int Ns;
//   for (Ns = 0; Ns < Nsecfilt; Ns++) {
//     fprintf (stderr, "secf: %08x\n", secfilt[Ns].flags);
//   }
//   return 1;
// }

// This function is only called for the final output step.  By this point, we have
// propagated the mosaic and tgroup flags to each image.
void setMeasureRank (Catalog *catalog) {

  int i;

  Measure     *measure     = catalog[0].measure;
  char        *measureRank = catalog[0].measureRank;

  /* set measureRank[] based on various quality measurements */
  for (i = 0; i < catalog[0].Nmeasure; i++) {
    measureRank[i] = 11; // start at a low rank

    // measurements without a valid photcode have lowest rank (should not be used anyway)
    PhotCode *code = GetPhotcodebyCode (measure[i].photcode);
    if (!code) continue;

    // measurements outside time range have poor rank
    if (TimeSelect) {
      if (measure[i].t < TSTART) 		                        { measureRank[i] = 10; continue; }
      if (measure[i].t > TSTOP)  		                        { measureRank[i] = 10; continue; }
    }
    
    // Nim < 0 for REF mags, imageFlags have bits for IMAGE, MOSAIC, NIGHT
    off_t Nim      = getImageEntry (i, 0);
    int imageFlags = getImageFlags (i, 0);

    if (Nim > -1) {
      // measurements ranked by inst mag limit (not REF mags)
      if (ImagSelect) {
	float mag = PhotInst (&measure[i], MAG_CLASS_PSF);
	if (mag < ImagMin) 			                        { measureRank[i] = 9; continue; }
	if (mag > ImagMax) 			                        { measureRank[i] = 9; continue; }
      }
    }

    // RANK 8 : Poor image
    if (Nim > -1) {
      if (imageFlags & ID_IMAGE_PHOTOM_POOR)                            { measureRank[i] = 8; continue; }
    }

    // RANK 7 : BAD photFlags (eg, SAT, CR), internal outliers
    if (measure[i].photFlags & code->photomBadMask)                     { measureRank[i] = 7; continue; }

    // RANK 6 : bad psfQF value
    if (!isfinite(measure[i].psfQF) || measure[i].psfQF < 0.85)         { measureRank[i] = 6; continue; }
	
    // RANK 5 : not in valid chip region 
    if (measure[i].dbFlags & ID_MEAS_AREA)                              { measureRank[i] = 5; continue; }

    // RANK 4 : POOR photFlags
    if (measure[i].photFlags & code->photomPoorMask)                    { measureRank[i] = 4; continue; }
    
    // RANK 3 : bad psfQFperfect value
    if (!isfinite(measure[i].psfQFperf) || measure[i].psfQFperf < 0.85) { measureRank[i] = 3; continue; }
	
    // RANK 2 : Poor mosaic
    if (Nim > -1) {
      if (imageFlags & ID_IMAGE_MOSAIC_POOR)                            { measureRank[i] = 2; continue; }
    }

    // RANK 1 : Poor night
    if (Nim > -1) {
      if (imageFlags & ID_IMAGE_NIGHT_POOR)                             { measureRank[i] = 1; continue; }
    }
    // RANK 0 : perfect measurement:
    measureRank[i] = 0;
  }
}

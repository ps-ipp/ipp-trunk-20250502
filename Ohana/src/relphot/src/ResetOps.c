# include "relphot.h"

static DVOAverageFlags photomBitsAverage = 
  ID_OBJ_EXT             | // extended in our data (eg, PS)
  ID_OBJ_EXT_ALT         | // extended in external data (eg, 2MASS)
  ID_OBJ_GOOD            | // good-quality measurement in our data (eg,PS)
  ID_OBJ_GOOD_ALT        | // good-quality measurement in  external data (eg, 2MASS)
  ID_OBJ_GOOD_STACK      | // good-quality object in the stack (> 1 good stack)
  ID_OBJ_BEST_STACK      | // the primary stack measurement are the best measurements
  ID_OBJ_SUSPECT_STACK   | // suspect object in the stack (> 1 good or suspect stack, < 2 good)
  ID_OBJ_BAD_STACK;        // good-quality object in the stack (> 1 good stack)

// flags used by the photometry analysis (excluding UBERCAL)
static DVOMeasureFlags photomBitsMeasure = 
  ID_MEAS_NOCAL          | // detection ignored for this analysis (photcode, time range)
  ID_MEAS_POOR_PHOTOM    | // detection is photometry outlier 
  ID_MEAS_SKIP_PHOTOM    | // detection was ignored for photometry measurement 
  ID_MEAS_AREA           | // detetion was outside acceptable area of device
  ID_MEAS_SYNTH_MAG      | // magnitude is synthetic
  ID_MEAS_STACK_PRIMARY  | // this stack measurement is in the primary skycell
  ID_MEAS_STACK_PHOT_SRC | // this measurement supplied the stack photometry
  ID_MEAS_PHOTOM_PSF     | // this measurement is used for the mean psf mag
  ID_MEAS_PHOTOM_APER    | // this measurement is used for the mean ap mag
  ID_MEAS_PHOTOM_KRON    | // this measurement is used for the mean kron mag
  ID_MEAS_MASKED_PSF     | // this measurement is masked based on IRLS weights for mean psf mag
  ID_MEAS_MASKED_APER    | // this measurement is masked based on IRLS weights for mean ap mag
  ID_MEAS_MASKED_KRON    ; // this measurement is masked based on IRLS weights for mean kron mag

// ID_MEAS_PHOTOM_UBERCAL -- externally-supplied zero point from ubercal analysis
// this is set by 'setphot', do not reset here

// flags used by the photometry analysis (excluding UBERCAL)
// unsigned int secfiltFlags = 
//   ID_PHOTOM_PASS_0 | // average measured at pass 0
//   ID_PHOTOM_PASS_1 | // average measured at pass 1
//   ID_PHOTOM_PASS_2 | // average measured at pass 2
//   ID_PHOTOM_PASS_3 | // average measured at pass 3
//   ID_PHOTOM_PASS_4 | // average measured at pass 3
//   ID_SECF_USE_SYNTH | // average measured at pass 3
//   ID_SECF_USE_UBERCAL | // average measured at pass 3
//   ID_SECF_OBJ_EXT; // average measured at pass 3
    

// Used in bcatalog.c, applied to the subset catalogs (bright, good stars used for calibration)
void ResetAverageActivePhotcodes (SecFilt *secfilt) {

  if (!RESET) return;

  int Ns;

  // only loop over the active photcodes
  for (Ns = 0; Ns < Nphotcodes; Ns++) {

    int thisCode = photcodes[Ns][0].code;
    int Nsec = GetPhotcodeNsec(thisCode);

    dvo_secfilt_init (&secfilt[Nsec], SECFILT_RESET_ALL);
  }
}

// Used in bcatalog.c, applied to the selected measurements which are already selected by photcode
void ResetMeasureZeroPoints (MeasureTiny *measure, off_t Nmeasure, off_t Ncat) {

  if (!RESET) return;

  // only reset Mcal for measures with a matching image
  // do not reset Mcal for ubercal images unless explicitly requested
  if (measure->dbFlags & ID_MEAS_PHOTOM_UBERCAL) {
    if (!KEEP_UBERCAL) {
      measure->McalPSF  = 0.0;
      measure->McalAPER = 0.0;
      measure->dbFlags &= ~ID_MEAS_PHOTOM_UBERCAL;
    } 
  } else {
    if (getImageEntry (Nmeasure, Ncat) >= 0) {
      measure->McalPSF  = 0.0;
      measure->McalAPER = 0.0;
    }
  }
  if (RESET_FLATCORR) {
    measure->Mflat = 0.0;
  }
  measure->dbFlags &= ~photomBitsMeasure;
}

// Used in load_images.c.  Applied to the subset of selected images used for the calibration.
// This will be only those images in the selected region and only those with the active
// photcode *unless* -use-all-images is selected.  
void ResetImages (Image *subset, off_t Nsubset) {

  // reset image values as needed.  always allow 'few' images to succeed, if possible (new
  // images / detections may have been added

  for (off_t i = 0; i < Nsubset; i++) {
    // reset these bits regardless (we will re-determine)
    subset[i].flags &= ~(ID_IMAGE_PHOTOM_POOR | ID_IMAGE_PHOTOM_FEW);
    if (RESET) {
      if (RESET_ZEROPTS) {
	if (!KEEP_UBERCAL || !(subset[i].flags & ID_IMAGE_PHOTOM_UBERCAL)) {
	  subset[i].McalPSF  = 0.0;
	  subset[i].McalAPER = 0.0;
	  subset[i].dMcal    = NAN;
	  subset[i].flags   &= ~ID_IMAGE_PHOTOM_UBERCAL;
	}
      }
      subset[i].flags &= ~(ID_IMAGE_MOSAIC_POOR | ID_IMAGE_NIGHT_POOR);
      subset[i].flags &= ~(ID_IMAGE_IMAGE_PHOTCAL | ID_IMAGE_MOSAIC_PHOTCAL | ID_IMAGE_TGROUP_PHOTCAL);
      subset[i].ubercalDist = 1000;
    }
  }
}

// Used in setMrelFinal.c
void ResetAverageAndMeasure (Catalog *catalog) {

  int Nsecfilt = GetPhotcodeNsecfilt ();

  // XXX I should really deprecate the concept of applying the average 
  // calculation to a limited set of photcodes.  
  // for now, just do all photcodes here (
  // as it stands, only stacks are limited by photcode; mean exp and forced warp 
  // are applied to all Nsecfilt

  for (off_t i = 0; i < catalog->Naverage; i++) {
    if (STAGES & STAGE_CHIP) {
      catalog->average[i].psfQF     = NAN;	// force recalculation in setMrelCatalog
      catalog->average[i].psfQFperf = NAN;	// force recalculation in setMrelCatalog
      catalog->average[i].stargal   = NAN;	// force recalculation in setMrelCatalog
      catalog->average[i].photFlagsUpper = 0;	// reset (will be re-calculated)
      catalog->average[i].photFlagsLower = 0;	// reset (will be re-calculated)
    }
    
    if (STAGES & STAGE_WARP) {
      catalog->average[i].NwarpOK        = 0;	// reset (will be re-calculated)
    }

    // RESET_FLATCORR independent of RESET?

    if (!RESET) continue;

    catalog->average[i].flags    &= ~photomBitsAverage; // reset all photometry bits (but not astrom)

    // Reset secfilt values only for the active photcodes
    for (int Ns = 0; Ns < Nphotcodes; Ns++) {
      
      int thisCode = photcodes[Ns][0].code;
      int Nsec = GetPhotcodeNsec(thisCode);
      off_t N = Nsecfilt*i+Nsec;
      
      dvo_secfilt_init (&catalog->secfilt[N], SECFILT_RESET_ALL);
    }

    // Reset measure values only for the active photcodes
    
    off_t m = catalog->average[i].measureOffset;
    
    for (off_t j = 0; j < catalog->average[i].Nmeasure; j++, m++) {
      
      /* select measurements for active photcodes */
      PhotCode *code = GetPhotcodebyCode (catalog->measure[m].photcode);
      if (!code) continue;
      // photcode exists, but is it one of the active ones?
      int found = FALSE;
      for (int Ns = 0; !found && (Ns < Nphotcodes); Ns++) {
	if (code->equiv != photcodes[Ns][0].code) continue;
	found = TRUE;
      }
      if (!found) continue; 

      // reset the Mflat value if requested
      if (RESET_FLATCORR) {
	catalog->measure[m].Mflat = 0.0;
      }
      
      /* select measurements by time */
      if (TimeSelect) {
	if (catalog->measure[m].t < TSTART) continue;
	if (catalog->measure[m].t > TSTOP) continue;
      }
	
      // only reset Mcal for measures with a matching image
      // do not reset Mcal for ubercal images unless explicitly requested

      if (catalog->measure[m].dbFlags & ID_MEAS_PHOTOM_UBERCAL) {
	if (!KEEP_UBERCAL) {
	  catalog->measure[m].McalPSF  = 0.0;
	  catalog->measure[m].McalAPER = 0.0;
	  catalog->measure[m].dbFlags &= ~ID_MEAS_PHOTOM_UBERCAL;
	} 
      } else {
	if (RESET_ZEROPTS && (getImageEntry (m, 0) >= 0)) {
	  catalog->measure[m].McalPSF  = 0.0;
	  catalog->measure[m].McalAPER = 0.0;
	}
      }
      catalog->measure[m].dbFlags &= ~photomBitsMeasure;
    }
  }
}



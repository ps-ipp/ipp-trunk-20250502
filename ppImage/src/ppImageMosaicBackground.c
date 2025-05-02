#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "ppImage.h"


bool ppImageMosaicBackground(pmConfig *config, const ppImageOptions *options) {
  assert(config);
  assert(options);
  
  if (options->doBackgroundContinuity) {
    bool status;
    pmFPAfile *in = psMetadataLookupPtr(&status, config->files, "PPIMAGE.INPUT");
    if (!status) {
      psErrorStackPrint(stderr, "Can't find required I/O file!\n");
      exit(EXIT_FAILURE);
    }
    pmFPAfile *out = psMetadataLookupPtr(&status, config->files, "PPIMAGE.BACKMDL");
    if (!status) {
      psErrorStackPrint(stderr, "Can't find required I/O file!\n");
      exit(EXIT_FAILURE);
    }
    //pmReadout *model = pmFPAGenerateReadout(config, view, psphotGetFilerule("PSPHOT.BACKMDL"), inFPA, binning, index);
    
    pmFPAview *view = pmFPAviewAlloc(0);
    //    pmFPAAddSourceFromView(out->fpa, view, out->format);
    //    psFree(view);
    if (!pmFPAfileIOChecks(config, view, PM_FPA_BEFORE)) {
      //	ESCAPE("load failure for Chip");
    }

    pmChip *chip;
    
    if (!pmPatternContinuityBackground(in,out,options->patternCellBG,options->patternCellMean,
				       options->maskValue, options->darkMask, options->patternContinuityEdgeWidth)) {
      // Free things?
      psFree(view);
      return(false);
    }

    // Write out output models
    psFree(view);
    view = pmFPAviewAlloc(0);
    while ((chip = pmFPAviewNextChip(view, out->fpa, 1)) != NULL) {
      pmCell *cell;
      while ((cell = pmFPAviewNextCell(view, out->fpa, 1)) != NULL) {
	pmReadout *readout;
	while ((readout = pmFPAviewNextReadout(view, out->fpa, 1)) != NULL) {
	  if (!pmFPAfileIOChecks (config, view, PM_FPA_AFTER)) {
	    psError(PS_ERR_UNKNOWN, false, "I/O failure in ppImageMosaicBackground");
	    psFree(view);
	    return(false);
	  }
	}
	if (!pmFPAfileIOChecks(config, view, PM_FPA_AFTER)) {
	  psError(PS_ERR_UNKNOWN, false, "I/O failure in ppImageMosaicBackground");
	  psFree(view);
	  return(false);
	}
      }
      if (!pmFPAfileIOChecks(config, view, PM_FPA_AFTER)) {
	psError(PS_ERR_UNKNOWN, false, "I/O failure in ppImageMosaicBackground");
	psFree(view);
	return(false);
      }
    }
    if (!pmFPAfileIOChecks(config, view, PM_FPA_AFTER)) {
      psError(PS_ERR_UNKNOWN, false, "I/O failure in ppImageMosaicBackground");
      psFree(view);
      return(false);
    }
    psFree(view);
  }

  return(true);
}

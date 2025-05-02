#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "ppImage.h"


bool ppImageMaskStats(pmConfig *config, pmFPAview *view, psMetadata *stats)
{
  PS_ASSERT_PTR_NON_NULL(view, false);
  PS_ASSERT_PTR_NON_NULL(config, false);

  if (!stats) {
      return true;
  }

  bool status;

  pmFPAfile *input = psMetadataLookupPtr(&status, config->files, "PPIMAGE.CHIP");
  if (!status) {
    psError(PS_ERR_UNEXPECTED_NULL, true, "PPIMAGE.CHIP file is not defined");
    return(false);
  }

  psMetadata *recipe = psMetadataLookupMetadata(&status, config->recipes, "PPIMAGE");

  psU16 staticMaskVal = psMetadataLookupU32(&status, recipe, "MASKSTAT.STATIC");
  psU16 magicMaskVal = psMetadataLookupU32(&status, recipe, "MASKSTAT.MAGIC");
  psU16 dynamicMaskVal = psMetadataLookupU32(&status, recipe, "MASKSTAT.DYNAMIC");
  psU16 advisoryMaskVal = psMetadataLookupU32(&status, recipe, "MASKSTAT.ADVISORY");

  psS32 Npix_valid = 0;
  psS32 Npix_static = 0;
  psS32 Npix_magic = 0;
  psS32 Npix_dynamic = 0;
  psS32 Npix_advisory = 0;

  pmChip *chip = pmFPAviewThisChip(view, input->fpa);  // Chip of interest
  if (!chip) {
    psError(PS_ERR_UNEXPECTED_NULL, false, "Unable to find chip");
    return(false);
  }
  if (chip->cells->n == 0) {
    psWarning("Chip has no cells");
    return(true);
  }
  if (chip->cells->n > 1) {
    psWarning("Chip has %ld cells; only the first will be processed", chip->cells->n);
  }
  pmCell *cell = chip->cells->data[0]; // Cell of interest
  if (!cell || !cell->process || !cell->file_exists) {
    // Nothing to process
    return(true);
  }
  if (cell->readouts->n == 0) {
    psWarning("Cell has no readouts");
    return(true);
  }
  if (cell->readouts->n > 1) {
    psWarning("Cell has %ld readouts; only the first will be processed", cell->readouts->n);
  }
  pmReadout *readout = cell->readouts->data[0]; // Readout of interest;
  if (!readout || !readout->data_exists) {
    // Nothing to process
    return(true);
  }

  psImage *mask = readout->mask;  // Mask of interest;
  psWarning("In ppImageMaskStats: %d %ld\n",Npix_valid, (long) mask);

  if (!pmSingleImageMaskStats(mask,&Npix_valid,&Npix_static,&Npix_magic,
                              &Npix_dynamic,&Npix_advisory,
                              staticMaskVal,magicMaskVal,
                              dynamicMaskVal,advisoryMaskVal)) {
    psError(PS_ERR_UNKNOWN, false, "Unable to calculate masks for readout.");
    return(false);
  }
  psMetadataAddS32(stats, PS_LIST_TAIL,"MASKFRAC_NPIX", 0,
                   "Number of valid pixels", Npix_valid);
  psMetadataAddF32(stats,PS_LIST_TAIL, "MASKFRAC_STATIC", 0,
                   "Fraction of pixels statically masked", (float) Npix_static / Npix_valid);
  psMetadataAddF32(stats,PS_LIST_TAIL, "MASKFRAC_DYNAMIC", 0,
                   "Fraction of pixels dynamically masked", (float) Npix_dynamic / Npix_valid);
  psMetadataAddF32(stats,PS_LIST_TAIL, "MASKFRAC_MAGIC", 0,
                   "Fraction of pixels magically masked", (float) Npix_magic / Npix_valid);
  psMetadataAddF32(stats,PS_LIST_TAIL, "MASKFRAC_ADVISORY", 0,
                   "Fraction of pixels masked as an advisory", (float) Npix_advisory / Npix_valid);
  psWarning("In ppImageMaskStats: %d %f %f %f %f\n",Npix_valid, (float) Npix_static / Npix_valid,
            (float) Npix_dynamic / Npix_valid, (float) Npix_magic / Npix_valid,
            (float) Npix_advisory / Npix_valid);

  if ((Npix_valid == 0)||(Npix_static + Npix_dynamic >= Npix_valid)) {
    if (psMetadataLookupS32(NULL, stats, "QUALITY") == 0) {
      psMetadataAddS32(stats, PS_LIST_TAIL, "QUALITY", PS_META_REPLACE, "No good pixels in image.", PPIMAGE_ERR_NO_PIXELS);
    }
  }
  
  return(true);
}

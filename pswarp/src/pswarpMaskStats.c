#include "pswarp.h"

bool pswarpMaskStats(const pmReadout *readout, psMetadata *stats, const pmConfig *config)
{
  PS_ASSERT_PTR_NON_NULL(readout, false);
  PS_ASSERT_PTR_NON_NULL(config, false);

  if (!stats || !readout || !readout->data_exists) {
    // Nothing to process
    return(true);
  }

  bool status;
  psMetadata *recipe = psMetadataLookupMetadata(&status, config->recipes, PSWARP_RECIPE);
  psU16 staticMaskVal = psMetadataLookupU32(&status, recipe, "MASKSTAT.STATIC");
  psU16 magicMaskVal = psMetadataLookupU32(&status, recipe, "MASKSTAT.MAGIC");
  psU16 dynamicMaskVal = psMetadataLookupU32(&status, recipe, "MASKSTAT.DYNAMIC");
  psU16 advisoryMaskVal = psMetadataLookupU32(&status, recipe, "MASKSTAT.ADVISORY");

  psS32 Npix_valid = 0;
  psS32 Npix_static = 0;
  psS32 Npix_magic = 0;
  psS32 Npix_dynamic = 0;
  psS32 Npix_advisory = 0;

  psImage *mask = readout->mask;  // Mask of interest;
  if (!pmSingleImageMaskStats(mask,&Npix_valid,&Npix_static,&Npix_magic,
                              &Npix_dynamic,&Npix_advisory,
                              staticMaskVal,magicMaskVal,
                              dynamicMaskVal,advisoryMaskVal)) {
    psError(PS_ERR_UNKNOWN, false, "Unable to calculate masks for readout.");
    return false;
  }

  // XXX with multiple inputs (eg, output stacks -> exposure), these only represent the last input
  psMetadataAddS32(stats, PS_LIST_TAIL,"MASKFRAC_NPIX",     PS_META_REPLACE, "Number of valid pixels", Npix_valid);
  psMetadataAddF32(stats,PS_LIST_TAIL, "MASKFRAC_STATIC",   PS_META_REPLACE, "Fraction of pixels statically masked", (float) Npix_static / Npix_valid);
  psMetadataAddF32(stats,PS_LIST_TAIL, "MASKFRAC_DYNAMIC",  PS_META_REPLACE, "Fraction of pixels dynamically masked", (float) Npix_dynamic / Npix_valid);
  psMetadataAddF32(stats,PS_LIST_TAIL, "MASKFRAC_MAGIC",    PS_META_REPLACE, "Fraction of pixels magically masked", (float) Npix_magic / Npix_valid);
  psMetadataAddF32(stats,PS_LIST_TAIL, "MASKFRAC_ADVISORY", PS_META_REPLACE, "Fraction of pixels masked as an advisory", (float) Npix_advisory / Npix_valid);
  return true;
}

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <pslib.h>
#include <psmodules.h>

#include "ppSub.h"


bool ppSubMaskStats(pmConfig *config, pmFPAview *view, psMetadata *stats)
{
  PS_ASSERT_PTR_NON_NULL(view, false);
  PS_ASSERT_PTR_NON_NULL(config, false);

  pmReadout *readout = pmFPAfileThisReadout(config->files, view, "PPSUB.OUTPUT");
  if (!stats || !readout || !readout->data_exists) {
      // Nothing to process
      return(true);
  }

  bool status;

  psMetadata *recipe = psMetadataLookupMetadata(&status, config->recipes, PPSUB_RECIPE);
  psImageMaskType staticMaskVal = psMetadataLookupImageMask(&status, recipe, "MASKSTAT.STATIC");
  psImageMaskType magicMaskVal = psMetadataLookupImageMask(&status, recipe, "MASKSTAT.MAGIC");
  psImageMaskType dynamicMaskVal = psMetadataLookupImageMask(&status, recipe, "MASKSTAT.DYNAMIC");
  psImageMaskType advisoryMaskVal = psMetadataLookupImageMask(&status, recipe, "MASKSTAT.ADVISORY");

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
  return(true);
}

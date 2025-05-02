#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <assert.h>

#include <pslib.h>

#include "pmHDU.h"
#include "pmFPA.h"
#include "pmFPAview.h"
#include "pmFPAfile.h"
#include "pmFPAfileIO.h"
#include "pmFPAAstrometry.h"

#include "pmMaskStats.h"

#define ESCAPE { \
        psError(psErrorCodeLast(), false, "I/O failure in pmMaskStats"); \
        psFree (view);                                                  \
        return false;                                                   \
    }




bool pmFPAMaskStats(pmFPA *fpa, pmConfig *config) {
  PS_ASSERT_PTR_NON_NULL(fpa, false);
  PS_ASSERT_PTR_NON_NULL(config, false);

  bool status;

  psImageMaskType staticMaskVal = psMetadataLookupImageMask(&status, config->recipes, "MASKSTAT.STATIC");
  psImageMaskType magicMaskVal = psMetadataLookupImageMask(&status, config->recipes, "MASKSTAT.MAGIC");
  psImageMaskType dynamicMaskVal = psMetadataLookupImageMask(&status, config->recipes, "MASKSTAT.DYNAMIC");
  psImageMaskType advisoryMaskVal = psMetadataLookupImageMask(&status, config->recipes, "MASKSTAT.ADVISORY");

  psS32 Npix_valid = 0;
  psS32 Npix_static = 0;
  psS32 Npix_magic = 0;
  psS32 Npix_dynamic = 0;
  psS32 Npix_advisory = 0;

  pmChip *chip;
  pmCell *cell;
  pmReadout *readout;
  pmFPAview *view = pmFPAviewAlloc(0);
  while ((chip = pmFPAviewNextChip(view, fpa, 1)) != NULL) {
    if (!chip->process || !chip->file_exists) { continue; }
    if (!chip->fromFPA) { continue; }
    if (!pmFPAfileIOChecks (config, view, PM_FPA_BEFORE)) ESCAPE;

    while ((cell = pmFPAviewNextCell(view, fpa, 1)) != NULL) {
      if (!cell->process || !cell->file_exists) {continue; }
      while ((readout = pmFPAviewNextReadout(view, fpa, 1)) != NULL) {
        if (!readout->data_exists) {continue; }

        psImage *mask = readout->mask;
        if (!pmSingleImageMaskStats(mask,&Npix_valid,&Npix_static,&Npix_magic,
                                    &Npix_dynamic,&Npix_advisory,
                                    staticMaskVal,magicMaskVal,
                                    dynamicMaskVal,advisoryMaskVal)) {
          psError(PS_ERR_UNKNOWN, false, "Unable to calculate masks for readout.");
          return(false);
        }
        psMetadataAddS32(readout->analysis, PS_LIST_TAIL,"MASKFRAC_NPIX", 0,
                         "Number of valid pixels", Npix_valid);
        psMetadataAddF32(readout->analysis,PS_LIST_TAIL, "MASKFRAC_STATIC", 0,
                         "Fraction of pixels statically masked", (float) Npix_static / Npix_valid);
        psMetadataAddF32(readout->analysis,PS_LIST_TAIL, "MASKFRAC_DYNAMIC", 0,
                         "Fraction of pixels dynamically masked", (float) Npix_dynamic / Npix_valid);
        psMetadataAddF32(readout->analysis,PS_LIST_TAIL, "MASKFRAC_MAGIC", 0,
                         "Fraction of pixels magically masked", (float) Npix_magic / Npix_valid);
        psMetadataAddF32(readout->analysis,PS_LIST_TAIL, "MASKFRAC_ADVISORY", 0,
                         "Fraction of pixels masked as an advisory", (float) Npix_advisory / Npix_valid);
      }
    }
  }
  return(true);
}



bool pmSingleImageMaskStats(psImage *mask,
                            psS32 *Npix_valid, psS32 *Npix_static, psS32 *Npix_magic,
                            psS32 *Npix_dynamic, psS32 *Npix_advisory,
                            psImageMaskType staticMaskVal, psImageMaskType magicMaskVal,
                            psImageMaskType dynamicMaskVal, psImageMaskType advisoryMaskVal) {
  PS_ASSERT_IMAGE_NON_NULL(mask, false);
  *Npix_valid = 0;
  *Npix_static = 0;
  *Npix_magic = 0;
  *Npix_dynamic = 0;
  *Npix_advisory = 0;

  psImageMaskType **maskData = mask->data.PS_TYPE_IMAGE_MASK_DATA;
  for (int i = 0; i < mask->numRows; i++) {
    for (int j = 0; j < mask->numCols; j++) {
      *Npix_valid += 1;
      if (maskData[i][j] & staticMaskVal) {
        *Npix_static += 1;
        continue;
      }
      if (maskData[i][j] & dynamicMaskVal) {
        *Npix_dynamic += 1;
        continue;
      }
      if (maskData[i][j] & magicMaskVal) {
        *Npix_magic += 1;
        continue;
      }
      if (maskData[i][j] & advisoryMaskVal) {
        *Npix_advisory += 1;
        continue;
      }
    }
  }
  return(true);
}


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define PPIMAGE_BURNTOOL_DEBUG 0

#include "ppImage.h"

bool ppImageBurntoolMask (pmConfig *config, ppImageOptions *options, pmFPAview *view,pmReadout *mask) {
  bool status = true;
  int burntool_cell;
  /* Find input filename */
  pmFPAfile *inputFile = psMetadataLookupPtr(&status , config->files, "PPIMAGE.INPUT");
  if (!status) {
    psError(PS_ERR_IO,false, "Unable to identify inputFile");
    return(false);
  }
  psFits *fits = inputFile->fits;

  /* Read input header, and find the burntool data table. */
  if (!psFitsMoveExtName(fits,"burntool_areas")) {
    psError(PS_ERR_IO,false, "Unable to find extension burntool_areas");
    return(false);
  }
  long Nrows = psFitsTableSize(fits);
  long row = 0;

  psLogMsg ("ppImageBurntoolMask", 4, "Inside burntool mask %ld", Nrows);

  /* The new burntool tables and method coming into play in 2022 has a slightly different format (an extra column). Need to differentiate */
  psS16 BURNTOOL_STATE_GOOD = psMetadataLookupS16(NULL, config->camera, "BURNTOOL.STATE.GOOD");

  /* Redirects and Memory juggling. */
  view->readout = 0;
  psImage *image = mask->mask;

  /* Set the maskValue from the recipes. */
  psImageMaskType maskValue = options->burntoolMask;
#if PPIMAGE_BURNTOOL_DEBUG
  psLogMsg("ppImageBurntoolMask", 4, "Status: %ld %d\n",Nrows,maskValue);
#endif

  burntool_cell = view->cell;
  burntool_cell = (view->cell % 8) * 8 + (view->cell - (view->cell % 8)) / 8;
  psLogMsg("ppImageBurntoolMask", 4, "Cell mapping: %d %d %d\n",view->cell,burntool_cell,-1);
  for (row = 0; row < Nrows; row++) {
    psMetadata *rowMD = psFitsReadTableRow(fits,row);

    if (psMetadataLookupS32(&status,rowMD,"cell") == burntool_cell) {
      if (((options->burntoolTrails & 0x01)&&(psMetadataLookupS32(&status,rowMD,"func") == 4))||
          (((options->burntoolTrails & 0x02)&&(psMetadataLookupS32(&status,rowMD,"up") == 1))||
           ((options->burntoolTrails & 0x04)&&(psMetadataLookupS32(&status,rowMD,"up") == 0)))) {
        /*       If the fit fails, burntool reports zero here.  This
                 signifies that it expected to see a trail (else why
                 fit) but did not find it when it attempted to
                 correct. */
#if PPIMAGE_BURNTOOL_DEBUG
        psLogMsg ("ppImageBurntoolMask", 4, "Masking! %d (%d %d %d) %d %d",
                  psMetadataLookupS32(&status,rowMD,"cell"),
                  ((options->burntoolTrails & 0x0001)&&(psMetadataLookupS32(&status,rowMD,"nfit") == 0)),
                  ((options->burntoolTrails & 0x02)&&(psMetadataLookupS32(&status,rowMD,"up") == 1)),
                  ((options->burntoolTrails & 0x04)&&(psMetadataLookupS32(&status,rowMD,"up") == 0)),
                  options->burntoolTrails,
                  maskValue
                  );
#endif

        /* do separate masking strategy for old (14 and lower) and new burntool tables*/
        if (BURNTOOL_STATE_GOOD >= 15) {
          for (int i = psMetadataLookupS32(&status,rowMD,"sxfit");
             i <= psMetadataLookupS32(&status,rowMD,"exfit");
             i++) {

            if (psMetadataLookupS32(&status,rowMD,"up") == 0) {
              for (int j = psMetadataLookupS32(&status,rowMD,"eyfit"); j <= psMetadataLookupS32(&status,rowMD,"y0"); j++) {
                #if PPIMAGE_BURNTOOL_DEBUG
                psLogMsg("ppImageBurntoolMask", 4, "Noisy!: %d %d %d %d\n",
                       i,j,image->data.PS_TYPE_IMAGE_MASK_DATA[j][i],maskValue);
                #endif
                image->data.PS_TYPE_IMAGE_MASK_DATA[j][i] |= maskValue;
              }
            } else {
              for (int j = psMetadataLookupS32(&status,rowMD,"sy"); j < psMetadataLookupS32(&status,rowMD,"eyfit") ; j++) {
                #if PPIMAGE_BURNTOOL_DEBUG
                psLogMsg("ppImageBurntoolMask", 4, "Noisy!: %d %d %d %d\n",
                       i,j,image->data.PS_TYPE_IMAGE_MASK_DATA[j][i],maskValue);
                #endif
                image->data.PS_TYPE_IMAGE_MASK_DATA[j][i] |= maskValue;
              }
            }
          }
        } else {
          for (int i = psMetadataLookupS32(&status,rowMD,"sxfit");
             i <= psMetadataLookupS32(&status,rowMD,"exfit");
             i++) {

            if (psMetadataLookupS32(&status,rowMD,"up") == 0) {
              for (int j = 0; j <= psMetadataLookupS32(&status,rowMD,"y1p"); j++) {
                #if PPIMAGE_BURNTOOL_DEBUG
                psLogMsg("ppImageBurntoolMask", 4, "Noisy!: %d %d %d %d\n",
                       i,j,image->data.PS_TYPE_IMAGE_MASK_DATA[j][i],maskValue);
                #endif
                image->data.PS_TYPE_IMAGE_MASK_DATA[j][i] |= maskValue;
              }
            }
            else {
              for (int j = psMetadataLookupS32(&status,rowMD,"y1m"); j < image->numRows ; j++) {
                #if PPIMAGE_BURNTOOL_DEBUG
                psLogMsg("ppImageBurntoolMask", 4, "Noisy!: %d %d %d %d\n",
                       i,j,image->data.PS_TYPE_IMAGE_MASK_DATA[j][i],maskValue);
                #endif
                image->data.PS_TYPE_IMAGE_MASK_DATA[j][i] |= maskValue;
              }
            }
          }

        }


      }
    }
    psFree(rowMD);
  }

  return(status);
}

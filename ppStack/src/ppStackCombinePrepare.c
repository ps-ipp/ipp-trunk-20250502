#include "ppStack.h"

bool ppStackCombinePrepare(const char *outName, const char *expName, const char *bkgName,
                           ppStackFileList files, ppStackThreadData *stack,
                           ppStackOptions *options, pmConfig *config)
{
    psAssert(stack, "Require stack");
    psAssert(options, "Require options");
    psAssert(config, "Require configuration");

    int row0, col0;                 // Offset for readout
    int numCols, numRows;           // Size of readout
    ppStackThread *thread = stack->threads->data[0]; // Representative thread
    if (!pmReadoutStackSetOutputSize(&col0, &row0, &numCols, &numRows, thread->readouts)) {
        psError(PPSTACK_ERR_ARGUMENTS, false, "problem setting output readout size.");
        return false;
    }

    pmFPAfileActivate(config->files, false, NULL);
    ppStackFileActivation(config, files, true);
    pmFPAview *view = ppStackFilesIterateDown(config); // View to readout
    if (!view) {
        return false;
    }

    pmCell *cell = pmFPAfileThisCell(config->files, view, outName); // Output cell
    options->outRO = pmReadoutAlloc(cell); // Output readout

    if (expName) {
      pmCell *expCell = pmFPAfileThisCell(config->files, view, expName); // Exposure cell
      options->expRO = pmReadoutAlloc(expCell); //Output readout
    }

    int bkg_r0 = 0,bkg_c0 = 0;
    int bkg_nC = 0,bkg_nR = 0;
    if (bkgName) {
      ppStackFileActivationSingle(config, PPSTACK_FILES_BKG, true, 0);
      pmFPAfile *file = pmFPAfileSelectSingle(config->files, "PPSTACK.INPUT.BKGMODEL", 0);
      if (!file)  {
        psError(PPSTACK_ERR_ARGUMENTS, false, "Output background model selected but no inputs provided.");
        return false;
      }
      pmFPAview *view = ppStackFilesIterateDown(config);
      pmReadout *ro = pmFPAviewThisReadout(view,file->fpa);

      bkg_r0 = ro->image->row0;
      bkg_c0 = ro->image->col0;
      bkg_nC = ro->image->numCols;
      bkg_nR = ro->image->numRows;
      pmCell *bkgCell = pmFPAfileThisCell(config->files, view, bkgName); // Bkg cell
      
      options->bkgRO = pmReadoutAlloc(bkgCell); // BKG readout
      options->bkgRO->parent->parent->parent->hdu = pmHDUAlloc(NULL);

      if (!options->bkgRO->parent->parent->parent->hdu) {
	fprintf(stderr,"failed to generate a HDU for this thing.\n");
      }
      options->bkgRO->parent->parent->parent->hdu->header = psMetadataCopy(options->bkgRO->parent->parent->parent->hdu->header,
									   ro->parent->parent->parent->hdu->header);

      options->bkgRO->parent->concepts = psMetadataCopy(options->bkgRO->parent->concepts,
							ro->parent->concepts);
      options->bkgRO->parent->parent->concepts = psMetadataCopy(options->bkgRO->parent->parent->concepts,
								ro->parent->parent->concepts);
      options->bkgRO->parent->parent->parent->concepts = psMetadataCopy(options->bkgRO->parent->parent->parent->concepts,
									ro->parent->parent->parent->concepts);

    } else {
      options->bkgRO = NULL;
    }

    psFree(view);

    psMetadata *recipe = psMetadataLookupMetadata(NULL, config->recipes, PPSTACK_RECIPE); // ppStack recipe
    psAssert(recipe, "We've thrown an error on this before.");
    psString maskBadStr = psMetadataLookupStr(NULL, recipe, "MASK.BAD"); // Name of bits to mask for bad
    psImageMaskType maskBad = pmConfigMaskGet(maskBadStr, config); // Bits to mask for bad pixels

    if (!pmReadoutStackDefineOutput(options->outRO, col0, row0, numCols, numRows, true, true, maskBad)) {
        psError(PPSTACK_ERR_ARGUMENTS, false, "Unable to prepare output.");
        return false;
    }

    if (expName) {
      if (!pmReadoutStackDefineOutput(options->expRO, col0, row0, numCols, numRows, true, true, 0)) {
        psError(PPSTACK_ERR_ARGUMENTS, false, "Unable to prepare output.");
        return false;
      }
    }

    if (bkgName) {
      if (!pmReadoutStackDefineOutput(options->bkgRO, bkg_c0, bkg_r0, bkg_nC, bkg_nR, false, false, 0)) {
        psError(PPSTACK_ERR_ARGUMENTS, false, "Unable to prepare output.");
        return false;
      }
    }

    return true;
}

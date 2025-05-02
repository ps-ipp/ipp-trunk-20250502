#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

# include "ppImage.h"

// XXX clean up error checks: return NULL, not psAbort
ppImageOptions *ppFocusParseCamera (pmConfig *config, int entry) {

    bool status = false;

    // the first input image defines the camera, and all recipes and options that follow
    // select only the first file from the INPUT array
    pmFPAfile *input = pmFPAfileDefineSingleFromArgs (&status, config, "PPIMAGE.INPUT", "INPUT", entry);
    if (!status) {
        psError(PS_ERR_IO, false, "Failed to build FPA from PPIMAGE.INPUT");
        return NULL;
    }

    // parse the options from the metadata format to the ppImageOptions structure
    ppImageOptions *options = ppImageOptionsParse (config);

    // the following are defined from the argument list, if given,
    // otherwise they revert to the config information or detrend database if specified
    // not all input or output images are used in a given recipe
    if (options->doBias) {
	if (!ppImageDefineFile (config, input->fpa, "PPIMAGE.BIAS", "BIAS", PM_FPA_FILE_IMAGE, PM_DETREND_TYPE_BIAS)) {
	    psError (PS_ERR_IO, false, "Can't find a bias image source");
	    return NULL;
	}
    }
    if (options->doDark) {
	if (!ppImageDefineFile (config, input->fpa, "PPIMAGE.DARK", "DARK", PM_FPA_FILE_IMAGE, PM_DETREND_TYPE_DARK)) {
	    psError (PS_ERR_IO, false, "Can't find a dark image source");
	    return NULL;
	}
    }
    if (options->doMask) {
	if (!ppImageDefineFile (config, input->fpa, "PPIMAGE.MASK", "MASK", PM_FPA_FILE_MASK, PM_DETREND_TYPE_MASK)) {
	    psError (PS_ERR_IO, false, "Can't find a mask image source");
	    return NULL;
	}
    }
    if (options->doFlat) {
	if (!ppImageDefineFile (config, input->fpa, "PPIMAGE.FLAT", "FLAT", PM_FPA_FILE_IMAGE, PM_DETREND_TYPE_FLAT)) {
	    psError (PS_ERR_IO, false, "Can't find a shutter image source");
	    return NULL;
	}
    }

    // the following files are output targets
    pmFPAfile *output = pmFPAfileDefineOutput (config, input->fpa, "PPIMAGE.OUTPUT");
    pmFPAfile *byChip = pmFPAfileDefineNewCamera (config, "PPIMAGE.OUTPUT.CHIP");
    pmFPAfile *byFPA1 = pmFPAfileDefineNewCamera (config, "PPIMAGE.OUTPUT.FPA1");
    pmFPAfile *byFPA2 = pmFPAfileDefineNewCamera (config, "PPIMAGE.OUTPUT.FPA2");

    // save any of these files?
    output->save = options->BaseFITS;
    byChip->save = options->ChipFITS;
    byFPA1->save = options->FPA1FITS;
    byFPA2->save = options->FPA2FITS;

    // output is used as a carrier: input to byChip
    output->freeLevel = PM_FPA_LEVEL_CHIP;

    // define the binned target files (which may just be carriers for some camera configurations)
    pmFPAfile *bin1 = pmFPAfileDefineFromFPA (config, byChip->fpa, options->xBin1, options->yBin1, "PPIMAGE.BIN1");
    pmFPAfile *bin2 = pmFPAfileDefineFromFPA (config, byChip->fpa, options->xBin2, options->yBin2, "PPIMAGE.BIN2");

    // bin1 and bin2 are used as carriers: input for byFPA1, byFPA2
    bin1->freeLevel = PM_FPA_LEVEL_FPA;
    bin2->freeLevel = PM_FPA_LEVEL_FPA;

    pmFPAfile *jpg1 = pmFPAfileDefineOutput (config, byFPA1->fpa, "PPIMAGE.JPEG1");
    pmFPAfile *jpg2 = pmFPAfileDefineOutput (config, byFPA2->fpa, "PPIMAGE.JPEG2");

    // XXX we could potentially not define these pmFPAfiles if no output is requested...
    bin1->save = options->Bin1FITS;
    bin2->save = options->Bin2FITS;
    jpg1->save = options->Bin1JPEG;
    jpg2->save = options->Bin2JPEG;

    // Chip selection: turn on only the chips specified (pass status to suppress missing-key log msg)
    char *chipLine = psMetadataLookupStr(&status, config->arguments, "CHIP_SELECTIONS");
    psArray *chips = psStringSplitArray (chipLine, ",", false);
    if (chips->n > 0) {
        pmFPASelectChip (input->fpa, -1, true); // deselect all chips
        for (int i = 0; i < chips->n; i++) {
            int chipNum = atoi(chips->data[i]);
            if (! pmFPASelectChip(input->fpa, chipNum, false)) {
                psError(PS_ERR_IO, false, "Chip number %d doesn't exist in camera.\n", chipNum);
                return false;
            }
        }
    }
    psFree (chips);

    return (options);
}

// remove from config all pmFPAfiles which could have been added (ignore missing entries)
void ppFocusDropCamera (pmConfig *config) {

    psMetadataRemoveKey (config->files, "PPIMAGE.INPUT");

    psMetadataRemoveKey (config->files, "PPIMAGE.BIAS");
    psMetadataRemoveKey (config->files, "PPIMAGE.DARK");
    psMetadataRemoveKey (config->files, "PPIMAGE.MASK");
    psMetadataRemoveKey (config->files, "PPIMAGE.FLAT");

    psMetadataRemoveKey (config->files, "PPIMAGE.OUTPUT");
    psMetadataRemoveKey (config->files, "PPIMAGE.OUTPUT.CHIP");
    psMetadataRemoveKey (config->files, "PPIMAGE.OUTPUT.FPA1");
    psMetadataRemoveKey (config->files, "PPIMAGE.OUTPUT.FPA2");

    psMetadataRemoveKey (config->files, "PPIMAGE.BIN1");
    psMetadataRemoveKey (config->files, "PPIMAGE.BIN2");

    psMetadataRemoveKey (config->files, "PPIMAGE.JPEG1");
    psMetadataRemoveKey (config->files, "PPIMAGE.JPEG2");

    psMetadataRemoveKey (config->files, "PSPHOT.INPUT");
    psMetadataRemoveKey (config->files, "PSPHOT.OUTPUT");
    psMetadataRemoveKey (config->files, "PSPHOT.RESID");
    psMetadataRemoveKey (config->files, "PSPHOT.BACKMDL");
    psMetadataRemoveKey (config->files, "PSPHOT.BACKGND");
    psMetadataRemoveKey (config->files, "PSPHOT.BACKSUB");
    psMetadataRemoveKey (config->files, "PSPHOT.PSF.LOAD");
    psMetadataRemoveKey (config->files, "PSPHOT.PSF.SAVE");

    return;
}


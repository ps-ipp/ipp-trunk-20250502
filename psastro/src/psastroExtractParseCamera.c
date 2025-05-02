/** @file psastroExtractParseCamera.c
 *
 *  @brief
 *
 *  @ingroup psastroExtract
 *
 *  @author IfA
 *  @version $Revision: 1.3 $ 
 *  @date $Date: 2009-02-07 02:03:34 $
 *  Copyright 2009 Institute for Astronomy, University of Hawaii
 */

# include "psastroInternal.h"

bool psastroExtractParseCamera (pmConfig *config) {

    bool status = false;

    // the input image(s) are required arguments; they define the camera
    status = false;
    pmFPAfile *input = pmFPAfileDefineFromArgs(&status, config, "PSASTRO.EXTRACT.INPUT", "INPUT");
    if (!input || !status) {
        psError(PSASTRO_ERR_CONFIG, false, "Failed to build FPA from PSASTRO.EXTRACT.INPUT");
        return false;
    }

    // the input image(s) are required arguments; they define the camera
    status = false;
    pmFPAfile *astrom = pmFPAfileDefineFromArgs(&status, config, "PSASTRO.EXTRACT.ASTROM", "ASTROM");
    if (!astrom || !status) {
        psError (PS_ERR_UNKNOWN, false, "failed to load astrometry definition");
        return NULL;
    }

# if (0)    
    // XXX for now, don't use the pmFPAfile methods
    // set up an output fpa structure & file based on the last input file
    pmFPAfile *output = pmFPAfileDefineOutput (config, input->fpa, "PSASTRO.OUT.MODEL");
    if (!output) {
	psError(PSASTRO_ERR_CONFIG, false, "Failed to build FPA for PSASTRO.OUT.MODEL from input");
	return false;
    }
    output->save = true;
# endif

    // Chip selection: turn on only the chips specified (option is not required)
    char *chipLine = psMetadataLookupStr(&status, config->arguments, "CHIP_SELECTIONS"); 
    psArray *chips = psStringSplitArray (chipLine, ",", false);
    if (chips->n > 0) {
	pmFPASelectChip (input->fpa, -1, true); // deselect all chips
	pmFPASelectChip (astrom->fpa, -1, true); // deselect all chips
	for (int i = 0; i < chips->n; i++) {
	    int chipNum = atoi(chips->data[i]);
	    if (! pmFPASelectChip(input->fpa, chipNum, false)) {
		psError(PSASTRO_ERR_CONFIG, true, "Chip number %d doesn't exist in camera.\n", chipNum);
		return false;
	    }
	    if (! pmFPASelectChip(astrom->fpa, chipNum, false)) {
		psError(PSASTRO_ERR_CONFIG, true, "Chip number %d doesn't exist in camera.\n", chipNum);
		return false;
	    }
        }
    }
    psFree (chips);

    psTrace("psastro", 1, "Done with psastroExtractParseCamera...\n");
    return true;
}

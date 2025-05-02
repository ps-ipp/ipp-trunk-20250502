/** @file psastroParseCamera.c
 *
 *  @brief
 *
 *  @ingroup psastro
 *
 *  @author IfA
 *  @version $Revision: 1.18 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-02-07 02:03:34 $
 *  Copyright 2009 Institute for Astronomy, University of Hawaii
 */

# include "psastroInternal.h"

bool psastroParseCamera (pmConfig *config) {

    bool status = false;

    // the input image(s) are required arguments; they define the camera
    pmFPAfile *input = pmFPAfileDefineFromArgs (&status, config, "PSASTRO.INPUT", "INPUT");
    if (!status) {
	psError(PSASTRO_ERR_CONFIG, false, "Failed to build FPA from PSASTRO.INPUT");
	return false;
    }

    // define the additional input/output files associated with psphot 
    if (!psastroDefineFiles (config, input)) {
	psError(PSASTRO_ERR_CONFIG, false, "Trouble defining the additional input/output files");
	return false;
    }

    // Chip selection: turn on only the chips specified (option is not required)
    char *chipLine = psMetadataLookupStr(&status, config->arguments, "CHIP_SELECTIONS"); 
    psArray *chips = psStringSplitArray (chipLine, ",", false);
    if (chips->n > 0) {
	pmFPASelectChip (input->fpa, -1, true); // deselect all chips
	for (int i = 0; i < chips->n; i++) {
	    int chipNum = atoi(chips->data[i]);
	    if (! pmFPASelectChip(input->fpa, chipNum, false)) {
		psError(PSASTRO_ERR_CONFIG, true, "Chip number %d doesn't exist in camera.\n", chipNum);
		return false;
	    }
        }
    }
    psFree (chips);

    psTrace("psastro", 1, "Done with psastroParseCamera...\n");
    return true;
}

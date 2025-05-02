/** @file psastroTIOParseCamera.c
 *
 *  @brief
 *
 *  @ingroup psastroTIO
 *
 *  @author IfA
 *  @version $Revision: 1.3 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-02-07 02:03:34 $
 *  Copyright 2009 Institute for Astronomy, University of Hawaii
 */

# include "psastroInternal.h"

bool psastroTIOParseCamera (pmConfig *config) {

    bool status = false;

    // the input image(s) are required arguments; they define the camera
    pmFPAfile *input = pmFPAfileDefineFromArgs (&status, config, "PSASTRO.INPUT", "INPUT");
    if (!status) {
	psError(PSASTRO_ERR_CONFIG, false, "Failed to build FPA from PSASTRO.INPUT");
	return false;
    }

    pmFPAfile *output = pmFPAfileDefineOutput (config, input->fpa, "PSASTRO.OUTPUT");
    if (!output) {
	psError(PSASTRO_ERR_CONFIG, false, "Failed to build FPA from PSASTRO.INPUT");
	return false;
    }
    output->save = true;


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

    psTrace("psastro", 1, "Done with psastroTIOParseCamera...\n");
    return true;
}

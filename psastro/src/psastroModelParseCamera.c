/** @file psastroModelParseCamera.c
 *
 *  @brief
 *
 *  @ingroup psastroModel
 *
 *  @author IfA
 *  @version $Revision: 1.3 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-02-07 02:03:34 $
 *  Copyright 2009 Institute for Astronomy, University of Hawaii
 */

# include "psastroInternal.h"

bool psastroModelParseCamera (pmConfig *config) {

    bool status = false;
    pmFPAfile *input = NULL;

    int nInput = psMetadataLookupS32 (&status, config->arguments, "INPUT.N");
    if (!status) psAbort ("missing INPUT.N in config->arguments");

    // find and load the input files (this determines the camera and builds the fpa structures,
    // but does not load the data)
    for (int i = 0; i < nInput; i++) {
	char name[PS_SMALLWORD];
	ps_snprintf_nowarn (name, PS_SMALLWORD, "INPUT.%d", i);
	input = pmFPAfileDefineFromArgs (&status, config, "PSASTRO.WCS", name);
    }
	
    // set up an output fpa structure & file based on the last input file
    pmFPAfile *output = pmFPAfileDefineOutput (config, input->fpa, "PSASTRO.OUT.MODEL");
    if (!output) {
	psError(PSASTRO_ERR_CONFIG, false, "Failed to build FPA for PSASTRO.OUT.MODEL from input");
	return false;
    }
    output->save = true;

    // Chip selection: turn on only the chips specified (option is not required)
    char *chipLine = psMetadataLookupStr(&status, config->arguments, "CHIP_SELECTIONS"); 
    psArray *chips = psStringSplitArray (chipLine, ",", false);
    if (chips->n > 0) {
	pmFPASelectChip (output->fpa, -1, true); // deselect all chips
	for (int i = 0; i < chips->n; i++) {
	    int chipNum = atoi(chips->data[i]);
	    if (! pmFPASelectChip(output->fpa, chipNum, false)) {
		psError(PSASTRO_ERR_CONFIG, true, "Chip number %d doesn't exist in camera.\n", chipNum);
		return false;
	    }
        }
    }
    psFree (chips);

    psTrace("psastro", 1, "Done with psastroModelParseCamera...\n");
    return true;
}

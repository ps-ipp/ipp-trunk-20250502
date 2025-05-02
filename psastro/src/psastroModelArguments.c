/** @file psastroModelArguments.c
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

# include "psastroStandAlone.h"

static char *usage = "USAGE: psastroModel [-output root] (cmffiles)";

pmConfig *psastroModelArguments (int argc, char **argv) {

    int N;

    if (argc == 1) {
        psError(PSASTRO_ERR_ARGUMENTS, true, "No arguments supplied");
        psErrorStackPrint(stderr, "%s", usage);
	exit (1);
    }

    // load config data from default locations
    pmConfig *config = pmConfigRead(&argc, argv, PSASTRO_RECIPE);
    if (config == NULL) {
        psError(PSASTRO_ERR_CONFIG, false, "Can't read site configuration");
        psErrorStackPrint(stderr, "%s", usage);
	exit (1);
    }

    // save the following additional recipe values based on command-line options
    // these options override the PSASTRO recipe values loaded from recipe files

    // XXX no options yet defined
    // psMetadata *options = pmConfigRecipeOptions (config, PSASTRO_RECIPE);

    // define the output filename
    if ((N = psArgumentGet (argc, argv, "-output"))) {
        psArgumentRemove (N, &argc, argv);
	psMetadataAddStr (config->arguments, PS_LIST_TAIL, "OUTPUT", 0, "", argv[N]);
        psArgumentRemove (N, &argc, argv);
    } else {
        psError(PSASTRO_ERR_ARGUMENTS, true, "Missing -output (root)");
        psErrorStackPrint(stderr, "%s", usage);
	exit (1);
    }

    // chip selection is used to limit chips to be processed
    if ((N = psArgumentGet (argc, argv, "-chip"))) {
        psArgumentRemove (N, &argc, argv);
        psMetadataAddStr (config->arguments, PS_LIST_TAIL, "CHIP_SELECTIONS", PS_META_REPLACE, "", argv[N]);
        psArgumentRemove (N, &argc, argv);
    }
    
    if (argc < 1) {
        psError(PSASTRO_ERR_ARGUMENTS, true, "Incorrect arguments supplied");
        psErrorStackPrint(stderr, "exit");
	exit (1);
    }

    // each additional word is a file; create names INPUT.%d for them
    for (int i = 0; i < argc - 1; i++) {
	char name[PS_SMALLWORD];
	ps_snprintf_nowarn (name, PS_SMALLWORD, "INPUT.%d", i);
	psArray *array = psArrayAlloc(1);
	array->data[0] = psStringCopy (argv[i+1]);
	psMetadataAddPtr(config->arguments, PS_LIST_TAIL, name,  PS_DATA_ARRAY, "", array);
    }	
    psMetadataAddS32(config->arguments, PS_LIST_TAIL, "INPUT.N", 0, "", argc - 1);
    return (config);
}

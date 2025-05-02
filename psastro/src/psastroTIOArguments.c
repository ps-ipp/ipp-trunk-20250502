/** @file psastroTIOArguments.c
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

# include "psastroStandAlone.h"

static char *usage = "USAGE: psastroTIO -file (filename) -output (root)";

pmConfig *psastroTIOArguments (int argc, char **argv) {

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

    // define the output filename
    if ((N = psArgumentGet (argc, argv, "-output")) == FALSE) {
        psError(PSASTRO_ERR_ARGUMENTS, true, "Missing -output (root)");
        psErrorStackPrint(stderr, "%s", usage);
	exit (1);
    }
    psArgumentRemove (N, &argc, argv);
    psMetadataAddStr (config->arguments, PS_LIST_TAIL, "OUTPUT", 0, "", argv[N]);
    psArgumentRemove (N, &argc, argv);

    int status = pmConfigFileSetsMD (config->arguments, &argc, argv, "INPUT", "-file", "-list");
    if (!status) {
        psError(PSASTRO_ERR_ARGUMENTS, true, "Missing -file (input) or -list (input)");
        psErrorStackPrint(stderr, "exit");
        return NULL;
    }

    // chip selection is used to limit chips to be processed
    if ((N = psArgumentGet (argc, argv, "-chip"))) {
        psArgumentRemove (N, &argc, argv);
        psMetadataAddStr (config->arguments, PS_LIST_TAIL, "CHIP_SELECTIONS", PS_META_REPLACE, "", argv[N]);
        psArgumentRemove (N, &argc, argv);
    }
    
    if (argc != 1) {
	psError(PSASTRO_ERR_ARGUMENTS, true, "Incorrect arguments supplied: argc = %d", argc);
	if (argc) {
	    for (int i = 1; i < argc; i++){
		fprintf (stderr, "argc %d : %s\n", i, argv[i]);
	    }
	}
	psErrorStackPrint(stderr, "exit");
	exit (1);
    }
    return (config);
}

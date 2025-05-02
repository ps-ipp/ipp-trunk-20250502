/** @file psastroExtractArguments.c
 *
 *  @brief
 *
 *  @ingroup psastroExtract
 *
 *  @author IfA
 *  @version $Revision: 1.3 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-02-07 02:03:34 $
 *  Copyright 2009 Institute for Astronomy, University of Hawaii
 */

# include "psastroStandAlone.h"

static char *usage = "USAGE: psastroExtract -file (input) -astrom (cmffiles) -outroot (outroot) [-chip]";

pmConfig *psastroExtractArguments (int argc, char **argv) {

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
    // psMetadata *options = pmConfigRecipeOptions (config, PSASTRO_RECIPE);

    // define the image pixel data
    if (!pmConfigFileSetsMD (config->arguments, &argc, argv, "INPUT", "-file", "-list")) {
        psError(PSASTRO_ERR_ARGUMENTS, true, "Missing -file (input) or -list (input)");
        psErrorStackPrint(stderr, "%s", usage);
	exit (1);
    }

    // define the astrometry data (XXX make this optional and use the image header?)
    if (!pmConfigFileSetsMD(config->arguments, &argc, argv, "ASTROM",   "-astrom", "-astromlist")) {
        psError(PSASTRO_ERR_ARGUMENTS, true, "Missing -astrom (smf) or -astromlist (smf)");
        psErrorStackPrint(stderr, "%s", usage);
	exit (1);
    }

    // define the output filename
    N = psArgumentGet (argc, argv, "-output");
    if (!N) {
	psError(PSASTRO_ERR_ARGUMENTS, true, "Missing -output (root)");
        psErrorStackPrint(stderr, "%s", usage);
	exit (1);
    }
    psArgumentRemove (N, &argc, argv);
    psMetadataAddStr (config->arguments, PS_LIST_TAIL, "OUTPUT", 0, "", argv[N]);
    psArgumentRemove (N, &argc, argv);

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

    return (config);
}

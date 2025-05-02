/** @file psastroArguments.c
 *
 *  @brief
 *
 *  @ingroup psastro
 *
 *  @author IfA
 *  @version $Revision: 1.34.2.1 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-02-19 17:59:50 $
 *  Copyright 2009 Institute for Astronomy, University of Hawaii
 */

# include "psastroStandAlone.h"

pmConfig *psastroArguments (int argc, char **argv) {

    bool status;
    int N;

    if (argc == 1) {
        psError(PSASTRO_ERR_ARGUMENTS, true, "No arguments supplied");
        psErrorStackPrint(stderr, "exit");
        return NULL;
    }

    // load config data from default locations
    pmConfig *config = pmConfigRead(&argc, argv, PSASTRO_RECIPE);
    if (config == NULL) {
        psError(PSASTRO_ERR_CONFIG, false, "Can't read site configuration");
        psErrorStackPrint(stderr, "exit");
        return NULL;
    }

    // save the following additional recipe values based on command-line options
    // these options override the PSASTRO recipe values loaded from recipe files
    psMetadata *options = pmConfigRecipeOptions (config, PSASTRO_RECIPE);

    // photcode : used in output to supplement header data (argument or recipe?)
    if ((N = psArgumentGet (argc, argv, "-photcode"))) {
        psArgumentRemove (N, &argc, argv);
        psMetadataAddStr (options, PS_LIST_TAIL, "PHOTCODE", PS_META_REPLACE, "", argv[N]);
        psArgumentRemove (N, &argc, argv);
    }

    // chip selection is used to limit chips to be processed
    if ((N = psArgumentGet (argc, argv, "-chip"))) {
        psArgumentRemove (N, &argc, argv);
        psMetadataAddStr (config->arguments, PS_LIST_TAIL, "CHIP_SELECTIONS", PS_META_REPLACE, "", argv[N]);
        psArgumentRemove (N, &argc, argv);
    }

    // apply the chip correction based on the reference astrometry?
    if ((N = psArgumentGet (argc, argv, "-fixchips"))) {
        psArgumentRemove (N, &argc, argv);
        psMetadataAddBool (config->arguments, PS_LIST_TAIL, "PSASTRO.FIX.CHIPS", PS_META_REPLACE, "", true);
    }
    // no valid header WCS: supply from astrom model
    if ((N = psArgumentGet (argc, argv, "-use-astrommodel"))) {
        psArgumentRemove (N, &argc, argv);
        psMetadataAddBool (config->arguments, PS_LIST_TAIL, "PSASTRO.USE.MODEL", PS_META_REPLACE, "", true);
    }
    // define the reference astrometry file (add a container ASTROM.MODEL to config->arguments if -astrommodel (file) is found)
    status = pmConfigFileSetsMD (config->arguments, &argc, argv, "ASTROM.MODEL", "-astrommodel", "-astrommodellist");

    // define the koppenhoefer correction file (add a container KH.CORRECT to config->arguments if -kh-correct (file) is found)
    status = pmConfigFileSetsMD (config->arguments, &argc, argv, "KH.CORRECT", "-kh-correct", NULL);

    // define the reference astrometry file
    status = pmConfigFileSetsMD (config->arguments, &argc, argv, "INPUT.MASK", "-mask", "-masklist");
    status = pmConfigFileSetsMD (config->arguments, &argc, argv, "REFMASK", "-refmask", "-refmasklist");

    if ((N = psArgumentGet(argc, argv, "-stats"))) {
        psArgumentRemove(N, &argc, argv);
        psMetadataAddStr(config->arguments, PS_LIST_TAIL, "STATS", PS_META_REPLACE, "Filename for summary statistics", argv[N]);
        psArgumentRemove(N, &argc, argv);
    }

    // apply mosastro mode?
    bool mosastro = false;
    if ((N = psArgumentGet (argc, argv, "-mosastro"))) {
        psArgumentRemove (N, &argc, argv);
        psMetadataAddBool (config->arguments, PS_LIST_TAIL, "PSASTRO.MOSAIC.MODE", PS_META_REPLACE, "", false);
    }
    if ((N = psArgumentGet (argc, argv, "+mosastro"))) {
        psArgumentRemove (N, &argc, argv);
        psMetadataAddBool (config->arguments, PS_LIST_TAIL, "PSASTRO.MOSAIC.MODE", PS_META_REPLACE, "", true);
        mosastro = true;
    }

    // apply chipastro mode?
    bool chipastro = false;
    if ((N = psArgumentGet (argc, argv, "-chipastro"))) {
        psArgumentRemove (N, &argc, argv);
        psMetadataAddBool (config->arguments, PS_LIST_TAIL, "PSASTRO.CHIP.MODE", PS_META_REPLACE, "", false);
    }
    if ((N = psArgumentGet (argc, argv, "+chipastro"))) {
        psArgumentRemove (N, &argc, argv);
        psMetadataAddBool (config->arguments, PS_LIST_TAIL, "PSASTRO.CHIP.MODE", PS_META_REPLACE, "", true);
        chipastro = true;
    }
    if ((N = psArgumentGet (argc, argv, "-skipastro"))) {
        psArgumentRemove (N, &argc, argv);
        if (mosastro) {
            psError(PSASTRO_ERR_ARGUMENTS, true, "cannot specify +mosastro with -skipastro");
            psErrorStackPrint(stderr, "exit");
            return false;
        }
        if (chipastro) {
            psError(PSASTRO_ERR_ARGUMENTS, true, "cannot specify +chipastro with -skipastro");
            return false;
        }
        psMetadataAddBool (config->arguments, PS_LIST_TAIL, "PSASTRO.SKIP.ASTRO", PS_META_REPLACE, "", true);

	// SKIP.ASTRO and USE.MODEL are fundamentally incompatible
        psMetadataAddBool (config->arguments, PS_LIST_TAIL, "PSASTRO.USE.MODEL", PS_META_REPLACE, "", false);
    }

    // run in visual mode?
    if ((N = psArgumentGet (argc, argv, "-visual"))) {
        psArgumentRemove (N, &argc, argv);
        pmVisualSetVisual (true);
    }

    // dump the configuration to a file?
    if ((N = psArgumentGet (argc, argv, "-dumpconfig"))) {
        psArgumentRemove (N, &argc, argv);
        psMetadataAddStr (config->arguments, PS_LIST_TAIL, "DUMP_CONFIG", PS_META_REPLACE, "", argv[N]);
        psArgumentRemove (N, &argc, argv);
    }

    status = pmConfigFileSetsMD (config->arguments, &argc, argv, "INPUT", "-file", "-list");
    if (!status) {
        psError(PSASTRO_ERR_ARGUMENTS, true, "Missing -file (input) or -list (input)");
        psErrorStackPrint(stderr, "exit");
        return NULL;
    }

    if (argc != 2) {
        psError(PSASTRO_ERR_ARGUMENTS, true, "Incorrect arguments supplied");
        psErrorStackPrint(stderr, "exit");
        return NULL;
    }

    // output positions is fixed
    psMetadataAddStr (config->arguments, PS_LIST_TAIL, "OUTPUT", 0, "", argv[1]);

    psTrace("psastro", 1, "Done with psastroArguments...\n");
    return (config);
}

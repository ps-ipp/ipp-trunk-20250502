/** @file psastro.c
 *
 *  @brief
 *
 *  @ingroup psastro
 *
 *  @author IfA
 *  @version $Revision: 1.28 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-02-07 02:03:34 $
 *  Copyright 2009 Institute for Astronomy, University of Hawaii
 */

# include "psastroStandAlone.h"

static void usage(void) {
    fprintf(stderr, "USAGE: psastro [-file image(s)] [-list imagelist] (output)\n");
    exit(PS_EXIT_CONFIG_ERROR);
}

int main (int argc, char **argv)
{
    psTimerStart ("complete");

    psastroErrorRegister();              // register our error codes/messages

    // model inits are needed in pmSourceIO
    // models defined in psphot/src/models are not available in psastro
    pmModelClassInit();

    // load configuration information
    pmConfig *config = psastroArguments(argc, argv);
    if (!config) usage();

    psastroVersionPrint();

    // load identify the data sources
    if (!psastroParseCamera (config)) {
        psErrorStackPrint(stderr, "error setting up the camera\n");
        psFree(config);
        exit(PS_EXIT_CONFIG_ERROR);
    }

    // load the raw detection data (using PSPHOT.SOURCES filerule)
    // select subset of stars for astrometry
    if (!psastroDataLoad (config)) {
        psErrorStackPrint(stderr, "error loading input data\n");
        psFree(config);
        exit(PS_EXIT_DATA_ERROR);
    }

    psLogMsg("psastro", 3, "TIMEMARK: psastroDataLoad: %f sec\n", psTimerMark ("complete"));

    psMetadata *stats = psMetadataAlloc(); // Statistics, for output
    psMetadataAddS32(stats, PS_LIST_TAIL, "QUALITY", 0, "No problems", 0);

    // run the full astrometry analysis (chip and/or mosaic)
    if (!psastroAnalysis(config, stats)) {
        psErrorStackPrint(stderr, "failure in psastro analysis\n");
        psFree(config);
        psFree(stats);
        exit(PS_EXIT_SYS_ERROR);
    }

    psLogMsg("psastro", 3, "TIMEMARK: psastroAnalysis: %f sec\n", psTimerMark ("complete"));

    // write out the results
    if (!psastroDataSave(config, stats)) {
        psErrorStackPrint(stderr, "failed to write out data\n");
        psFree(config);
        psFree(stats);
        exit(PS_EXIT_DATA_ERROR);
    }

    psFree (stats);
    psLogMsg("psastro", 3, "complete psastro run: %f sec\n", psTimerMark ("complete"));

    psastroCleanup(config);
    exit(PS_EXIT_SUCCESS);
}

/** @file psastroModel.c
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

int main (int argc, char **argv) {

    pmConfig *config = NULL;

    psTimerStart ("complete");

    psastroErrorRegister();              // register our error codes/messages

    // model inits are needed in pmSourceIO
    // models defined in psphot/src/models are not available in psastro
    pmModelClassInit ();

    // load configuration information
    config = psastroModelArguments (argc, argv);

    // load identify the data sources
    if (!psastroModelParseCamera (config)) {
	psErrorStackPrint(stderr, "error setting up the camera\n");
	exit (1);
    }

    // load the raw pixel data (from PSPHOT.SOURCES)
    // select subset of stars for astrometry
    if (!psastroModelDataLoad (config)) {
	psErrorStackPrint(stderr, "error loading input data\n");
	exit (1);
    }

    // run the full astrometry analysis (chip and/or mosaic)
    if (!psastroModelAnalysis (config)) {
	psErrorStackPrint(stderr, "failure in psastro model analysis\n");
	exit (1);
    }
    
    // run the full astrometry analysis (chip and/or mosaic)
    if (!psastroModelAdjust (config)) {
	psErrorStackPrint(stderr, "failure in psastro model adjust\n");
	exit (1);
    }
    
    // save the model
    if (!psastroModelDataSave (config)) {
	psErrorStackPrint(stderr, "error saving output data\n");
	exit (1);
    }

    psLogMsg ("psastro", 3, "complete psastro run: %f sec\n", psTimerMark ("complete"));

    // psastroCleanup (config);
    exit (EXIT_SUCCESS);
}

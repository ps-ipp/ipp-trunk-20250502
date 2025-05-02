/** @file psastroTIO.c
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

pmConfig *psastroTIOArguments (int argc, char **argv);
bool psastroTIOParseCamera (pmConfig *config);
bool psastroTIODataLoad (pmConfig *config);
bool psastroTIODataSave (pmConfig *config);

int main (int argc, char **argv) {

    pmConfig *config = NULL;

    psTimerStart ("complete");

    // model inits are needed in pmSourceIO
    // models defined in psphot/src/models are not available in psastro
    pmModelClassInit ();

    // load configuration information
    config = psastroTIOArguments (argc, argv);

    // load identify the data sources
    if (!psastroTIOParseCamera (config)) {
	psErrorStackPrint(stderr, "error setting up the camera\n");
	exit (1);
    }

    psLogMsg ("psastro", 3, "parsed camera: %f sec\n", psTimerMark ("complete"));

    // load the raw pixel data (from PSPHOT.SOURCES)
    // select subset of stars for astrometry
    if (!psastroTIODataLoad (config)) {
	psErrorStackPrint(stderr, "error loading input data\n");
	exit (1);
    }

    psLogMsg ("psastro", 3, "read data: %f sec\n", psTimerMark ("complete"));

    // save the model
    if (!psastroTIODataSave (config)) {
	psErrorStackPrint(stderr, "error saving output data\n");
	exit (1);
    }

    psLogMsg ("psastro", 3, "save data: %f sec\n", psTimerMark ("complete"));

    // psastroCleanup (config);
    exit (EXIT_SUCCESS);
}

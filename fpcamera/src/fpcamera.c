# include "fpcamera.h"

/** @file fpcamera.c
 *
 *  @brief
 *
 *  @ingroup fpcamera
 *
 *  @author Eugene Magnier, IfA
 *  @version $Revision$ $LastChangedBy$
 *  @date $Date$
 *  Copyright 2022 Institute for Astronomy, University of Hawaii
 */

# define ESCAPE(ERROR, MSG) { psErrorStackPrint(stderr, MSG); psFree(config); psFree(stats); exit(ERROR); }

void usage(void) {
    fprintf (stderr, "USAGE: one of the following:\n\n");
    fprintf (stderr, "  fpcamera -file filename[,filename,...] -mask maskfile[,maskfile,...] -variance varfile[,varfile,...] -astrom-file (smffile) OutFileBaseName\n");
    fprintf (stderr, "  fpcamera -list filelist -masklist masklist -varlist varlist -astrom-file (smffile) -OutFileBaseName\n\n");
    fprintf (stderr, "where:\n");
    fprintf (stderr, "  FileNameList is a text file containing filenames, one per line\n");
    fprintf (stderr, "  MaskFileNameList is a text file of mask filenames, one per line\n");
    fprintf (stderr, "  VarFileNameList is a text file of variance filenames, one per line\n");
    fprintf (stderr, "  OutFileBaseName is the 'root name' for output files\n\n");
    fprintf(stderr, "Optional arguments:\n");
    fprintf(stderr, "   -stats STATS.mdc: Output statistics into STATS.mdc\n");
    fprintf(stderr, "   -chip CHIPNUM: Only process this chip number.\n\n");
    fprintf(stderr, "   -D FPCAMERA.CATDIR (catdir): specify the reference catalog.\n\n");
    exit (PS_EXIT_CONFIG_ERROR);
}

int main (int argc, char **argv) {

    // these two must be defined and set to NULL so ESCAPE can try to free them
    psMetadata *stats = NULL;
    pmConfig  *config = NULL;

    // initialize & load configuration information
    config = fpcameraArguments(argc, argv);
    if (!config) usage();

    fpcameraVersionPrint();

    // identify the data sources from the header of the astrometry file
    if (!fpcameraParseCamera (config)) ESCAPE(PS_EXIT_CONFIG_ERROR, "Error setting up the camera");

    // load the raw detection data (using PSPHOT.SOURCES filerule)
    // select subset of stars for astrometry
    if (!fpcameraDataLoad (config)) ESCAPE(PS_EXIT_DATA_ERROR, "error loading input data");
    psLogMsg("fpcamera", 3, "TIMEMARK: fpcameraDataLoad: %f sec\n", psTimerMark ("complete"));

    stats = psMetadataAlloc(); // Statistics, for output
    psMetadataAddS32(stats, PS_LIST_TAIL, "QUALITY", 0, "No problems", 0);

    // run the full astrometry analysis (chip and/or mosaic)
    if (!fpcameraAnalysis(config, stats)) ESCAPE(PS_EXIT_SYS_ERROR, "failure in fpcamera analysis");
    psLogMsg("fpcamera", 3, "TIMEMARK: fpcameraAnalysis: %f sec\n", psTimerMark ("complete"));

    // write out the results
    if (!fpcameraDataSave(config, stats)) ESCAPE(PS_EXIT_DATA_ERROR, "failed to write out data");
    psLogMsg("fpcamera", 3, "complete fpcamera run: %f sec\n", psTimerMark ("complete"));

    fpcameraCleanup(config, stats);
    exit(PS_EXIT_SUCCESS);
}

/* code outline

   - init & parse arguments
   
   - construct pmFPAfiles for the inputs and output, identify camera

   - load astrometry from smf file

   - load reference sources for full field

   - loop over chips (already detrended)

     - generate subset of sources for this chip

     - generate a PSF model for this chip
       - use stars in a specific apparent magnitude range?
       - use a starting guess PSF model and choose stars based on initial fit?

     - fit known stars with PSF model to pixel data

     - output cmf / smf
*/


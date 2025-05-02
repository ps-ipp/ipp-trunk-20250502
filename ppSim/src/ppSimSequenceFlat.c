# include "ppSimSequence.h"

bool ppSimSequenceFlat (FILE *simfile, FILE *inject, psMetadata *sequence, int nSeq, psRandom *rng, const char *path, const char *basename, const char *ppSimCommand, const char *injectCommand, psArray *files) {

    bool status;

    // determine the filters & exposure times
    char *filterList = psMetadataLookupStr (&status, sequence, "FILTERS");
    psArray *filters = psStringSplitArray (filterList, ",: ", false);

    psVector *exptimes = psMetadataLookupPtr (&status, sequence, "EXPTIMES");

    if (filters->n != exptimes->n) {
	psLogMsg ("ppSimSequence", PS_LOG_WARN, "mis-match in filter and exptime lists");
	exit (1);
    }

    // number of images for each filter, exptime set
    int nSetup = psMetadataLookupS32 (&status, sequence, "NSETUP");

    int nImage = 0;

    // loop over the filters & exposure times
    for (int i = 0; i < filters->n; i++) {
	    
	// loop over the filters & exposure times
	for (int j = 0; j < nSetup; j++) {
	        
	    // define the output filename
	    psString filename = NULL;
	    if (path) {
		psStringAppend (&filename, "%s/%s.%03d.%03d", path, basename, nSeq, nImage);
	    } else {
		psStringAppend (&filename, "%s.%03d.%03d", basename, nSeq, nImage);
	    }

	    // define the ppSim comand
	    psString command = NULL;

	    psStringAppend (&command, "%s -type FLAT", ppSimCommand);

	    psStringAppend (&command, " -filter %s", (char *) filters->data[i]);
	    psStringAppend (&command, " -exptime %f", exptimes->data.F32[i]);

	    psStringAppend (&command, " %s", filename);

	    fprintf (simfile, "%s\n", command);
	    psFree (command);

	    // define the inject command
	    // path should be dirname/filename
	
	    // we use the filename above (really the file root) to construct the filenames
	    for (int i = 0; i < files->n; i++) {
		command = psStringCopy (injectCommand);
		psStringAppend (&command, " %s.%s", filename, (char *) files->data[i]);
		fprintf (inject, "%s\n", command);
		psFree (command);
	    }

            psFree (filename);
            nImage ++;
	}
    }
    return true;
}

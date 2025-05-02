# include "ppSimSequence.h"

bool ppSimSequenceBias (FILE *simfile, FILE *inject, psMetadata *sequence, int nSeq, psRandom *rng, const char *path, const char *basename, const char *ppSimCommand, const char *injectCommand, psArray *files) {

    bool status, setLevel, setRange;

    // optional details
    float level = psMetadataLookupF32 (&setLevel, sequence, "BIAS.LEVEL");
    float range = psMetadataLookupF32 (&setRange, sequence, "BIAS.RANGE");

    int nImages = psMetadataLookupS32 (&status, sequence, "NIMAGES");

    // loop over the filters & exposure times
    int nImage = 0;
    for (int i = 0; i < nImages; i++) {
	        
	// define the output filename
	psString filename = NULL;
	if (path) {
	    psStringAppend (&filename, "%s/%s.%03d.%03d", path, basename, nSeq, nImage);
	} else {
	    psStringAppend (&filename, "%s.%03d.%03d", basename, nSeq, nImage);
	}

	// define the ppSim command
	psString command = NULL;

	psStringAppend (&command, "%s -type BIAS", ppSimCommand);

	if (setLevel) psStringAppend (&command, " -biaslevel %f", level);
	if (setRange) psStringAppend (&command, " -biasrange %f", range);
          
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
    return true;
}

# include "ppSimSequence.h"

bool ppSimSequenceDark (FILE *simfile, FILE *inject, psMetadata *sequence, int nSeq, psRandom *rng, const char *path, const char *basename, const char *ppSimCommand, const char *injectCommand, psArray *files) {

    bool status, setRate;
    float min, max = 0;

    setRate = false;
    min = psMetadataLookupF32 (&status, sequence, "DARK.MIN");
    if (status) {
	max = psMetadataLookupF32 (&status, sequence, "DARK.MAX");
	setRate = true;
    }

    psVector *exptimes = psMetadataLookupPtr (&status, sequence, "EXPTIMES");
    psVector *nImages = psMetadataLookupPtr (&status, sequence, "NIMAGES");

    assert (exptimes->n == nImages->n);

    // loop over the filters & exposure times
    int nImage = 0;
    for (int i = 0; i < nImages->n; i++) {
	    
	float exptime = exptimes->data.F32[i];
	float n = nImages->data.S32[i];

	for (int j = 0; j < n; j++) {

	    // XXX need to add output filename
	    psString filename = NULL;
	    if (path) {
		psStringAppend (&filename, "%s/%s.%03d.%03d", path, basename, nSeq, nImage);
	    } else {
		psStringAppend (&filename, "%s.%03d.%03d", basename, nSeq, nImage);
	    }

	    // define the ppSim command
	    psString command = NULL;

	    psStringAppend (&command, "%s -type DARK", ppSimCommand);
          
	    if (setRate) {
		double frnd = psRandomUniform(rng);
		float rate = min + (max - min)*frnd;
		psStringAppend (&command, " -darkrate %f", rate);
	    }

	    psStringAppend (&command, " -exptime %f", exptime);

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

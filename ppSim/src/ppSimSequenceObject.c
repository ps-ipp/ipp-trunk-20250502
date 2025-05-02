# include "ppSimSequence.h"

bool ppSimSequenceObject (FILE *simfile, FILE *inject, psMetadata *sequence, int nSeq, psRandom *rng, const char *path, const char *basename, const char *refcat, const char *ppSimCommand, const char *injectCommand, psArray *files) {

    bool status;

    // generate ppSim lines that look like:
    // ppSim -camera $camera -type OBJECT -filter $filter -exptime $exptime
    //       -skyrate $sky -ra $ra -dec $dec -pa $pa -scale $scale -zp $zp -seeing $seeing $filename",

    // sequence reference coordinate
    float Ro = psMetadataLookupF32 (&status, sequence, "CENTER.RA");
    float Do = psMetadataLookupF32 (&status, sequence, "CENTER.DEC");

    // determine the filters & exposure times
    char *filterList = psMetadataLookupStr (&status, sequence, "FILTERS");
    psArray *filters = psStringSplitArray (filterList, ",: ", false);

    psVector *exptimes = psMetadataLookupPtr (&status, sequence, "EXPTIMES");

    psVector *skymags = psMetadataLookupPtr (&status, sequence, "SKYMAGS");

    float IQmin = psMetadataLookupF32 (&status, sequence, "IQ_MIN");
    float IQmax = psMetadataLookupF32 (&status, sequence, "IQ_MAX");

    if (filters->n != exptimes->n) {
	psLogMsg ("ppSimSequence", PS_LOG_WARN, "mis-match in filter and exptime lists");
	exit (1);
    }

    // track the number of files produced
    int nImage = 0;

    // loop over the filters & exposure times
    for (int i = 0; i < filters->n; i++) {
	        
	// offset parameters
	float dR = psMetadataLookupF32 (&status, sequence, "OFFSET.RA");
	float dD = psMetadataLookupF32 (&status, sequence, "OFFSET.DEC");
      
	int nR = psMetadataLookupS32 (&status, sequence, "OFFSET.NR");
	int nD = psMetadataLookupS32 (&status, sequence, "OFFSET.ND");
      
	// loop over the offset sequence
	for (int iR = 0; iR < nR; iR++) {
	    for (int iD = 0; iD < nD; iD++) {

		// RA & DEC in degrees XXX (should be radians...)
		// offsets are in arcseconds
		float R = Ro + dR*(iR - 0.5*nR + 0.5) / cos (RAD_DEG*Do) / 3600.0;
		float D = Do + dD*(iD - 0.5*nD + 0.5) / 3600.0;
              
		// dither parameters
		float dr = psMetadataLookupF32 (&status, sequence, "DITHER.RA");
		float dd = psMetadataLookupF32 (&status, sequence, "DITHER.DEC");
          
		int nr = psMetadataLookupS32 (&status, sequence, "DITHER.NR");
		int nd = psMetadataLookupS32 (&status, sequence, "DITHER.ND");
          
		// loop over the dither sequence
		for (int ir = 0; ir < nr; ir++) {
		    for (int id = 0; id < nd; id++) {

			// ra, dec in degrees; offsets in arcsec
			float ra = R + dr*(ir - 0.5*nr + 0.5) / cos (RAD_DEG*D) / 3600.0;
			float dec = D + dd*(id - 0.5*nd + 0.5) / 3600.0;
	              
			// rotation sequence parameters
			float pos_min   = psMetadataLookupF32 (&status, sequence, "POS_MIN");
			float pos_max   = psMetadataLookupF32 (&status, sequence, "POS_MAX");
			float pos_delta = psMetadataLookupF32 (&status, sequence, "POS_DELTA");
			assert (pos_delta > 0.0);
			assert (pos_max >= pos_min);
	                
			// loop over rotation sequence
			for (float pos = pos_min; pos <= pos_max; pos += pos_delta) {
	                  
			    // define the output filename
			    psString filename = NULL;
			    if (path) {
				psStringAppend (&filename, "%s/%s.%03d.%03d", path, basename, nSeq, nImage);
			    } else {
				psStringAppend (&filename, "%s.%03d.%03d", basename, nSeq, nImage);
			    }

			    // define the ppSim command
			    psString command = NULL;

			    psStringAppend (&command, "%s -type OBJECT", ppSimCommand);
			    psStringAppend (&command, " -filter %s", (char *) filters->data[i]);
			    psStringAppend (&command, " -exptime %f", exptimes->data.F32[i]);
			    psStringAppend (&command, " -skymags %f", skymags->data.F32[i]);

			    psStringAppend (&command, " -ra %f", ra);
			    psStringAppend (&command, " -dec %f", dec);
			    psStringAppend (&command, " -pa %f", pos);
			    psStringAppend (&command, " -obs_mode OBJECT.%s", (char *) filters->data[i]);

			    if (refcat) { psStringAppend (&command, " -D PSASTRO:PSASTRO.CATDIR %s", refcat); }

			    double frnd = psRandomUniform(rng);
			    float seeing = IQmin + (IQmax - IQmin)*frnd;
	                  
			    psStringAppend (&command, " -seeing %f", seeing);

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
		}
	    }
	}
    }
    return true;
}

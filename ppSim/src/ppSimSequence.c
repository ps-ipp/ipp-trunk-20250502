# include "ppSimSequence.h"
# include <sys/stat.h>

int main (int argc, char **argv) {

    bool status;
    int argNum;
    unsigned int nFail;

    psLibInit(NULL);

    char *dbname = NULL;
    if ((argNum = psArgumentGet (argc, argv, "-dbname"))) {
        psArgumentRemove(argNum, &argc, argv);
        dbname = psStringCopy (argv[argNum]);
        psArgumentRemove(argNum, &argc, argv);
    }

    char *path = NULL;
    if ((argNum = psArgumentGet (argc, argv, "-path"))) {
        psArgumentRemove(argNum, &argc, argv);
        path = psStringCopy (argv[argNum]);
        psArgumentRemove(argNum, &argc, argv);
        psString line = NULL;           // Line to execute
        psStringAppend(&line, "mkdir -p %s", path);
        if (!system(line)) {
            psWarning("Unable to create directory %s", path);
        }
        psFree(line);
    }

    char *camera = NULL;
    if ((argNum = psArgumentGet (argc, argv, "-camera"))) {
        psArgumentRemove(argNum, &argc, argv);
        camera = psStringCopy (argv[argNum]);
        psArgumentRemove(argNum, &argc, argv);
    }

    char *workdir = NULL;
    if ((argNum = psArgumentGet (argc, argv, "-workdir"))) {
        psArgumentRemove(argNum, &argc, argv);
        workdir = psStringCopy (argv[argNum]);
        psArgumentRemove(argNum, &argc, argv);
    }

    char *ppsim_recipe = NULL;
    if ((argNum = psArgumentGet (argc, argv, "-ppsim_recipe"))) {
        psArgumentRemove(argNum, &argc, argv);
        ppsim_recipe = psStringCopy (argv[argNum]);
        psArgumentRemove(argNum, &argc, argv);
    }

    char *refcat = NULL;
    if ((argNum = psArgumentGet (argc, argv, "-refcat"))) {
        psArgumentRemove(argNum, &argc, argv);
        refcat = psStringCopy (argv[argNum]);
        psArgumentRemove(argNum, &argc, argv);
    }

    char *dvodb = NULL;
    if ((argNum = psArgumentGet (argc, argv, "-dvodb"))) {
        psArgumentRemove(argNum, &argc, argv);
        dvodb = psStringCopy (argv[argNum]);
        psArgumentRemove(argNum, &argc, argv);
    }

    char *tess_id = NULL;
    if ((argNum = psArgumentGet (argc, argv, "-tess_id"))) {
        psArgumentRemove(argNum, &argc, argv);
        tess_id = psStringCopy (argv[argNum]);
        psArgumentRemove(argNum, &argc, argv);
    }

    char *basename = NULL;
    if ((argNum = psArgumentGet (argc, argv, "-basename"))) {
        psArgumentRemove(argNum, &argc, argv);
        basename = psStringCopy (argv[argNum]);
        psArgumentRemove(argNum, &argc, argv);
    } else {
        basename = psStringCopy ("simtest");
    }

    char *label = NULL;
    if ((argNum = psArgumentGet (argc, argv, "-label"))) {
        psArgumentRemove(argNum, &argc, argv);
        label = psStringCopy (argv[argNum]);
        psArgumentRemove(argNum, &argc, argv);
    }

    if (argc != 4) {
        fprintf (stderr, "USAGE: ppSimSequence (sequence) (simulate) (inject) [options]\n");
        fprintf (stderr, "generates a set of simulated data defined by the sequence file\n");
        fprintf (stderr, " (sequence) : a mdc-file describing the desired image sequences\n");
        fprintf (stderr, " (simulate) : an output file with commands to generate the images\n");
        fprintf (stderr, " (inject)   : an output file with commands to inject the images into the pipeline\n");
        fprintf (stderr, "options:\n");
        fprintf (stderr, " -camera (camera) [otherwise must be set in sequences file]\n");
        fprintf (stderr, " -ppsim_recipe (recipe)\n");
        fprintf (stderr, " -dbname (dbname)\n");
        fprintf (stderr, " -path (path)\n");
        fprintf (stderr, " -workdir (workdir)\n");
        fprintf (stderr, " -basename (basename)\n");
        fprintf (stderr, " -label (label)\n");
        fprintf (stderr, " -dvodb (dvodb)\n");
        fprintf (stderr, " -refcat (catdir)\n");
        fprintf (stderr, " -tess_id (tess_id)\n");
        exit (2);
    }

    // load the sequence description
    psMetadata *config = psMetadataConfigRead (NULL, &nFail, argv[1], false);
    if (!config) {
        psLogMsg ("ppSimSequence", PS_LOG_WARN, "unable to read sequence description from %s", argv[1]);
        exit (1);
    }

    FILE *simfile = fopen (argv[2], "w");
    if (!simfile) {
        psLogMsg ("ppSimSequence", PS_LOG_WARN, "unable to open %s for output", argv[2]);
        exit (1);
    }

    FILE *inject = fopen (argv[3], "w");
    if (!inject) {
        psLogMsg ("ppSimSequence", PS_LOG_WARN, "unable to open %s for output", argv[3]);
        exit (1);
    }

    // build the base injectCommand string
    psString injectCommand = psStringCopy ("ipp_inject_fileset.pl --telescope SIMTEST");
    if (dbname)  psStringAppend (&injectCommand, " --dbname %s",  dbname);
    if (workdir) psStringAppend (&injectCommand, " --workdir %s", workdir);
    if (label)   psStringAppend (&injectCommand, " --label %s", label);
    if (dvodb)   psStringAppend (&injectCommand, " --dvodb %s", dvodb);
    if (tess_id) psStringAppend (&injectCommand, " --tess_id %s", tess_id);

    // build the base ppSimCommand string
    psString ppSimCommand = psStringCopy ("ppSim");
    if (ppsim_recipe) psStringAppend (&ppSimCommand, " -recipe PPSIM %s", ppsim_recipe);

    psRandom *rng = psRandomAlloc(PS_RANDOM_TAUS);

    // global camera option (if not set, we look in each sequence)
    if (camera == NULL) {
	camera = psMetadataLookupStr (&status, config, "CAMERA");
    }

    psArray *files = NULL;
    psArray *sequences = NULL;

    { // find the FILERULE (if it exists) to set the file extension (.fits is default)
	psMetadataItem *filerule = psMetadataLookup (config, "FILERULE");
	if (filerule == NULL) {
	    psLogMsg ("ppSimSequence", PS_LOG_INFO, "no FILERULE, assuming .fits ending");
	    files = psArrayAlloc(1);
	    files->data[0] = psStringCopy("fits");
	    goto sequence;
	} 
	if (filerule->type == PS_DATA_METADATA_MULTI) {
	    psArray *rules = psListToArray (filerule->data.list);
	    psAssert (rules, "failed to get array from list?");
	    psAssert (rules->n > 1, "supposed to be multiple entries in the list?");
	    files = psArrayAllocEmpty(rules->n);
	    for (int i = 0; i < rules->n; i++) {
		psMetadataItem *item = rules->data[i];
		if (item->type != PS_DATA_STRING) {
		    psLogMsg ("ppSimSequence", PS_LOG_WARN, "invalid FILERULE type");
		    exit (PS_EXIT_CONFIG_ERROR);
		}
		psArrayAdd (files, 16, item->data.str);
	    }
	    goto sequence;
	}
	if (filerule->type == PS_DATA_STRING) {
	    files = psArrayAlloc(1);
	    files->data[0] = psStringCopy(filerule->data.str);
	    goto sequence;
	}
	psLogMsg ("ppSimSequence", PS_LOG_WARN, "invalid FILERULE type");
	exit (PS_EXIT_CONFIG_ERROR);
    }

sequence:
    { // find the set of sequences which define the ppSim data to be produced
	psMetadataItem *item = psMetadataLookup (config, "SEQUENCE");
	if (item == NULL) {
	    psLogMsg ("ppSimSequence", PS_LOG_WARN, "missing SEQUENCE description");
	    exit (PS_EXIT_CONFIG_ERROR);
	}

	if (item->type == PS_DATA_METADATA) {
	    sequences = psArrayAlloc(1);
	    sequences->data[0] = psMemIncrRefCounter (item->data.V);
	} else {
	    if (item->type != PS_DATA_METADATA_MULTI)  {
		psLogMsg ("ppSimSequence", PS_LOG_WARN, "SEQUENCE is not MULTI or METADATA");
		exit (1);
	    }
	    sequences = psListToArray (item->data.list);
	}
    }

    for (int i = 0; i < sequences->n; i++) {

        // XXX this is not obvious: the entry on the list is a metadata item
        // containing the psMetadata :: why is this not just a metadata?
        psMetadataItem *item = sequences->data[i];
        psMetadata *sequence = item->data.V;
        // double check item type?

        // determine the sequence type
        char *type = psMetadataLookupStr (&status, sequence, "OBSTYPE");
        if (!status) {
            psLogMsg ("ppSimSequence", PS_LOG_WARN, "SEQUENCE %d is missing a type", i);
            exit (1);
        }

        // determine the camera for the sequence and define the ppSim command
        if (camera == NULL) {
            camera = psMetadataLookupStr (&status, sequence, "CAMERA");
        }
	if (!camera) {
            psLogMsg ("ppSimSequence", PS_LOG_WARN, "CAMERA is not defined");
            exit (1);
	}

        psString injectCommandReal = NULL;
        psString ppSimCommandReal = NULL;

        psStringAppend (&injectCommandReal, "%s --camera %s", injectCommand, camera);
        psStringAppend (&ppSimCommandReal, "%s -camera %s", ppSimCommand, camera);

        if (!strcasecmp (type, "BIAS")) {
            ppSimSequenceBias (simfile, inject, sequence, i, rng, path, basename, ppSimCommandReal, injectCommandReal, files);
	    psFree (injectCommandReal);
	    psFree (ppSimCommandReal);
            continue;
        }
        if (!strcasecmp (type, "DARK")) {
            ppSimSequenceDark (simfile, inject, sequence, i, rng, path, basename, ppSimCommandReal, injectCommandReal, files);
	    psFree (injectCommandReal);
	    psFree (ppSimCommandReal);
            continue;
        }
        if (!strcasecmp (type, "FLAT")) {
            ppSimSequenceFlat (simfile, inject, sequence, i, rng, path, basename, ppSimCommandReal, injectCommandReal, files);
	    psFree (injectCommandReal);
	    psFree (ppSimCommandReal);
            continue;
        }
        if (!strcasecmp (type, "OBJECT")) {
            ppSimSequenceObject (simfile, inject, sequence, i, rng, path, basename, refcat, ppSimCommandReal, injectCommandReal, files);
	    psFree (injectCommandReal);
	    psFree (ppSimCommandReal);
            continue;
        }

        psLogMsg ("ppSimSequence", PS_LOG_WARN, "SEQUENCE %d has an unknown type: %s", i, type);
        exit (1);
    }

    exit (0);
}

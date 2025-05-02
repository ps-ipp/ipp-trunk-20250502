# include "psphotInternal.h"

bool FillImage_Threaded (psThreadJob *job);

bool SetThreads () {

    psThreadTask *task = NULL;

    task = psThreadTaskAlloc("FILL_IMAGE", 6);
    task->function = &FillImage_Threaded;
    psThreadTaskAdd(task);
    psFree(task);

    return true;
}

bool FillImage (psImage *image, int xs, int ys, int dx, int dy, int value) {

    psRegion region = psRegionSet (xs, xs + dx, ys, ys + dy);
    psImage *subset = psImageSubset (image, region);
    psImageInit (subset, value);
    psFree (subset);
    return true;
}

bool FillImage_Threaded (psThreadJob *job) {

    psImage *image = job->args->data[0];
    int xs = PS_SCALAR_VALUE(job->args->data[1],S32);
    int ys = PS_SCALAR_VALUE(job->args->data[2],S32);
    int dx = PS_SCALAR_VALUE(job->args->data[3],S32);
    int dy = PS_SCALAR_VALUE(job->args->data[4],S32);
    int value = PS_SCALAR_VALUE(job->args->data[5],S32);

    // we want the threads to be likely to interact.  run lots of psImageSubsets
    psRegion region = psRegionSet (xs, xs + dx, ys, ys + dy);
    for (int i = 0; i < 100; i++) {
        psImage *subset = psImageSubset (image, region);
        psImageInit (subset, value + i);
        psFree (subset);
    }
    return true;
}

int main (int argc, char **argv) {

    if (argc != 3) {
        fprintf (stderr, "USAGE: psphotTest (output.fits) (nThreads)\n");
        exit (2);
    }

    (void) psTraceSetLevel ("psLib.sys.mutex", 3);

    int nThreads = atoi (argv[2]);

    // create the thread pool with number of desired threads, supplying our thread launcher function
    psThreadPoolInit (nThreads);

    SetThreads();

    psImage *image = psImageAlloc (1000, 1000, PS_TYPE_S32);

    for (int ix = 0; ix < 1000; ix += 100) {
        for (int iy = 0; iy < 1000; iy += 100) {

            // allocate a job -- if threads are not defined, this just runs the job
            psThreadJob *job = psThreadJobAlloc ("FILL_IMAGE");

            psArrayAdd(job->args, 1, image);
            PS_ARRAY_ADD_SCALAR(job->args, ix, PS_TYPE_S32);
            PS_ARRAY_ADD_SCALAR(job->args, iy, PS_TYPE_S32);
            PS_ARRAY_ADD_SCALAR(job->args, 100, PS_TYPE_S32);
            PS_ARRAY_ADD_SCALAR(job->args, 100, PS_TYPE_S32);
            PS_ARRAY_ADD_SCALAR(job->args, ix + iy, PS_TYPE_S32);

            // FillImage (image, ix, iy, 100, 100, ix + iy);

            if (!psThreadJobAddPending(job)) {
                fprintf (stderr, "failure to run FillImage(1)");
                exit (1);
            }
        }
    }


    // wait for the threads to finish and manage results
    if (!psThreadPoolWait (true, true)) {
        fprintf (stderr, "failure to run FillImage (2)");
        exit (1);
    }

    psFits *fits = psFitsOpen (argv[1], "w");
    psFitsWriteImage (fits, NULL, image, 0, NULL);
    psFitsClose (fits);

    psThreadPoolFinalize ();
    psFree(image);

    fprintf (stderr, "found %d leaks\n", psMemCheckLeaks (0, NULL, stdout, false));
    exit (0);
}

# if (0)

psRegion region = psRegionSet (0,0,0,0);        // a region representing the entire array
    psphotTestArguments (&argc, argv);

    psFits *file = psFitsOpen (argv[1], "r");
    psMetadata *header = psFitsReadHeader (NULL, file);
    psImage *image = psFitsReadImage (NULL, file, region, 0);
    psFitsClose (file);

    psImageJpegColormap (argv[5]);

    // psImage *fimage = psImageCopy (NULL, image, PS_TYPE_F32);

    int binning = atof(argv[6]);

    psStats *stats = psStatsAlloc (PS_STAT_SAMPLE_MEAN);
    psImage *fimage = psImageRebin (NULL, image, NULL, 0, binning, stats);

    float min = atof(argv[3]);
    float max = atof(argv[4]);

    psImageJpeg (fimage, argv[2], min, max);

    psFree (header);
    psFree (image);

# endif

# if (0)

    psMetadata *row;
    psArray *table;

    psMetadataItem *mdi;

    psMetadataConfigWrite (header, argv[2], NULL);

    // attempt to write image with NAXIS = 0
    mdi = psMetadataLookup (header, "NAXIS");
    mdi->data.S32 = 0;
    mdi->type = PS_DATA_S32;

    // create a test image
    // psImage *tmpimage = psImageAlloc (10, 10, PS_DATA_F32);

    // create a test table
    table = psArrayAllocEmpty (10);

    for (int i = 0; i < 10; i++) {
        row = psMetadataAlloc ();
        psMetadataAdd (row, PS_LIST_TAIL, "ROW",   PS_DATA_S32,    "", i);
        psMetadataAdd (row, PS_LIST_TAIL, "FROW",  PS_TYPE_F32,    "", 0.1*i);
        psMetadataAdd (row, PS_LIST_TAIL, "DUMMY", PS_DATA_STRING, "", "test line");

        table->data[i] = row;
    }
    table->n = 10;

    psMetadata *theader = psMetadataAlloc ();
    psMetadataAdd (theader, PS_LIST_HEAD, "EXTNAME", PS_DATA_STRING, "extension name", "SMPFILE");

    psFits *fits = psFitsOpen (argv[3], "w");
    // psFitsWriteImage (fits, header, tmpimage, 0);
    psFitsWriteHeader (header, fits);
    psFitsWriteTable (fits, theader, table);

# endif

# if (0)

void psExit (int status, char *process, char *format, ...) {

    va_list ap;

    va_start (ap, format);
    fprintf (stderr, "exiting %s\n", process);
    vfprintf (stderr, format, ap);
    va_end (ap);

    exit (status);
}

# endif

#include <pslib.h>
#include <string.h>
#include "pstampint.h"
#include "pstampROI.h"

// pstamprequest  - create a fits table containing a postage stamp server request


typedef struct {
    psMetadata  *md;
    psString    fileName;
    psString    requestName;
    bool        verbose;
} psrOptions;

char *string_columns[] = {
    "PROJECT",
    "JOB_TYPE",
    "REQ_TYPE", // byid,  byexp, bydiff, bycoord
    "ID",       // db id, exposure name, diff_image_id, n/a
    "COMPONENT",
    "REQFILT",
    "STAMP_NAME",
    NULL
};
char *double_columns[]= {
    "CENTER_X",
    "CENTER_Y",
    "WIDTH",
    "HEIGHT",
    "MJD_MIN",
    "MJD_MAX",
};
char *u32_columns[] = {
    "ROWNUM",
    "COORD_MASK",
    "OPTION_MASK", // bitmask or of PSTAMP_SELECT_IMAGE PSTAMP_SELECT_MASK PSTAMP_SELECT_WEIGHT
    NULL
};

static void usage(int exitStatus)
{
    psErrorStackPrint(stderr, "Unable to parse command-line arguments.");
    exit(exitStatus);
}

static psMetadata *initializeTable()
{
    psMetadata *md = psMetadataAlloc();

    //  unset columns with string type default to "null"
    for (char **col_name = string_columns; *col_name != NULL; col_name++) {
        psMetadataAddStr(md, PS_LIST_TAIL, *col_name, PS_META_DEFAULT, "", "null");
    }
    //  unset columns with u32 type default to 0
    for (char **col_name = u32_columns; *col_name != NULL; col_name++) {
        psMetadataAddU32(md, PS_LIST_TAIL, *col_name, PS_META_DEFAULT, "", 0);
    }
    //  unset columns with f64 type default to 0.
    for (char **col_name = double_columns; *col_name != NULL; col_name++) {
        psMetadataAddF64(md, PS_LIST_TAIL, *col_name, PS_META_DEFAULT, "", 0.0);
    }
    return md;
}
static void getId(char *idString, int argnum, int *pArgc, char *argv[], psrOptions *options)
{
    if (*pArgc < 2) {
        fprintf(stderr, "must specify %s\n", idString);
        usage(PS_EXIT_DATA_ERROR);
    }
    // catch common error
    if (*argv[argnum] == '-') {
        fprintf(stderr, "%s is not a valid %s\n", argv[argnum], idString);
        usage(PS_EXIT_DATA_ERROR);
    }

    psMetadataAddStr (options->md, PS_LIST_TAIL, idString, PS_META_REPLACE, "", argv[argnum]);
    psArgumentRemove(argnum, pArgc, argv);
}


static pstampImageType getType(int argnum, int *pArgc, char *argv[], psrOptions *options, char *paramName)
{
    if (paramName) {
        if (*pArgc < (argnum+2)) {
            fprintf(stderr, "must specify image type and %s\n", paramName);
            usage(PS_EXIT_DATA_ERROR);
        }
    }  else {
        if (*pArgc < (argnum+1)) {
            fprintf(stderr, "must specify image type");
            usage(PS_EXIT_DATA_ERROR);
        }
    }
    char *type = argv[argnum];

    psArgumentRemove(argnum, pArgc, argv);

    pstampImageType itype = PSTAMP_UNKNOWN;

    if (strcmp(type, "raw") == 0) {
        itype = PSTAMP_RAW;
    } else if (strcmp(type, "chip") == 0) {
        itype = PSTAMP_CHIP;
    } else if (strcmp(type, "warp") == 0) {
        itype = PSTAMP_WARP;
    } else if (strcmp(type, "diff") == 0) {
        itype = PSTAMP_DIFF;
    } else if (strcmp(type, "stack") == 0) {
        itype = PSTAMP_STACK;
    } else {
        fprintf(stderr, "unknown image type %s\n", type);
        usage(PS_EXIT_DATA_ERROR);
    }
    psMetadataAddStr (options->md, PS_LIST_TAIL, "IMG_TYPE", PS_META_REPLACE, "", type);

    if (paramName) {
        getId(paramName, argnum, pArgc, argv, options);
    }

    return itype;
}

static void doById(int argnum, int *pArgc, char *argv[], psrOptions *options)
{
    switch (getType(argnum, pArgc, argv, options, "ID")) {
    case PSTAMP_RAW:
        getId("COMPONENT", argnum, pArgc, argv, options);
        break;
    case PSTAMP_CHIP:
        getId("COMPONENT", argnum, pArgc, argv, options);
        break;
    case PSTAMP_WARP:
    case PSTAMP_DIFF:
    case PSTAMP_STACK:
        break;
    default:
        fprintf(stderr, "programming error unexpected image type\n");
        exit(1);
    }
}

static void doByExp(int argnum, int *pArgc, char *argv[], psrOptions *options)
{
    switch (getType(argnum, pArgc, argv, options, "ID")) {
    case PSTAMP_RAW:
        getId("COMPONENT", argnum, pArgc, argv, options);
        break;
    case PSTAMP_CHIP:
        getId("COMPONENT", argnum, pArgc, argv, options);
        break;
    case PSTAMP_WARP:
    case PSTAMP_DIFF:
    case PSTAMP_STACK:
        break;
    default:
        fprintf(stderr, "programming error unexpected image type\n");
        exit(1);
    }
}
static void doByCoord(int argnum, int *pArgc, char *argv[], psrOptions *options)
{
    switch (getType(argnum, pArgc, argv, options, NULL)) {
    case PSTAMP_RAW:
        getId("COMPONENT", argnum, pArgc, argv, options);
        break;
    case PSTAMP_CHIP:
        getId("COMPONENT", argnum, pArgc, argv, options);
        break;
    case PSTAMP_WARP:
    case PSTAMP_DIFF:
    case PSTAMP_STACK:
        break;
    default:
        fprintf(stderr, "programming error unexpected image type\n");
        exit(1);
    }
}

#define PRINT_MULTIPLE_STYLE_ERROR   \
    fprintf(stderr, "one of -bycoord -byid -byexp -bydiff may be specified\n")

static psrOptions *parseArguments(int argc, char *argv[], int *pExitStatus)
{
    psrOptions *options = psAlloc(sizeof(psrOptions));
    psMetadata *md = initializeTable();
    int         argnum;
    bool        gotStyle  = false;
    bool        needCoord = false;
    bool        gotCenter = false;
    bool        gotRange  = false;
    bool        needROI   = false;
    // bool        makeStamps= false; XXX unused
    unsigned    optionMask = PSTAMP_SELECT_IMAGE;

    options->md = md;

    psMetadataAddU32 (md, PS_LIST_TAIL, "ROWNUM", PS_META_REPLACE, "", 1);

    // Job type. 
    // "stamp" for make postage stamps. 
    // "get_image" requests that whole images to be retrieved (magic masked for gpc1)
    // "list_uri" results in a list of matching images
    if ((argnum = psArgumentGet(argc, argv, "-get_image"))) {
        psMetadataAddStr (md, PS_LIST_TAIL, "JOB_TYPE", PS_META_REPLACE, "", "get_image");
        psArgumentRemove(argnum, &argc, argv);
    } else if ((argnum = psArgumentGet(argc, argv, "-list_uri"))) {
        psMetadataAddStr (md, PS_LIST_TAIL, "JOB_TYPE", PS_META_REPLACE, "", "list_uri");
        psArgumentRemove(argnum, &argc, argv);
    } else {
        // default JOB_TYPE is stamp
        psMetadataAddStr (md, PS_LIST_TAIL, "JOB_TYPE", PS_META_REPLACE, "", "stamp");
        needROI = true;
        // XXX unused makeStamps = true;
    }

    if ((argnum = psArgumentGet(argc, argv, "-req_name"))) {
        psArgumentRemove(argnum, &argc, argv);
        if (argc < 2) {
            psError(PS_ERR_BAD_PARAMETER_VALUE, true, "value required for request name");
            usage(PS_EXIT_DATA_ERROR);
        }
        options->requestName = argv[argnum];
        psArgumentRemove(argnum, &argc, argv);
    } else {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, "req_name is required\n");
        usage(PS_EXIT_DATA_ERROR);
    }

    if ((argnum = psArgumentGet(argc, argv, "-project"))) {
        psArgumentRemove(argnum, &argc, argv);
        if (argc < 2) {
            psError(PS_ERR_BAD_PARAMETER_VALUE, true, "project is required");
            usage(PS_EXIT_DATA_ERROR);
        }
        psMetadataAddStr(md, PS_LIST_TAIL, "PROJECT", PS_META_REPLACE, "", argv[argnum]);
        psArgumentRemove(argnum, &argc, argv);
    } else {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, "project is required\n");
        usage(PS_EXIT_DATA_ERROR);
    }

    // if provided, stamp name tag will be appended to the base name for the postage stamp images
    if ((argnum = psArgumentGet(argc, argv, "-stamp_name"))) {
        psArgumentRemove(argnum, &argc, argv);
        if (argc < 2) {
            psError(PS_ERR_BAD_PARAMETER_VALUE, true, "missing value for stamp_name");
            usage(PS_EXIT_DATA_ERROR);
        }
        psMetadataAddStr(md, PS_LIST_TAIL, "STAMP_NAME", PS_META_REPLACE, "", argv[argnum]);
        psArgumentRemove(argnum, &argc, argv);
    }

    if ((argnum = psArgumentGet(argc, argv, "-mask"))) {
        psArgumentRemove(argnum, &argc, argv);
        optionMask |= PSTAMP_SELECT_MASK;
    }
    if ((argnum = psArgumentGet(argc, argv, "-weight"))) {
        psArgumentRemove(argnum, &argc, argv);
        optionMask |= PSTAMP_SELECT_WEIGHT;
    }
    psMetadataAddU32(md, PS_LIST_TAIL, "OPTION_MASK", PS_META_REPLACE, "", optionMask);

    // find style & image type
    if ((argnum = psArgumentGet(argc, argv, "-bycoord"))) {
        gotStyle = true;
        psMetadataAddStr(md, PS_LIST_TAIL, "REQ_TYPE", PS_META_REPLACE, "", 1+argv[argnum]);

        psArgumentRemove(argnum, &argc, argv); needCoord = true;
        needROI = true;
        doByCoord(argnum, &argc, argv, options);
    }

    if ((argnum = psArgumentGet(argc, argv, "-byid"))) {
        if (gotStyle) {
            PRINT_MULTIPLE_STYLE_ERROR;
            usage(PS_EXIT_DATA_ERROR);
        }
        gotStyle = true;
        psMetadataAddStr(md, PS_LIST_TAIL, "REQ_TYPE", PS_META_REPLACE, "", 1+argv[argnum]);
        psArgumentRemove(argnum, &argc, argv);
        doById(argnum, &argc, argv, options);
    }

    if ((argnum = psArgumentGet(argc, argv, "-byexp"))) {
        if (gotStyle) {
            PRINT_MULTIPLE_STYLE_ERROR;
            usage(PS_EXIT_DATA_ERROR);
        }
        gotStyle = true;
        psMetadataAddStr(md, PS_LIST_TAIL, "REQ_TYPE", PS_META_REPLACE, "", 1+argv[argnum]);
        psArgumentRemove(argnum, &argc, argv);
        doByExp(argnum, &argc, argv, options);
    } 

    if ((argnum = psArgumentGet(argc, argv, "-bydiff"))) {
        if (gotStyle) {
            PRINT_MULTIPLE_STYLE_ERROR;
            usage(PS_EXIT_DATA_ERROR);
        }
        gotStyle = true;
        psMetadataAddStr(md, PS_LIST_TAIL, "REQ_TYPE", PS_META_REPLACE, "", 1+argv[argnum]);
        psArgumentRemove(argnum, &argc, argv);
        // It looks like byExp and byId are identical, so reuse them
        doByExp(argnum, &argc, argv, options);
    } 

    if (!gotStyle) {
        fprintf(stderr, "one of -bycoord -byid -byexp -bydiff must be specified\n");
        usage(PS_EXIT_DATA_ERROR);
    }

    if ((argnum = psArgumentGet(argc, argv, "-mjd_min"))) {
        psArgumentRemove(argnum, &argc, argv);
        if (argc < 2) {
            psError(PS_ERR_BAD_PARAMETER_VALUE, true, "missing value mjd_min");
            usage(PS_EXIT_DATA_ERROR);
        }
        psMetadataAddF64(md, PS_LIST_TAIL, "MJD_MIN", PS_META_REPLACE, "", atof(argv[argnum]));
        psArgumentRemove(argnum, &argc, argv);
    } 
    if ((argnum = psArgumentGet(argc, argv, "-mjd_max"))) {
        psArgumentRemove(argnum, &argc, argv);
        if (argc < 2) {
            psError(PS_ERR_BAD_PARAMETER_VALUE, true, "missing value mjd_max");
            usage(PS_EXIT_DATA_ERROR);
        }
        psMetadataAddF64(md, PS_LIST_TAIL, "MJD_MAX", PS_META_REPLACE, "", atof(argv[argnum]));
        psArgumentRemove(argnum, &argc, argv);
    }

    pstampROI roiParam;

    // try and parse the ROI even if we don't "need it" to absorb arguments if they were provided
    if (!pstampGetROI(&roiParam, &argc, argv, &gotCenter, &gotRange)) {
        if (needROI) {
            usage(PS_EXIT_DATA_ERROR);
        } else {
            psErrorClear();
        }
    }

    if (needROI) {
        unsigned coord_mask = 0;

        if (roiParam.celestialCenter) {
            psMetadataAddF64 (md, PS_LIST_TAIL, "CENTER_X", PS_META_REPLACE, "",
                RAD_TO_DEG(roiParam.centerRA));
            psMetadataAddF64 (md, PS_LIST_TAIL, "CENTER_Y", PS_META_REPLACE, "",
                RAD_TO_DEG(roiParam.centerDEC));
        } else {
            if (needCoord) {
                fprintf(stderr, "need to specify ROI in sky coordinates with -bycoord\n");
                usage(PS_EXIT_DATA_ERROR);
            }
            coord_mask |= PSTAMP_CENTER_IN_PIXELS;
            psMetadataAddF64 (md, PS_LIST_TAIL, "CENTER_X", PS_META_REPLACE, "", atof(roiParam.center[0]));
            psMetadataAddF64 (md, PS_LIST_TAIL, "CENTER_Y", PS_META_REPLACE, "", atof(roiParam.center[1]));
        }

        if (!roiParam.celestialRange) {
            coord_mask |= PSTAMP_RANGE_IN_PIXELS;
        }
        psMetadataAddF64 (md, PS_LIST_TAIL, "WIDTH", PS_META_REPLACE, "",  atof(roiParam.range[0]));
        psMetadataAddF64 (md, PS_LIST_TAIL, "HEIGHT", PS_META_REPLACE, "", atof(roiParam.range[1]));

        psMetadataAddU32(md, PS_LIST_TAIL, "COORD_MASK", PS_META_REPLACE, "", coord_mask);
    }

    // only argument left should be the required file name for the request file

    if (argc == 2) {
        options->fileName = psStringCopy(argv[1]);
    } else if (argc == 1) {
        fprintf(stderr, "output file name is required\n");
        usage(PS_EXIT_DATA_ERROR);
    } else {
        fprintf(stderr, "too many arguments supplied:");
        fprintf(stderr, " %s", argv[1]);
        for (int i=2; i<argc; i++) {
            fprintf(stderr, ", %s", argv[i]);
        }
        fprintf(stderr, "\n");
        usage(PS_EXIT_DATA_ERROR);
    }

    return options;
}

static bool writeTable(psrOptions *options, int *pExitStatus)
{
    psFits *fitsFile = psFitsOpen(options->fileName, "w");
    if (fitsFile == NULL) {
        psError(PS_ERR_IO, true, "failed to open %s for output\n", options->fileName);
        *pExitStatus = PS_EXIT_SYS_ERROR;
        return false;
    }
    psMetadata *header = psMetadataAlloc();

    psMetadataAddStr(header, PS_LIST_TAIL, "EXTVER",   PS_META_REPLACE, "", STAMP_REQUEST_VERSION);
    psMetadataAddStr(header, PS_LIST_TAIL, "REQ_NAME", PS_META_REPLACE, "", options->requestName);

    psArray    *table = psArrayAlloc(1);
    table->data[0] = options->md;

    if (!psFitsWriteTable(fitsFile, header, table, STAMP_REQUEST_EXTNAME)) {
        psError(PS_ERR_IO, false, "failed to write fits table");
        *pExitStatus = PS_EXIT_SYS_ERROR;
        return false;
    }

    if (! psFitsClose(fitsFile)) {
        psError(PS_ERR_IO, false, "failed to close fits table");
        *pExitStatus = PS_EXIT_SYS_ERROR;
        return false;
    }

    return true;
}

int main(int argc, char *argv[])
{
    int exitStatus = 0;

    // all of the action happens in parseArguments
    psrOptions *options = parseArguments(argc, argv, &exitStatus);

    if (!options) {
        return 1;
    }

    if (options->verbose) {
        psMetadataPrint(stderr, options->md, 0);
    }

    if (!writeTable(options, &exitStatus)) {
        psErrorStackPrint(stderr, "failed to create request table");
    }
    return exitStatus;
}

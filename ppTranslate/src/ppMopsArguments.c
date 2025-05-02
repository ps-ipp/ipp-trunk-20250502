#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <pslib.h>
#include <psmodules.h>

#include "ppMops.h"

// Print usage information and die
static void usage(const char *program,  // Name of the program
                  psMetadata *arguments // Command-line arguments
                  )
{
    fprintf(stderr, "\nPan-STARRS IPP-MOPS detection translator\n\n");
    fprintf(stderr, "Usage: %s INPUT_LIST OUTPUT_NAME\n", program);
    fprintf(stderr, "\n");
    fprintf(stderr, "Description: Merge detections from INPUT_LIST into the single OUTPUT_NAME file\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "\tIf the CMF input files have different versions, \n\
\tmerging cannot be performed.\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "\tIf -version option is not given, the output version is\n\
\tthe version of the input file(s) otherwise the output version is (possibly forced to)\n\
\tthe version option.\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "\tIf the input file(s) version is equals to the version option:\n\
\t  No change in version (neither data creation nor data loss)\n\
\tIf the input file(s) version is strictly less than the version option:\n\
\t Data for version option are set to default values: 0, NaN, NULL\n\
\tIf the input file(s) version is strictly greater than the version option:\n\
\t  Data are those of the lower version.\n");
    fprintf(stderr, "\n");
    psArgumentHelp(arguments);
    fprintf(stderr, "\t\tCMF file version can be set to either 1 for PS1_DV1, 2 for PS1_DV2, or 3 for PS1_DV3\n");
    fprintf(stderr, "\t\tSee IPP-MOPS ICD for details\n");
    psLibFinalize();
    exit(PS_EXIT_CONFIG_ERROR);
}

static void mopsArgumentsFree(ppMopsArguments *args)
{
    psFree(args->camera);
    psFree(args->obscode);
    psFree(args->input);
    psFree(args->exp_name);
    psFree(args->comment);
    psFree(args->obsMode);
    psFree(args->difftype);
    psFree(args->shutoutc);
    return;
}

ppMopsArguments *ppMopsArgumentsAlloc(void)
{
    ppMopsArguments *args = psAlloc(sizeof(ppMopsArguments)); // Data to return
    psMemSetDeallocator(args, (psFreeFunc)mopsArgumentsFree);
    args->input = NULL;
    args->exp_name = NULL;
    args->exp_id = 0;
    args->chip_id = 0;
    args->cam_id = 0;
    args->fake_id = 0;
    args->warp_id = 0;
    args->diff_id = 0;
    args->camera = NULL;
    args->obscode = NULL;
    args->zp = NAN;
    args->positive = true;
    args->zpErr = NAN;
    args->rmsAstrom = NAN;
    args->output = NULL;
    args->version = 1;
    args->comment = NULL;
    args->obsMode = NULL;
    args->difftype = NULL;
    args->sky = NAN;
    args->shutoutc = NULL;
    return args;
}


ppMopsArguments *ppMopsArgumentsParse(int argc, char *argv[])
{
    assert(argv);

    psTrace("ppMops.args", 1, "Parsing command-line arguments\n");

    psArgumentVerbosity(&argc, argv);

    psMetadata *arguments = psMetadataAlloc(); // Command-line arguments
    psMetadataAddStr(arguments, PS_LIST_TAIL, "-exp_name", 0, "Exposure name", NULL);
    psMetadataAddS64(arguments, PS_LIST_TAIL, "-exp_id", 0, "Exposure identifier", 0);
    psMetadataAddS64(arguments, PS_LIST_TAIL, "-chip_id", 0, "Chip stage identifier", 0);
    psMetadataAddS64(arguments, PS_LIST_TAIL, "-cam_id", 0, "Camera stage identifier", 0);
    psMetadataAddS64(arguments, PS_LIST_TAIL, "-fake_id", 0, "Fake stage identifier", 0);
    psMetadataAddS64(arguments, PS_LIST_TAIL, "-warp_id", 0, "Warp stage identifier", 0);
    psMetadataAddS64(arguments, PS_LIST_TAIL, "-diff_id", 0, "Diff stage identifier", 0);
    psMetadataAddStr(arguments, PS_LIST_TAIL, "-camera", 0, "Camera name", "No comment provided");
    psMetadataAddBool(arguments, PS_LIST_TAIL, "-inverse", 0, "Inverse subtraction?", false);
    psMetadataAddF32(arguments, PS_LIST_TAIL, "-zp", 0, "Magnitude zero point", NAN);
    psMetadataAddF32(arguments, PS_LIST_TAIL, "-zp_error", 0, "Error in magnitude zero point", NAN);
    psMetadataAddF32(arguments, PS_LIST_TAIL, "-astrom_rms", 0, "Astrometric solution RMS", NAN);
    psMetadataAddU16(arguments, PS_LIST_TAIL, "-version", 0, "CMF file version", 0);
    psMetadataAddStr(arguments, PS_LIST_TAIL, "-comment", 0, "Exposure comment", "No comment provided");
    psMetadataAddStr(arguments, PS_LIST_TAIL, "-obsmode", 0, "Observation mode", "No obsmode provided");
    psMetadataAddStr(arguments, PS_LIST_TAIL, "-difftype", 0, "Either WW (Warp-Warp Diff) or WS (Warp-Stack Diff)", "No difftype provided");
    psMetadataAddF32(arguments, PS_LIST_TAIL, "-sky", 0, "Exposure avg sky background", NAN);
    psMetadataAddStr(arguments, PS_LIST_TAIL, "-shutoutc", 0, "Camera exposure shutter open (UTC)", "No shutoutc provided");
    if (argc == 1 || !psArgumentParse(arguments, &argc, argv) || argc != 3) {
        usage(argv[0], arguments);
    }

    ppMopsArguments *args = ppMopsArgumentsAlloc(); // Arguments, to return

    psString inList = psSlurpFilename(argv[1]); // List of filenames
    args->input = psStringSplitArray(inList, "\n", false);
    psFree(inList);
    if (!args->input || args->input->n == 0) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, "No inputs provided.");
        return NULL;
    }
    args->output = psStringCopy(argv[2]);

    args->exp_name = psMemIncrRefCounter(psMetadataLookupStr(NULL, arguments, "-exp_name"));
    args->exp_id = psMetadataLookupS64(NULL, arguments, "-exp_id");
    args->chip_id = psMetadataLookupS64(NULL, arguments, "-chip_id");
    args->cam_id = psMetadataLookupS64(NULL, arguments, "-cam_id");
    args->fake_id = psMetadataLookupS64(NULL, arguments, "-fake_id");
    args->warp_id = psMetadataLookupS64(NULL, arguments, "-warp_id");
    args->diff_id = psMetadataLookupS64(NULL, arguments, "-diff_id");
    args->camera = psMemIncrRefCounter(psMetadataLookupStr(NULL, arguments, "-camera"));
    args->positive = !psMetadataLookupBool(NULL, arguments, "-inverse"); // NOTE: negated
    args->zp = psMetadataLookupF32(NULL, arguments, "-zp");
    args->zpErr = psMetadataLookupF32(NULL, arguments, "-zp_error");
    args->rmsAstrom = psMetadataLookupF32(NULL, arguments, "-astrom_rms");
    args->version = psMetadataLookupU16(NULL, arguments, "-version");
    args->comment = psMemIncrRefCounter(psMetadataLookupStr(NULL, arguments, "-comment"));
    args->obsMode = psMemIncrRefCounter(psMetadataLookupStr(NULL, arguments, "-obsmode"));
    args->difftype = psMemIncrRefCounter(psMetadataLookupStr(NULL, arguments, "-difftype"));
    args->sky = psMetadataLookupF32(NULL, arguments, "-sky");
    args->shutoutc = psMemIncrRefCounter(psMetadataLookupStr(NULL, arguments, "-shutoutc"));

    if (!args->camera) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, "camera name not provided.");
        return NULL;
    }
    if (!strcasecmp(args->camera, "GPC1")) {
      args->obscode = psStringCopy("F51");
    }
    if (!strcasecmp(args->camera, "GPC2")) {
      args->obscode = psStringCopy("F52");
    }
    if (!args->obscode) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, "unknown camera name.");
        return NULL;
    }

    psTrace("ppMops.args", 1, "Done parsing command-line arguments\n");

    psFree(arguments);
    return args;
}

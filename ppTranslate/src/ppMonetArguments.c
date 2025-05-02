#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <pslib.h>
#include <psmodules.h>

#include "ppMonet.h"

// Print usage information and die
static void usage(const char *program,  // Name of the program
                  psMetadata *arguments // Command-line arguments
                  )
{
    fprintf(stderr, "\nPan-STARRS IPP-Monet detection translator\n\n");
    fprintf(stderr, "Usage: %s INPUT_NAME OUTPUT_NAME\n", program);
    fprintf(stderr, "\n");
    psArgumentHelp(arguments);
    psLibFinalize();
    exit(PS_EXIT_CONFIG_ERROR);
}

static void monetArgumentsFree(ppMonetArguments *args)
{
    psFree(args->input);
    psFree(args->exp_name);
    psFree(args->output);
    return;
}

ppMonetArguments *ppMonetArgumentsAlloc(void)
{
    ppMonetArguments *args = psAlloc(sizeof(ppMonetArguments)); // Data to return
    psMemSetDeallocator(args, (psFreeFunc)monetArgumentsFree);

    args->input = NULL;
    args->exp_name = NULL;
    args->exp_id = 0;
    args->chip_id = 0;
    args->cam_id = 0;
    args->zp = NAN;
    args->zpErr = NAN;
    args->rmsAstrom = NAN;
    args->output = NULL;

    return args;
}


ppMonetArguments *ppMonetArgumentsParse(int argc, char *argv[])
{
    assert(argv);

    psTrace("ppMonet.args", 1, "Parsing command-line arguments\n");

    psArgumentVerbosity(&argc, argv);

    psMetadata *arguments = psMetadataAlloc(); // Command-line arguments
    psMetadataAddStr(arguments, PS_LIST_TAIL, "-exp_name", 0, "Exposure name", NULL);
    psMetadataAddS64(arguments, PS_LIST_TAIL, "-exp_id", 0, "Exposure identifier", 0);
    psMetadataAddS64(arguments, PS_LIST_TAIL, "-chip_id", 0, "Chip stage identifier", 0);
    psMetadataAddS64(arguments, PS_LIST_TAIL, "-cam_id", 0, "Camera stage identifier", 0);
    psMetadataAddF32(arguments, PS_LIST_TAIL, "-zp", 0, "Magnitude zero point", NAN);
    psMetadataAddF32(arguments, PS_LIST_TAIL, "-zp_error", 0, "Error in magnitude zero point", NAN);
    psMetadataAddF32(arguments, PS_LIST_TAIL, "-astrom_rms", 0, "Astrometric solution RMS", NAN);

    if (argc == 1 || !psArgumentParse(arguments, &argc, argv) || argc != 3) {
        usage(argv[0], arguments);
    }

    ppMonetArguments *args = ppMonetArgumentsAlloc(); // Arguments, to return

    args->input = psStringCopy(argv[1]);
    args->output = psStringCopy(argv[2]);

    args->exp_name = psMetadataLookupStr(NULL, arguments, "-exp_name");
    args->exp_id = psMetadataLookupS64(NULL, arguments, "-exp_id");
    args->chip_id = psMetadataLookupS64(NULL, arguments, "-chip_id");
    args->cam_id = psMetadataLookupS64(NULL, arguments, "-cam_id");
    args->zp = psMetadataLookupF32(NULL, arguments, "-zp");
    args->zpErr = psMetadataLookupF32(NULL, arguments, "-zp_error");
    args->rmsAstrom = psMetadataLookupF32(NULL, arguments, "-astrom_rms");

    psTrace("ppMonet.args", 1, "Done parsing command-line arguments\n");

    return args;
}

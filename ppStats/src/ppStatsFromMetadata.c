# include "ppStatsInternal.h"

// USAGE: ppStatsFromMetadata (input) (output) [-recipe PPSTATS_METADATA recipe]
int main(int argc, char **argv)
{
    bool status;

    psLibInit(NULL);

    // Parse the configuration and arguments
    pmConfig *config = pmConfigRead(&argc, argv, PPSTATS_MD_RECIPE);
    if (!config) {
        psErrorStackPrint(stderr, "Unable to read configuration.\n");
        exit(PS_EXIT_CONFIG_ERROR);
    }

    if (argc != 4) {
        psErrorStackPrint(stderr, "USAGE: ppStatsFromMetadata (input) (output) (recipe)\n");
        exit(PS_EXIT_CONFIG_ERROR);
    }

    psMetadata *input = NULL;
    if (!strcmp (argv[1], "-")) {
        psString string = psSlurpFile (stdin);
        input = psMetadataConfigParse (NULL, NULL, string, false);
        psFree (string);
    } else {
        input = psMetadataConfigRead (NULL, NULL, argv[1], false);
    }

    // parse the recipe to determine the fields of interest
    psMetadata *recipes = psMetadataLookupPtr (&status, config->recipes, PPSTATS_MD_RECIPE);
    if (!recipes) {
        psErrorStackPrint(stderr, "missing recipe set %s\n", PPSTATS_MD_RECIPE);
        exit(PS_EXIT_CONFIG_ERROR);
    }

    psMetadata *recipe = psMetadataLookupPtr (&status, recipes, argv[3]);
    if (!recipes) {
        psErrorStackPrint(stderr, "missing ppStatsFromMetadata recipe %s\n", argv[3]);
        exit(PS_EXIT_CONFIG_ERROR);
    }

    psArray *entries = ppStatsFromMetadataEntries (recipe);
    if (!entries) {
        psErrorStackPrint(stderr, "problem with recipe %s\n", PPSTATS_MD_RECIPE);
        exit(PS_EXIT_CONFIG_ERROR);
    }

    if (!ppStatsFromMetadataParse (input, entries)) {
        psErrorStackPrint(stderr, "problem with input data\n");
        exit(PS_EXIT_CONFIG_ERROR);
    }

    // calculate the stats for the non-constant entries (already calculated)
    if (!ppStatsFromMetadataStats (entries)) {
        psErrorStackPrint(stderr, "problem calculating statistics\n");
        exit(PS_EXIT_CONFIG_ERROR);
    }

    // calculate the stats for the non-constant entries (already calculated)
    if (!ppStatsFromMetadataPrint (entries, argv[2])) {
        psErrorStackPrint(stderr, "problem calculating statistics\n");
        exit(PS_EXIT_CONFIG_ERROR);
    }

    exit (0);
}

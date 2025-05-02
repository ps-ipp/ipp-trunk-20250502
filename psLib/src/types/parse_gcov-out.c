#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAXSTR 10000

static char outStr[MAXSTR];
static float average;
static int sum;

void parseFile(FILE *in, const char *filename);

int main(void)
{

    FILE *output = NULL;
    FILE *input = NULL;
    average = 0.0;
    sum = 0;
    int N = 0;

    input = fopen("arg_gcov.out", "r");
    parseFile(input, "psArguments.c");
    fclose(input);
    N++;

    input = fopen("arr_gcov.out", "r");
    parseFile(input, "psArray.c");
    fclose(input);
    N++;

    input = fopen("bit_gcov.out", "r");
    parseFile(input, "psBitSet.c");
    fclose(input);
    N++;

    input = fopen("hash_gcov.out", "r");
    parseFile(input, "psHash.c");
    fclose(input);
    N++;

    input = fopen("list_gcov.out", "r");
    parseFile(input, "psList.c");
    fclose(input);
    N++;

    input = fopen("look_gcov.out", "r");
    parseFile(input, "psLookupTable.c");
    fclose(input);
    N++;

    input = fopen("md_gcov.out", "r");
    parseFile(input, "psMetadata.c");
    fclose(input);
    N++;

    input = fopen("mdic_gcov.out", "r");
    parseFile(input, "psMetadataItemCompare.c");
    fclose(input);
    N++;

    input = fopen("mdip_gcov.out", "r");
    parseFile(input, "psMetadataItemParse.c");
    fclose(input);
    N++;

    input = fopen("mdc_gcov.out", "r");
    parseFile(input, "psMetadataConfig.c");
    fclose(input);
    N++;

    input = fopen("pix_gcov.out", "r");
    parseFile(input, "psPixels.c");
    fclose(input);
    N++;

    average = average / sum;

    output = fopen("Coverage-Report.txt", "w");
    fprintf(output, "\nTOTAL COVERAGE IN THE ../src/types/ DIRECTORY\n");
    fprintf(output, "\n%s", outStr);
    fprintf(output, "  -------------------------------"
            "-------------------------------------\n");
    fprintf(output, "  ---> Total                        = Lines executed:");
    fprintf(output, "%3.2f%%  of %d\n\n", average, sum);
    fclose(output);
    return 0;
}

void parseINT(const char *nums, float percent)
{
    sum += atoi(nums);
    average += (float)((int)(percent * atoi(nums)));
}

float parseDBL(const char *nums)
{
    char temp[7];
    int i = 0;
    int j = 9;
    float out = 0.0;
    while (nums[j] != '%') {
        temp[i] = nums[j];
        i++;
        j++;
    }
    //    average += (float)atof(temp);
    out = (float)atof(temp);
    return out;
}

void parseFile(FILE *in, const char *filename)
{
    char currentStr[100];
    char searchStr[100];
    char out[250];
    sprintf(out, "  >><< %-25s    =", filename);
    sprintf(searchStr, "'%s'", filename);
    //    printf("\n searchStr = %s\n", searchStr);
    strcat(outStr, out);
    int i;
    float numLines = 0.0;
    while ( fscanf(in, "%s", currentStr) != EOF) {
        if ( strncmp(currentStr, searchStr, 99) == 0 ) {
            for (i = 0; i < 4; i++) {
                if (i == 1) {
                    fscanf(in, "%s", out);
                    numLines = parseDBL(out);
                    sprintf(currentStr, "%-16s", out);
                } else if ( i == 3) {
                    fscanf(in, "%s", out);
                    parseINT(out, numLines);
                    sprintf(currentStr, "%s", out);
                } else {
                    fscanf(in, "%s", currentStr);
                }
                strcat(outStr, " ");
                strcat(outStr, currentStr);
            }
            continue;
        }
    }
    strcat(outStr, "\n\n");
}


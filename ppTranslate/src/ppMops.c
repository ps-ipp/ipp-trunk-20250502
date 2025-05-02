#include <stdio.h>
#include <pslib.h>

#include "ppMops.h"

void test()
{
    psMetadata *md = psMetadataAlloc();

    psVector *vec = psVectorAlloc(42, PS_TYPE_S32);

    psMetadataAddVector(md, PS_LIST_TAIL, "TEST", 0, NULL, vec);

    psFree(vec);
    psFree(md);

    psLibFinalize();

    fprintf (stderr, "found %d leaks at %s\n", 
    	psMemCheckLeaks2 (0,
		NULL, stdout, false, 500), "ppMops");

    exit(0);
}

/*
  Behavior:

  If the CMF input files have different versions, merging cannot be
  performed.

  If -version option is not given: 
    the output version is the version of the input file(s)
  otherwise
    the output version is (possibly forced to) the version option

  If the input file(s) version is equals to the version option:
    no change in version (neither data creation nor data loss)
  If the input file(s) version is strictly less than the version option:
    Data for version option are set tp default values: 0, NaN, NULL
  If the input file(s) version is strictly greater than the version option:
    Data are those of the lower version

  Example:
   -> ppMops dv1_input_files_list output -version 1
      is the same as 'ppMops dv1_input_files_list output'
   -> ppMops dv1_input_files_list output -version 1
      is the same as 'ppMops dv1_input_files_list output'
   -> ppMops dv1_input_files_list output -version 2
      Aggregate DV1 values and add default DV2 values
      Saved as DV2 file
   -> ppMops dv2_input_files_list output -version 1
      Aggregate DV1 values and truncate DV2 values 
      Saved as DV1 file
 */

int main(int argc, char *argv[])
{
    psLibInit(NULL);

    // test();

    ppMopsArguments *args = ppMopsArgumentsParse(argc, argv); // Parsed arguments
    if (!args) {
        psErrorStackPrint(stderr, "Error parsing arguments");
        exit(PS_EXIT_CONFIG_ERROR);
    }

    psArray *detections = ppMopsRead(args); // Detections from each input
    if (!detections) {
        psErrorStackPrint(stderr, "Unable to read detections");
        exit(PS_EXIT_SYS_ERROR);
    }

    if (!ppMopsPurgeDuplicates(detections)) {
        psErrorStackPrint(stderr, "Unable to merge detections");
        exit(PS_EXIT_SYS_ERROR);
    }

    if (!ppMopsWrite(detections, args)) {
        psErrorStackPrint(stderr, "Unable to write detections");
        exit(PS_EXIT_SYS_ERROR);
    }

    for (int i = 0; i < detections->n; i++) {
        psFree(detections->data[i]);
    }
    psFree(detections);
    psFree(args);

    psLibFinalize();

/*     fprintf (stderr, "found %d leaks at %s\n",  */
/*     	psMemCheckLeaks2 (0, */
/* 		NULL, stdout, false, 500), "ppMops"); */

    return PS_EXIT_SUCCESS;
}


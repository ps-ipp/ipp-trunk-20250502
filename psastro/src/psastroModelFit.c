/** @file psastroModelFit.c
 *
 *  @brief
 *
 *  @ingroup psastroModel
 *
 *  @author IfA
 *  @version $Revision: 1.3 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-02-07 02:03:34 $
 *  Copyright 2009 Institute for Astronomy, University of Hawaii
 */

# include "psastroStandAlone.h"

int main (int argc, char **argv) {

    psTimerStart ("complete");

    if (argc != 3) {
	fprintf (stderr, "USAGE: psastroModelFit (input) (output)\n");
	exit (1);
    }

    FILE *f = fopen (argv[1], "r");
    if (f == NULL) {
	fprintf (stderr, "problem opening data file %s\n", argv[1]);
	exit (2);
    }

    char name[1024];
    psVector *Xo = psVectorAllocEmpty (100, PS_TYPE_F32);
    psVector *Yo = psVectorAllocEmpty (100, PS_TYPE_F32);
    psVector *Po = psVectorAllocEmpty (100, PS_TYPE_F32);

    float x, y, p;

    while (fscanf (f, "%s %f %f %f", name, &x, &y, &p) != EOF) {
	psVectorAppend (Xo, x);
	psVectorAppend (Yo, y);
	psVectorAppend (Po, p*PS_RAD_DEG);
    }	

    // psTraceSetLevel("psLib.math", 5);
    psastroModelFitBoresite (Xo, Yo, Po, argv[2]);

    psLogMsg ("psastro", 3, "complete psastroModelFit run: %f sec\n", psTimerMark ("complete"));

    exit (EXIT_SUCCESS);
}

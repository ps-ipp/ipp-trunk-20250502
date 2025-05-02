#include <stdio.h>
#include <string.h>
#include <pslib.h>
#include <psmodules.h>

#include "tap.h"
#include "pstap.h"

int main (int argc, char **argv)
{
    if (argc != 8) {
	fprintf (stderr, "USAGE: tap_pmModel_CentralPixel (model) (dx) (dy) (theta) (Reff) (Arat) (Sidx)\n");
	exit (2);
    }

    float dx = atof(argv[2]);
    float dy = atof(argv[3]);
    float theta = atof(argv[4]);

    float Reff = atof(argv[5]);
    float Arat = atof(argv[6]);
    float Sidx = atof(argv[7]);

    psMemId id = psMemGetId();

    plan_tests(6);

    pmModelCPset *cpset = pmModelCP_Load (argv[1]);
    ok (cpset, "loaded pmModelCPset from file");

    ok (cpset->RmajorNitem ==  4, "correct number of Rmajor values");
    ok (cpset->AratioNitem ==  7, "correct number of Aratio values");
    ok (cpset->SindexNitem == 10, "correct number of Sindex values");
    ok (cpset->images->n == 280, "correct number of CP images");

    pmModelCP *cp = NULL;
    
    float valuePixel = NAN;
    float valueModel = NAN;

    cp = pmModelCP_GetImage (cpset, log10(Reff), Arat, Sidx);
    valuePixel = pmModelCP_GetFlux (cp, dx, dy, theta);
    valueModel = pmModelCP_FullSersic (dx, dy, theta, Reff, Arat, Sidx);
    fprintf (stdout, "%f / %f = %f\n", valuePixel, valueModel, valuePixel / valueModel);

    fprintf (stdout, "%f, %f, %f\n", cp->Rmajor, cp->Aratio, cp->Sindex);

    psFree (cpset);
    ok(!psMemCheckLeaks (id, NULL, stderr, false), "no memory leaks");

    return exit_status();
}

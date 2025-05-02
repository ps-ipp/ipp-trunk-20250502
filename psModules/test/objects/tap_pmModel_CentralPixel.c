#include <stdio.h>
#include <string.h>
#include <pslib.h>
#include <psmodules.h>

#include "tap.h"
#include "pstap.h"

int main (int argc, char **argv)
{
    if (argc != 3) {
	fprintf (stderr, "USAGE: tap_pmModel_CentralPixel (model) (set)\n");
	exit (2);
    }

    int set = atoi(argv[2]);

    psMemId id = psMemGetId();

    plan_tests(240);

    pmModelCP *t0 = pmModelCP_Alloc();
    ok (t0, "allocated pmModelCP");
    psFree (t0);

    pmModelCPset *t1 = pmModelCPset_Alloc();
    ok (t1, "allocated pmModelCPset");
    psFree (t1);
    
    pmModelCPset *cpset = pmModelCP_Load (argv[1]);
    ok (cpset, "loaded pmModelCPset from file");

    ok (cpset->RmajorNitem ==  4, "correct number of Rmajor values");
    ok (cpset->AratioNitem ==  7, "correct number of Aratio values");
    ok (cpset->SindexNitem == 10, "correct number of Sindex values");
    ok (cpset->images->n == 280, "correct number of CP images");

    pmModelCP *cp = NULL;
    
    if (1) {
	cp = pmModelCP_GetImage (cpset, 0.0, 1.0, 1.0);
	ok (cp, "returned a cp image");
	if (cp) {
	    ok_float_tol (cp->Rmajor, 0.0, 0.001, "got image with correct Rmajor");
	    ok_float_tol (cp->Aratio, 1.0, 0.001, "got image with correct Aratio");
	    ok_float_tol (cp->Sindex, 1.0, 0.001, "got image with correct Sindex");
	}

	cp = pmModelCP_GetImage (cpset, 1.0, 1.0, 1.0);
	ok (cp, "returned a cp image");
	if (cp) {
	    ok_float_tol (cp->Rmajor, 1.0, 0.001, "got image with correct Rmajor");
	    ok_float_tol (cp->Aratio, 1.0, 0.001, "got image with correct Aratio");
	    ok_float_tol (cp->Sindex, 1.0, 0.001, "got image with correct Sindex");
	}
    
	cp = pmModelCP_GetImage (cpset, 0.0, 0.4, 1.0);
	ok (cp, "returned a cp image");
	if (cp) {
	    ok_float_tol (cp->Rmajor, 0.0, 0.001, "got image with correct Rmajor");
	    ok_float_tol (cp->Aratio, 0.4, 0.001, "got image with correct Aratio");
	    ok_float_tol (cp->Sindex, 1.0, 0.001, "got image with correct Sindex");
	}
    
	cp = pmModelCP_GetImage (cpset, 0.0, 0.4, 3.5);
	ok (cp, "returned a cp image");
	if (cp) {
	    ok_float_tol (cp->Rmajor, 0.0, 0.001, "got image with correct Rmajor");
	    ok_float_tol (cp->Aratio, 0.4, 0.001, "got image with correct Aratio");
	    ok_float_tol (cp->Sindex, 3.5, 0.001, "got image with correct Sindex");
	}
    }
    
    float valuePixel = NAN;
    float valueModel = NAN;

    switch (set) {
      case 0:
	cp = pmModelCP_GetImage (cpset, 1.0, 1.0, 1.0);
	valuePixel = pmModelCP_GetFlux (cp, 0.0, 0.0, 0.0);
	valueModel = pmModelCP_FullSersic (0.0, 0.0, 0.0, pow(10.0, cp->Rmajor), cp->Aratio, cp->Sindex);
	fprintf (stdout, "%f / %f = %f\n", valuePixel, valueModel, valuePixel / valueModel);
	break;

      case 1:
	for (int i = 0; i < cpset->images->n; i++) {
	    cp = cpset->images->data[i];
	    valuePixel = pmModelCP_GetFlux (cp, 0.0, 0.0, 0.0);
	    valueModel = pmModelCP_FullSersic (0.0, 0.0, 0.0, pow(10.0, cp->Rmajor), cp->Aratio, cp->Sindex);
	    fprintf (stdout, "%f / %f = %f | %f  %f  %f\n", valuePixel, valueModel, valuePixel / valueModel, pow(10.0, cp->Rmajor), cp->Aratio, cp->Sindex);
	}
	break;

      case 2:
	for (int i = 0; i < cpset->images->n; i++) {
	    cp = cpset->images->data[i];
	    valuePixel = pmModelCP_GetFlux (cp, 0.0, 0.0, 30.0);
	    valueModel = pmModelCP_FullSersic (0.0, 0.0, 30.0, pow(10.0, cp->Rmajor), cp->Aratio, cp->Sindex);
	    fprintf (stdout, "%f / %f = %f | %f  %f  %f\n", valuePixel, valueModel, valuePixel / valueModel, pow(10.0, cp->Rmajor), cp->Aratio, cp->Sindex);
	}
	break;

      case 3:
	for (int i = 0; i < cpset->images->n; i++) {
	    cp = cpset->images->data[i];
	    valuePixel = pmModelCP_GetFlux (cp, 0.0, 0.5, 0.0);
	    valueModel = pmModelCP_FullSersic (0.0, 0.5, 0.0, pow(10.0, cp->Rmajor), cp->Aratio, cp->Sindex);
	    fprintf (stdout, "%f / %f = %f | %f  %f  %f\n", valuePixel, valueModel, valuePixel / valueModel, pow(10.0, cp->Rmajor), cp->Aratio, cp->Sindex);
	}
	break;

      case 4:
	for (int i = 0; i < cpset->images->n; i++) {
	    cp = cpset->images->data[i];
	    valuePixel = pmModelCP_GetFlux (cp, 0.5, 0.5, 0.0);
	    valueModel = pmModelCP_FullSersic (0.5, 0.5, 0.0, pow(10.0, cp->Rmajor), cp->Aratio, cp->Sindex);
	    fprintf (stdout, "%f / %f = %f | %f  %f  %f\n", valuePixel, valueModel, valuePixel / valueModel, pow(10.0, cp->Rmajor), cp->Aratio, cp->Sindex);
	}
	break;

      case 5:
	for (int i = 0; i < cpset->images->n; i++) {
	    cp = cpset->images->data[i];
	    valuePixel = pmModelCP_GetFlux (cp, -0.5, 0.5, 30.0);
	    valueModel = pmModelCP_FullSersic (-0.5, 0.5, 30.0, pow(10.0, cp->Rmajor), cp->Aratio, cp->Sindex);
	    fprintf (stdout, "%f / %f = %f | %f  %f  %f\n", valuePixel, valueModel, valuePixel / valueModel, pow(10.0, cp->Rmajor), cp->Aratio, cp->Sindex);
	}
	break;

      case 6:
	for (int i = 0; i < cpset->images->n; i++) {
	    cp = cpset->images->data[i];
	    valuePixel = pmModelCP_GetFlux (cp, 0.0, -0.5, 0.0);
	    valueModel = pmModelCP_FullSersic (0.0, -0.5, 0.0, pow(10.0, cp->Rmajor), cp->Aratio, cp->Sindex);
	    fprintf (stdout, "%f / %f = %f | %f  %f  %f\n", valuePixel, valueModel, valuePixel / valueModel, pow(10.0, cp->Rmajor), cp->Aratio, cp->Sindex);
	}
	break;

      case 7:
	cp = cpset->images->data[224];
	float delta = 1.0 / 11.0;
	float offset = -10*delta;
	for (float dx = offset; dx <= 1.0; dx += delta) {
	    valuePixel = pmModelCP_GetFlux (cp, dx, 0.0, 0.0);
	    valueModel = pmModelCP_FullSersic (dx, 0.0, 0.0, pow(10.0, cp->Rmajor), cp->Aratio, cp->Sindex);
	    fprintf (stdout, "%f / %f = %f | %f  %f  %f\n", valuePixel, valueModel, valuePixel / valueModel, pow(10.0, cp->Rmajor), cp->Aratio, cp->Sindex);
	}
	break;

      case 8:
	cp = cpset->images->data[224];
	valuePixel = pmModelCP_GetFlux (cp, 0.0, 0.0, 45.0);
	valueModel = pmModelCP_FullSersic (0.0, 0.0, 45.0, pow(10.0, cp->Rmajor), cp->Aratio, cp->Sindex);
	fprintf (stdout, "%f / %f = %f | %f  %f  %f\n", valuePixel, valueModel, valuePixel / valueModel, pow(10.0, cp->Rmajor), cp->Aratio, cp->Sindex);
	break;

      case 9:
	for (int i = 0; i < cpset->images->n; i++) {
	    cp = cpset->images->data[i];
	    valuePixel = pmModelCP_GetFlux (cp, -0.5, -0.5, 0.0);
	    valueModel = pmModelCP_FullSersic (-0.5, -0.5, 0.0, pow(10.0, cp->Rmajor), cp->Aratio, cp->Sindex);
	    fprintf (stdout, "%f / %f = %f | %f  %f  %f\n", valuePixel, valueModel, valuePixel / valueModel, pow(10.0, cp->Rmajor), cp->Aratio, cp->Sindex);
	}
	break;
      case 10:
	for (int i = 0; i < cpset->images->n; i++) {
	    cp = cpset->images->data[i];
	    valuePixel = pmModelCP_GetFlux (cp, 0.5, 0.0, 0.0);
	    valueModel = pmModelCP_FullSersic (0.5, 0.0, 0.0, pow(10.0, cp->Rmajor), cp->Aratio, cp->Sindex);
	    fprintf (stdout, "%f / %f = %f | %f  %f  %f\n", valuePixel, valueModel, valuePixel / valueModel, pow(10.0, cp->Rmajor), cp->Aratio, cp->Sindex);
	}
	break;

      case 11:
	for (int i = 0; i < cpset->images->n; i++) {
	    cp = cpset->images->data[i];
	    valuePixel = pmModelCP_GetFlux (cp, -0.5, 0.0, 0.0);
	    valueModel = pmModelCP_FullSersic (-0.5, 0.0, 0.0, pow(10.0, cp->Rmajor), cp->Aratio, cp->Sindex);
	    fprintf (stdout, "%f / %f = %f | %f  %f  %f\n", valuePixel, valueModel, valuePixel / valueModel, pow(10.0, cp->Rmajor), cp->Aratio, cp->Sindex);
	}
	break;
      case 12:
	psTimerStart ("test");
	for (int i = 0; i < cpset->images->n; i++) {
	    cp = cpset->images->data[i];
	    valuePixel = pmModelCP_GetFlux (cp, 0.0, 0.0, 0.0);
	}
	fprintf (stderr, "CP code: %f\n", psTimerMark ("test"));
	psTimerStart ("test");
	for (int i = 0; i < cpset->images->n; i++) {
	    cp = cpset->images->data[i];
	    valueModel = pmModelCP_FullSersic (0.0, 0.0, 0.0, pow(10.0, cp->Rmajor), cp->Aratio, cp->Sindex);
	}
	fprintf (stderr, "Full code: %f\n", psTimerMark ("test"));
	break;
    }

    psFree (cpset);
    ok(!psMemCheckLeaks (id, NULL, stderr, false), "no memory leaks");

    return exit_status();
}

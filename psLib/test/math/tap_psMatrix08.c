/** @file  tap_psMatrix_08.c
*
*  @brief psMatrixLUSolve, psMatrixGJSolve tests (for ill-conditioned matrix)
*  @author  Eugene Magnier, IfA
*
*  @version $Revision: 1.2 $  $Name: not supported by cvs2svn $
*  @date  $Date: 2007-05-02 04:20:06 $
*
*  Copyright 2004-2005 Institute for Astronomy, University of Hawaii
*
*/
#include <stdio.h>
#include <string.h>
#include <pslib.h>
#include "tap.h"
#include "pstap.h"

# define DEBUG 1

psS32 main( psS32 argc, char* argv[] )
{
    plan_tests(23);
    // psTraceSetLevel("psLib.math.psMatrixGJSolve", 4);

    // Transpose input image into output image
    {
        psMemId id = psMemGetId();

	psFits *fits = NULL;

	// we have a specific image and vector pair which gave us trouble elsewhere:
	// XXX this is an ill-conditioned matrix.  LU Decomposition does not inform us that it is ill-conditioned.  
	// the result solves the equation, but what are the errors on the values?
	fits = psFitsOpen ("data/Agj.fits", "r");
        ok(fits, "opened test image Agj.fits");

	psImage *Aimage = psFitsReadImage (fits, psRegionSet(0,0,0,0), 0);
        ok(Aimage, "loaded test image Agj.fits");

	psImage *aimage = psImageCopy (NULL, Aimage, Aimage->type.type);
        ok(aimage, "copied test image Agj.fits");

	psFitsClose (fits);

	fits = psFitsOpen ("data/Bgj.fits", "r");
        ok(fits, "opened test image Bgj.fits");

	psImage *Bimage = psFitsReadImage (fits, psRegionSet(0,0,0,0), 0);
        ok(Aimage, "loaded test image Bgj.fits");

	psFitsClose (fits);

	psVector *Bvector = psVectorAlloc (Bimage->numRows, Bimage->type.type);
        ok(Bvector, "allocated B vector");

	for (int i = 0; i < Bimage->numRows; i++) {
	    double value = psImageGet (Bimage, 0, i);
	    psVectorSet (Bvector, i, value);
	}

	bool status;
	status = psMatrixLUSolve(Aimage, Bvector);
        ok(!status, "psMatrixLUSolve correctly returns false for ill-conditioned matrix");

# if (DEBUG)
	fprintf (stderr, "LU Solution:\n");
	for (int i = 0; i < Bimage->numRows; i++) {
	    double value = psVectorGet (Bvector, i);
	    double valerr = psImageGet (Aimage, i, i);
	    fprintf (stderr, "%f +/- %f\n", value, valerr);
	}

	// calculate Ax and compare with B:
	fprintf (stderr, "result:\n");
	for (int i = 0; i < Bimage->numRows; i++) {
	    double value = 0;
	    for (int j = 0; j < Bvector->n; j++) {
		double tmpV = psVectorGet (Bvector, j);
		double tmpI = psImageGet (aimage, j, i);
		value += tmpV*tmpI;
	    }
	    double actual = psImageGet (Bimage, 0, i);
	    fprintf (stderr, "%f vs %f (delta: %f)\n", value, actual, actual - value);
	}
# endif

        psFree(Aimage);
        psFree(Bimage);
        psFree(Bvector);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // Transpose input image into output image
    {
        psMemId id = psMemGetId();

	psFits *fits = NULL;

	// we have a specific ill-conditioned matrix in Agj.fits. psMatrixGJSolve detects this and reports a failure.
	fits = psFitsOpen ("data/Agj.fits", "r");
        ok(fits, "opened test image Agj.fits");

	psImage *Aimage = psFitsReadImage (fits, psRegionSet(0,0,0,0), 0);
        ok(Aimage, "loaded test image Agj.fits");

	psFitsClose (fits);

	fits = psFitsOpen ("data/Bgj.fits", "r");
        ok(fits, "opened test image Bgj.fits");

	psImage *Bimage = psFitsReadImage (fits, psRegionSet(0,0,0,0), 0);
        ok(Bimage, "loaded test image Bgj.fits");

	psFitsClose (fits);

	psVector *Bvector = psVectorAlloc (Bimage->numRows, Bimage->type.type);
        ok(Bvector, "allocated B vector");

	for (int i = 0; i < Bimage->numRows; i++) {
	    double value = psImageGet (Bimage, 0, i);
	    psVectorSet (Bvector, i, value);
	}

	bool status;
	status = psMatrixGJSolve(Aimage, Bvector);
        ok(!status, "psMatrixGJSolve correctly returns false for ill-conditioned matrix");

# if (DEBUG)
	fprintf (stderr, "GJ Solution:\n");
	for (int i = 0; i < Bimage->numRows; i++) {
	    double value = psVectorGet (Bvector, i);
	    fprintf (stderr, "%f\n", value);
	}
# endif

        psFree(Aimage);
        psFree(Bimage);
        psFree(Bvector);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // Transpose input image into output image
    {
        psMemId id = psMemGetId();

	psFits *fits = NULL;

	// we have a specific ill-conditioned matrix in Agj.fits. psMatrixGJSolve detects this and reports a failure.
	fits = psFitsOpen ("data/Agj.fits", "r");
        ok(fits, "opened test image Agj.fits");

	psImage *aimage = psFitsReadImage (fits, psRegionSet(0,0,0,0), 0);
        ok(aimage, "loaded test image Agj.fits");

	psImage *Aimage = psImageCopy (NULL, aimage, PS_TYPE_F64);
        ok(Aimage, "converted test image to F64");

	psFitsClose (fits);

	fits = psFitsOpen ("data/Bgj.fits", "r");
        ok(fits, "opened test image Bgj.fits");

	psImage *bimage = psFitsReadImage (fits, psRegionSet(0,0,0,0), 0);
        ok(bimage, "loaded test image Bgj.fits");

	psImage *Bimage = psImageCopy (NULL, bimage, PS_TYPE_F64);
        ok(Bimage, "converted test image to F64");

	psFitsClose (fits);

	psVector *Bvector = psVectorAlloc (Bimage->numRows, Bimage->type.type);
        ok(Bvector, "allocated B vector");

	for (int i = 0; i < Bimage->numRows; i++) {
	    double value = psImageGet (Bimage, 0, i);
	    psVectorSet (Bvector, i, value);
	}

	bool status;
	status = psMatrixGJSolve(Aimage, Bvector);
        ok(!status, "psMatrixGJSolve correctly returns false for ill-conditioned matrix");

# if (DEBUG)	
	fprintf (stderr, "GJ Solution:\n");
	for (int i = 0; i < Bimage->numRows; i++) {
	    double value = psVectorGet (Bvector, i);
	    double valerr = psImageGet (Aimage, i, i);
	    fprintf (stderr, "%f +/- %f\n", value, valerr);
	}
	
	// calculate Ax and compare with B:
	fprintf (stderr, "result:\n");
	for (int i = 0; i < Bimage->numRows; i++) {
	    double value = 0;
	    for (int j = 0; j < Bvector->n; j++) {
		double tmpV = psVectorGet (Bvector, j);
		double tmpI = psImageGet (aimage, j, i);
		value += tmpV*tmpI;
	    }
	    double actual = psImageGet (Bimage, 0, i);
	    fprintf (stderr, "%f vs %f (delta: %f)\n", value, actual, actual - value);
	}
# endif

        psFree(Aimage);
        psFree(Bimage);
        psFree(aimage);
        psFree(bimage);
        psFree(Bvector);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }
}

#include <stdio.h>
#include <string.h>
#include <pslib.h>

#include "tap.h"
#include "pstap.h"

// save function used to dump out test images while debugging algorithm
int SaveImage (psMetadata *header, psImage *image, char *filename) {

    psFits *fits = psFitsOpen (filename, "w");
    psFitsWriteImage (fits, NULL, image, 0, NULL);
    psFitsClose (fits);
    return (TRUE);
}

# define TEST_4PT_0 1
# define TEST_4PT_1 1
# define TEST_4PT_2 1
# define TEST_4PT_3 1
# define TEST_6PT_0 1
# define TEST_9PT_0 1
# define TEST_9PT_1 1
# define TEST_9PT_2 1
# define TEST_9PT_3 1
# define TEST_9PT_4 1
# define TEST_9PT_5 1

int main (void)
{

    plan_tests(219);
    
    // *** tests to demonstrate the validity of the algorithm or concept ***

    // test with unconstrained cell: 3x3 grid fitted to 8 points with simple slope and scale difference
    # if (TEST_9PT_5)
    {
        psMemId id = psMemGetId();

	psImageBinning *binning = psImageBinningAlloc();
	binning->nXfine = 6;
	binning->nYfine = 6;
	binning->nXruff = 3;
	binning->nYruff = 3;

	// generate a grid of test data points
	psVector *x = psVectorAlloc (50, PS_TYPE_F32);
	psVector *y = psVectorAlloc (50, PS_TYPE_F32);
	psVector *f = psVectorAlloc (50, PS_TYPE_F32);

	// the underlying field is f = ix + iy, where ix,iy are fine pixel coordinates
	// place the measurement points exactly on the ruff reference pixel centers
	int n = 0;
	for (float ix = 1.0; ix < 6.0; ix += 2.0) {
	    for (float iy = 1.0; iy < 6.0; iy += 2.0) {
		if ((ix == 1.0) && (iy == 1.0)) continue;
		x->data.F32[n] = ix;
		y->data.F32[n] = iy;
		f->data.F32[n] = x->data.F32[n] + y->data.F32[n];
		n++;
	    }
	}
	x->n = n;
	y->n = n;
	f->n = n;

	psImage *field = psImageAlloc(6, 6, PS_TYPE_F32);
	for (int ix = 0; ix < 6; ix++) {
	    for (int iy = 0; iy < 6; iy++) {
		field->data.F32[iy][ix] = (ix + 0.5) + (iy + 0.5);
	    }
	}

	psStats *stats = psStatsAlloc (PS_STAT_SAMPLE_MEDIAN);

	// scale defines both field and map image sizes (nXfine, nXruff)
	psImageMap *map = psImageMapAlloc (NULL, binning, stats);

	// fit the data to the map
	psImageMapFit (map, NULL, 0, x, y, f, NULL);

	psImage *model = psImageAlloc(field->numCols, field->numRows, PS_TYPE_F32);
	for (int ix = 0; ix < model->numCols; ix++) {
	    for (int iy = 0; iy < model->numRows; iy++) {
		model->data.F32[iy][ix] = psImageUnbinPixel (ix + 0.5, iy + 0.5, map->map, map->binning);
		if ((ix < 3) && (iy < 3)) {
		    is_float (model->data.F32[iy][ix], NAN, "model matches expected NaN");
		} else {
		    is_float_tol (model->data.F32[iy][ix], field->data.F32[iy][ix], 1e-5, "model matches inputs");
		}
	    }
	}

	// SaveImage (NULL, map->map, "map.fits");
	// SaveImage (NULL, field, "field.fits");
	// SaveImage (NULL, model, "model.fits");

	psFree (model);
	psFree (binning);
	psFree (map);
	psFree (stats);
	psFree (field);
	psFree (x);
	psFree (y);
	psFree (f);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }
    # endif

    // test for more points: 3x3 grid fitted to 9 points with simple slope and scale difference
    # if (TEST_9PT_4)
    {
        psMemId id = psMemGetId();

	psImageBinning *binning = psImageBinningAlloc();
	binning->nXfine = 6;
	binning->nYfine = 6;
	binning->nXruff = 3;
	binning->nYruff = 3;

	// generate a grid of test data points
	psVector *x = psVectorAlloc (50, PS_TYPE_F32);
	psVector *y = psVectorAlloc (50, PS_TYPE_F32);
	psVector *f = psVectorAlloc (50, PS_TYPE_F32);

	// the underlying field is f = ix + iy, where ix,iy are fine pixel coordinates
	// place the measurement points exactly on the ruff reference pixel centers
	int n = 0;
	for (float ix = 0.5; ix < 6.0; ix += 1.0) {
	    for (float iy = 0.5; iy < 6.0; iy += 1.0) {
		x->data.F32[n] = ix;
		y->data.F32[n] = iy;
		f->data.F32[n] = x->data.F32[n] + y->data.F32[n];
		n++;
	    }
	}
	x->n = n;
	y->n = n;
	f->n = n;

	psImage *field = psImageAlloc(6, 6, PS_TYPE_F32);
	for (int ix = 0; ix < 6; ix++) {
	    for (int iy = 0; iy < 6; iy++) {
		field->data.F32[iy][ix] = (ix + 0.5) + (iy + 0.5);
	    }
	}

	psStats *stats = psStatsAlloc (PS_STAT_SAMPLE_MEDIAN);

	// scale defines both field and map image sizes (nXfine, nXruff)
	psImageMap *map = psImageMapAlloc (NULL, binning, stats);

	// fit the data to the map
	psImageMapFit (map, NULL, 0, x, y, f, NULL);

	psImage *model = psImageAlloc(field->numCols, field->numRows, PS_TYPE_F32);
	for (int ix = 0; ix < model->numCols; ix++) {
	    for (int iy = 0; iy < model->numRows; iy++) {
		model->data.F32[iy][ix] = psImageUnbinPixel (ix + 0.5, iy + 0.5, map->map, map->binning);
		is_float_tol (model->data.F32[iy][ix], field->data.F32[iy][ix], 1e-5, "model matches inputs");
	    }
	}

	// SaveImage (NULL, map->map, "map.fits");
	// SaveImage (NULL, field, "field.fits");
	// SaveImage (NULL, model, "model.fits");

	psFree (model);
	psFree (binning);
	psFree (map);
	psFree (stats);
	psFree (field);
	psFree (x);
	psFree (y);
	psFree (f);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }
    # endif

    // test for more points: 3x3 grid fitted to 9 points with simple slope and scale difference
    # if (TEST_9PT_3)
    {
        psMemId id = psMemGetId();

	psImageBinning *binning = psImageBinningAlloc();
	binning->nXfine = 6;
	binning->nYfine = 6;
	binning->nXruff = 3;
	binning->nYruff = 3;

	// generate a grid of test data points
	psVector *x = psVectorAlloc (50, PS_TYPE_F32);
	psVector *y = psVectorAlloc (50, PS_TYPE_F32);
	psVector *f = psVectorAlloc (50, PS_TYPE_F32);

	// the underlying field is f = ix + iy, where ix,iy are fine pixel coordinates
	// place the measurement points exactly on the ruff reference pixel centers
	int n = 0;
	for (float ix = 1.0; ix < 6; ix += 2.0) {
	    for (float iy = 1.0; iy < 6; iy += 2.0) {
		y->data.F32[n] = iy;
		x->data.F32[n] = ix;
		if ((ix == 1.0) && (iy == 1.0)) {
		    x->data.F32[n] = ix - 0.1;
		}
		if ((ix == 3.0) && (iy == 1.0)) {
		    y->data.F32[n] = iy - 0.1;
		}
		if ((ix == 5.0) && (iy == 3.0)) {
		    x->data.F32[n] = ix + 0.1;
		}
		if ((ix == 3.0) && (iy == 5.0)) {
		    y->data.F32[n] = iy + 0.1;
		}
		f->data.F32[n] = x->data.F32[n] + y->data.F32[n];
		n++;
	    }
	}
	x->n = n;
	y->n = n;
	f->n = n;

	psImage *field = psImageAlloc(6, 6, PS_TYPE_F32);
	for (int ix = 0; ix < 6; ix++) {
	    for (int iy = 0; iy < 6; iy++) {
		field->data.F32[iy][ix] = (ix + 0.5) + (iy + 0.5);
	    }
	}

	psStats *stats = psStatsAlloc (PS_STAT_SAMPLE_MEDIAN);

	// scale defines both field and map image sizes (nXfine, nXruff)
	psImageMap *map = psImageMapAlloc (NULL, binning, stats);

	// fit the data to the map
	psImageMapFit (map, NULL, 0, x, y, f, NULL);

	psImage *model = psImageAlloc(field->numCols, field->numRows, PS_TYPE_F32);
	for (int ix = 0; ix < model->numCols; ix++) {
	    for (int iy = 0; iy < model->numRows; iy++) {
		model->data.F32[iy][ix] = psImageUnbinPixel (ix + 0.5, iy + 0.5, map->map, map->binning);
		is_float_tol (model->data.F32[iy][ix], field->data.F32[iy][ix], 1e-5, "model matches inputs");
	    }
	}

	// SaveImage (NULL, map->map, "map.fits");
	// SaveImage (NULL, field, "field.fits");
	// SaveImage (NULL, model, "model.fits");
	
	psFree (model);
	psFree (binning);
	psFree (map);
	psFree (stats);
	psFree (field);
	psFree (x);
	psFree (y);
	psFree (f);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }
    # endif

    // test for more points: 3x3 grid fitted to 9 points with simple slope and scale difference
# if (TEST_9PT_2)
    {
        psMemId id = psMemGetId();

	psImageBinning *binning = psImageBinningAlloc();
	binning->nXfine = 3;
	binning->nYfine = 3;
	binning->nXruff = 3;
	binning->nYruff = 3;

	// generate a grid of test data points
	psVector *x = psVectorAlloc (50, PS_TYPE_F32);
	psVector *y = psVectorAlloc (50, PS_TYPE_F32);
	psVector *f = psVectorAlloc (50, PS_TYPE_F32);

	// the underlying field is f = ix + iy, where ix,iy are fine pixel coordinates
	// place the measurement points exactly on the ruff reference pixel centers
	int n = 0;
	for (float ix = 0.0; ix < 3; ix += 1.0) {
	    for (float iy = 0.0; iy < 3; iy += 1.0) {
		x->data.F32[n] = ix + 0.5;
		y->data.F32[n] = iy + 0.5;

		if (ix == 0.0) {
		    x->data.F32[n] -= 0.1;
		}
		if (iy == 0.0) {
		    y->data.F32[n] -= 0.1;
		}
		if (ix == 2.0) {
		    x->data.F32[n] += 0.1;
		}
		if (iy == 2.0) {
		    y->data.F32[n] += 0.1;
		}
		f->data.F32[n] = x->data.F32[n] + y->data.F32[n];
		n++;
	    }
	}
	x->n = n;
	y->n = n;
	f->n = n;

	psImage *field = psImageAlloc(3, 3, PS_TYPE_F32);
	for (int ix = 0; ix < 3; ix++) {
	    for (int iy = 0; iy < 3; iy++) {
		field->data.F32[iy][ix] = (ix + 0.5) + (iy + 0.5);
	    }
	}

	psStats *stats = psStatsAlloc (PS_STAT_SAMPLE_MEDIAN);

	// scale defines both field and map image sizes (nXfine, nXruff)
	psImageMap *map = psImageMapAlloc (NULL, binning, stats);

	// fit the data to the map
	psImageMapFit (map, NULL, 0, x, y, f, NULL);

	psImage *model = psImageAlloc(field->numCols, field->numRows, PS_TYPE_F32);
	for (int ix = 0; ix < model->numCols; ix++) {
	    for (int iy = 0; iy < model->numRows; iy++) {
		model->data.F32[iy][ix] = psImageUnbinPixel (ix + 0.5, iy + 0.5, map->map, map->binning);
		is_float_tol (model->data.F32[iy][ix], field->data.F32[iy][ix], 1e-5, "model matches inputs");
	    }
	}
	// SaveImage (NULL, map->map, "map.fits");
	// SaveImage (NULL, field, "field.fits");
	// SaveImage (NULL, model, "model.fits");

	psFree (model);
	psFree (binning);
	psFree (map);
	psFree (stats);
	psFree (field);
	psFree (x);
	psFree (y);
	psFree (f);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }
# endif

    // still a simple test: 3x3 grid fitted to 9 points with simple slope and scale difference
    # if (TEST_9PT_1)
    {
        psMemId id = psMemGetId();

	psImageBinning *binning = psImageBinningAlloc();
	binning->nXfine = 6;
	binning->nYfine = 6;
	binning->nXruff = 3;
	binning->nYruff = 3;

	// generate a grid of test data points
	psVector *x = psVectorAlloc (9, PS_TYPE_F32);
	psVector *y = psVectorAlloc (9, PS_TYPE_F32);
	psVector *f = psVectorAlloc (9, PS_TYPE_F32);

	// the underlying field is f = ix + iy, where ix,iy are fine pixel coordinates
	// place the measurement points exactly on the ruff reference pixel centers
	int n = 0;
	for (int ix = 1; ix < 6; ix += 2) {
	    for (int iy = 1; iy < 6; iy += 2) {
		x->data.F32[n] = ix;
		y->data.F32[n] = iy;
		f->data.F32[n] = x->data.F32[n] + y->data.F32[n];
		n++;
	    }
	}

	psImage *field = psImageAlloc(6, 6, PS_TYPE_F32);
	for (int ix = 0; ix < 6; ix++) {
	    for (int iy = 0; iy < 6; iy++) {
		field->data.F32[iy][ix] = (ix + 0.5) + (iy + 0.5);
	    }
	}

	psStats *stats = psStatsAlloc (PS_STAT_SAMPLE_MEDIAN);

	// scale defines both field and map image sizes (nXfine, nXruff)
	psImageMap *map = psImageMapAlloc (NULL, binning, stats);

	// fit the data to the map
	psImageMapFit (map, NULL, 0, x, y, f, NULL);

	psImage *model = psImageAlloc(field->numCols, field->numRows, PS_TYPE_F32);
	for (int ix = 0; ix < model->numCols; ix++) {
	    for (int iy = 0; iy < model->numRows; iy++) {
		model->data.F32[iy][ix] = psImageUnbinPixel (ix + 0.5, iy + 0.5, map->map, map->binning);
		is_float_tol (model->data.F32[iy][ix], field->data.F32[iy][ix], FLT_EPSILON, "model matches inputs");
	    }
	}

	// SaveImage (NULL, map->map, "map.fits");
	// SaveImage (NULL, field, "field.fits");
	// SaveImage (NULL, model, "model.fits");

	psFree (model);
	psFree (binning);
	psFree (map);
	psFree (stats);
	psFree (field);
	psFree (x);
	psFree (y);
	psFree (f);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }
    # endif

    // very simple test: 3x3 grid fitted to 9 points with simple slope
    # if (TEST_9PT_0)
    {
	// function is defined over the range 0-1000, 0-1000
        psMemId id = psMemGetId();

	psImageBinning *binning = psImageBinningAlloc();
	binning->nXfine = 3;
	binning->nYfine = 3;
	binning->nXruff = 3;
	binning->nYruff = 3;

	// generate a grid of test data points
	psVector *x = psVectorAlloc (9, PS_TYPE_F32);
	psVector *y = psVectorAlloc (9, PS_TYPE_F32);
	psVector *f = psVectorAlloc (9, PS_TYPE_F32);

	int n = 0;
	psImage *field = psImageAlloc(3, 3, PS_TYPE_F32);
	for (int ix = 0; ix < 3; ix++) {
	    for (int iy = 0; iy < 3; iy++) {
		x->data.F32[n] = ix + 0.5;
		y->data.F32[n] = iy + 0.5;
		f->data.F32[n] = x->data.F32[n] + y->data.F32[n];
		field->data.F32[iy][ix] = f->data.F32[n];
		n++;
	    }
	}

	psStats *stats = psStatsAlloc (PS_STAT_SAMPLE_MEDIAN);

	// scale defines both field and map image sizes (nXfine, nXruff)
	psImageMap *map = psImageMapAlloc (NULL, binning, stats);

	// fit the data to the map
	psImageMapFit (map, NULL, 0, x, y, f, NULL);

	psImage *model = psImageAlloc(field->numCols, field->numRows, PS_TYPE_F32);
	for (int ix = 0; ix < model->numCols; ix++) {
	    for (int iy = 0; iy < model->numRows; iy++) {
		model->data.F32[iy][ix] = psImageUnbinPixel (ix + 0.5, iy + 0.5, map->map, map->binning);
		is_float_tol (model->data.F32[iy][ix], field->data.F32[iy][ix], FLT_EPSILON, "model matches inputs");
	    }
	}

	// SaveImage (NULL, map->map, "map.fits");
	// SaveImage (NULL, field, "field.fits");
	// SaveImage (NULL, model, "model.fits");

	psFree (model);
	psFree (binning);
	psFree (map);
	psFree (stats);
	psFree (field);
	psFree (x);
	psFree (y);
	psFree (f);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }
    # endif

    // test for more points: 3x3 grid fitted to 9 points with simple slope and scale difference
    # if (TEST_6PT_0)
    {
        psMemId id = psMemGetId();

	psImageBinning *binning = psImageBinningAlloc();
	binning->nXfine = 3;
	binning->nYfine = 2;
	binning->nXruff = 3;
	binning->nYruff = 2;

	// generate a grid of test data points
	psVector *x = psVectorAlloc (50, PS_TYPE_F32);
	psVector *y = psVectorAlloc (50, PS_TYPE_F32);
	psVector *f = psVectorAlloc (50, PS_TYPE_F32);

	// the underlying field is f = ix + iy, where ix,iy are fine pixel coordinates
	// place the measurement points exactly on the ruff reference pixel centers
	int n = 0;
	for (float ix = 0.5; ix < 3.0; ix += 1.0) {
	    for (float iy = 0.5; iy < 2.0; iy += 1.0) {
		// x->data.F32[n] = ix;
		// y->data.F32[n] = iy;
		if ((ix == 0.5) && (iy == 0.5)) {
		    x->data.F32[n] = ix + 0.0;
		    y->data.F32[n] = iy + 0.1; // add in both points.  
		    // f->data.F32[n] = x->data.F32[n] + y->data.F32[n];
		    // n++;
		    // x->data.F32[n] = ix + 0.0;
		    // y->data.F32[n] = iy - 0.1;
		} else {
		    x->data.F32[n] = ix;
		    y->data.F32[n] = iy;
		}
		f->data.F32[n] = x->data.F32[n] + y->data.F32[n];
		n++;
	    }
	}
	x->n = n;
	y->n = n;
	f->n = n;

	psImage *field = psImageAlloc(3, 2, PS_TYPE_F32);
	for (int ix = 0; ix < 3; ix++) {
	    for (int iy = 0; iy < 2; iy++) {
		field->data.F32[iy][ix] = (ix + 0.5) + (iy + 0.5);
	    }
	}

	psStats *stats = psStatsAlloc (PS_STAT_SAMPLE_MEDIAN);

	// scale defines both field and map image sizes (nXfine, nXruff)
	psImageMap *map = psImageMapAlloc (NULL, binning, stats);

	// fit the data to the map
	psImageMapFit (map, NULL, 0, x, y, f, NULL);

	psImage *model = psImageAlloc(field->numCols, field->numRows, PS_TYPE_F32);
	for (int ix = 0; ix < model->numCols; ix++) {
	    for (int iy = 0; iy < model->numRows; iy++) {
		model->data.F32[iy][ix] = psImageUnbinPixel (ix + 0.5, iy + 0.5, map->map, map->binning);
		is_float_tol (model->data.F32[iy][ix], field->data.F32[iy][ix], FLT_EPSILON, "model matches inputs");
	    }
	}

	// SaveImage (NULL, map->map, "map.fits");
	// SaveImage (NULL, field, "field.fits");
	// SaveImage (NULL, model, "model.fits");

	psFree (model);
	psFree (binning);
	psFree (map);
	psFree (stats);
	psFree (field);
	psFree (x);
	psFree (y);
	psFree (f);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }
    # endif

    // more complex test: 2x2 grid fitted to 16 points with simple slope offset from grid centers
    // this one uses points inset relative to the reference points so they are all visible
    // to the reference
    # if (TEST_4PT_3)
    {
	// function is defined over the range 0-1000, 0-1000
        psMemId id = psMemGetId();

	psImageBinning *binning = psImageBinningAlloc();
	binning->nXfine = 4;
	binning->nYfine = 4;
	binning->nXruff = 2;
	binning->nYruff = 2;

	// generate a grid of test data points
	psVector *x = psVectorAlloc (16, PS_TYPE_F32);
	psVector *y = psVectorAlloc (16, PS_TYPE_F32);
	psVector *f = psVectorAlloc (16, PS_TYPE_F32);

	int n = 0;
	// actual field is f = x + y, where x & y are the subpixel positions
	for (float ix = 0.5; ix < 4; ix += 1.0) {
	    for (float iy = 0.5; iy < 4; iy += 1.0) {
		x->data.F32[n] = ix;
		y->data.F32[n] = iy;
		f->data.F32[n] = x->data.F32[n] + y->data.F32[n];
		n++;
	    }
	}

	psImage *field = psImageAlloc(4, 4, PS_TYPE_F32);
	for (int ix = 0; ix < 4; ix++) {
	    for (int iy = 0; iy < 4; iy++) {
		field->data.F32[iy][ix] = (ix + 0.5) + (iy + 0.5);
	    }
	}

	psStats *stats = psStatsAlloc (PS_STAT_SAMPLE_MEDIAN);

	// scale defines both field and map image sizes (nXfine, nXruff)
	psImageMap *map = psImageMapAlloc (NULL, binning, stats);

	// fit the data to the map
	psImageMapFit (map, NULL, 0, x, y, f, NULL);

	psImage *model = psImageAlloc(field->numCols, field->numRows, PS_TYPE_F32);
	for (int ix = 0; ix < model->numCols; ix++) {
	    for (int iy = 0; iy < model->numRows; iy++) {
		model->data.F32[iy][ix] = psImageUnbinPixel (ix + 0.5, iy + 0.5, map->map, map->binning);
		is_float_tol (model->data.F32[iy][ix], field->data.F32[iy][ix], 10*FLT_EPSILON, "model matches inputs");
	    }
	}

	// SaveImage (NULL, map->map, "map.fits");
	// SaveImage (NULL, field, "field.fits");
	// SaveImage (NULL, model, "model.fits");

	psFree (model);
	psFree (binning);
	psFree (map);
	psFree (stats);
	psFree (field);
	psFree (x);
	psFree (y);
	psFree (f);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }
    # endif

    // more complex test: 2x2 grid fitted to 4 points with simple slope offset from grid centers
    // this one uses points inset relative to the reference points so they are all visible
    // to the reference
    # if (TEST_4PT_2)
    {
	// function is defined over the range 0-1000, 0-1000
        psMemId id = psMemGetId();

	psImageBinning *binning = psImageBinningAlloc();
	binning->nXfine = 2;
	binning->nYfine = 2;
	binning->nXruff = 2;
	binning->nYruff = 2;

	// generate a grid of test data points
	psVector *x = psVectorAlloc (5, PS_TYPE_F32);
	psVector *y = psVectorAlloc (5, PS_TYPE_F32);
	psVector *f = psVectorAlloc (5, PS_TYPE_F32);

	int n = 0;
	psImage *field = psImageAlloc(2, 2, PS_TYPE_F32);
	// actual field is f = x + y, where x & y are the subpixel positions
	for (int ix = 0; ix < 2; ix++) {
	    for (int iy = 0; iy < 2; iy++) {
		# if (1)
		if (!ix && !iy) {
		    x->data.F32[n] = ix + 0.5 - 0.1;
		    y->data.F32[n] = iy + 0.5 - 0.0;
// 		    f->data.F32[n] = x->data.F32[n] + y->data.F32[n];
// 		    n++;
// 		    x->data.F32[n] = ix + 0.5;
//		    y->data.F32[n] = iy + 0.5 - 0.1;
		} else {
		    x->data.F32[n] = ix + 0.5;
		    y->data.F32[n] = iy + 0.5;
		}
		# endif
		# if (0)
		// offset points from centers
		if (ix) {
		    x->data.F32[n] = ix + 0.5 - 0.1;
		} else {
		    x->data.F32[n] = ix + 0.5 + 0.1;
		}
		if (iy) {
		    y->data.F32[n] = iy + 0.5 - 0.1;
		} else {
		    y->data.F32[n] = iy + 0.5 + 0.1;
		}
		# endif
		f->data.F32[n] = x->data.F32[n] + y->data.F32[n];
		field->data.F32[iy][ix] = ix + 0.5 + iy + 0.5;
		n++;
	    }
	}
	x->n = n;
	y->n = n;
	f->n = n;

	psStats *stats = psStatsAlloc (PS_STAT_SAMPLE_MEDIAN);

	// scale defines both field and map image sizes (nXfine, nXruff)
	psImageMap *map = psImageMapAlloc (NULL, binning, stats);

	// fit the data to the map
	psImageMapFit (map, NULL, 0, x, y, f, NULL);

	psImage *model = psImageAlloc(field->numCols, field->numRows, PS_TYPE_F32);
	for (int ix = 0; ix < model->numCols; ix++) {
	    for (int iy = 0; iy < model->numRows; iy++) {
		model->data.F32[iy][ix] = psImageUnbinPixel (ix + 0.5, iy + 0.5, map->map, map->binning);
		is_float_tol (model->data.F32[iy][ix], field->data.F32[iy][ix], 5*FLT_EPSILON, "model matches inputs");
	    }
	}

	// SaveImage (NULL, map->map, "map.fits");
	// SaveImage (NULL, field, "field.fits");
	// SaveImage (NULL, model, "model.fits");

	psFree (model);
	psFree (binning);
	psFree (map);
	psFree (stats);
	psFree (field);
	psFree (x);
	psFree (y);
	psFree (f);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }
    # endif

    // still a simple test: 2x2 grid fitted to 4 points with simple slope and scale difference
    # if (TEST_4PT_1)
    {
	// function is defined over the range 0-1000, 0-1000
        psMemId id = psMemGetId();

	psImageBinning *binning = psImageBinningAlloc();
	binning->nXfine = 4;
	binning->nYfine = 4;
	binning->nXruff = 2;
	binning->nYruff = 2;

	// generate a grid of test data points
	psVector *x = psVectorAlloc (4, PS_TYPE_F32);
	psVector *y = psVectorAlloc (4, PS_TYPE_F32);
	psVector *f = psVectorAlloc (4, PS_TYPE_F32);

	// the underlying field is f = ix + iy, where ix,iy are fine pixel coordinates
	int n = 0;
	for (int ix = 1; ix < 4; ix += 2) {
	    for (int iy = 1; iy < 4; iy += 2) {
		x->data.F32[n] = ix;
		y->data.F32[n] = iy;
		f->data.F32[n] = x->data.F32[n] + y->data.F32[n];
		n++;
	    }
	}

	psImage *field = psImageAlloc(4, 4, PS_TYPE_F32);
	for (int ix = 0; ix < 4; ix++) {
	    for (int iy = 0; iy < 4; iy++) {
		field->data.F32[iy][ix] = (ix + 0.5) + (iy + 0.5);
	    }
	}

	psStats *stats = psStatsAlloc (PS_STAT_SAMPLE_MEDIAN);

	// scale defines both field and map image sizes (nXfine, nXruff)
	psImageMap *map = psImageMapAlloc (NULL, binning, stats);

	// fit the data to the map
	psImageMapFit (map, NULL, 0, x, y, f, NULL);

	psImage *model = psImageAlloc(field->numCols, field->numRows, PS_TYPE_F32);
	for (int ix = 0; ix < model->numCols; ix++) {
	    for (int iy = 0; iy < model->numRows; iy++) {
		model->data.F32[iy][ix] = psImageUnbinPixel (ix + 0.5, iy + 0.5, map->map, map->binning);
		is_float_tol (model->data.F32[iy][ix], field->data.F32[iy][ix], FLT_EPSILON, "model matches inputs");
	    }
	}

	// SaveImage (NULL, map->map, "map.fits");
	// SaveImage (NULL, field, "field.fits");
	// SaveImage (NULL, model, "model.fits");

	psFree (model);
	psFree (binning);
	psFree (map);
	psFree (stats);
	psFree (field);
	psFree (x);
	psFree (y);
	psFree (f);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }
    # endif

    // simplest possible test: 2x2 grid fitted to 4 points with simple slope
    # if (TEST_4PT_0)
    {
	// function is defined over the range 0-1000, 0-1000
        psMemId id = psMemGetId();

	psImageBinning *binning = psImageBinningAlloc();
	binning->nXfine = 2;
	binning->nYfine = 2;
	binning->nXruff = 2;
	binning->nYruff = 2;

	// generate a grid of test data points
	psVector *x = psVectorAlloc (4, PS_TYPE_F32);
	psVector *y = psVectorAlloc (4, PS_TYPE_F32);
	psVector *f = psVectorAlloc (4, PS_TYPE_F32);

	int n = 0;
	psImage *field = psImageAlloc(2, 2, PS_TYPE_F32);
	for (int ix = 0; ix < 2; ix++) {
	    for (int iy = 0; iy < 2; iy++) {
		x->data.F32[n] = ix + 0.5;
		y->data.F32[n] = iy + 0.5;
		f->data.F32[n] = x->data.F32[n] + y->data.F32[n];
		field->data.F32[iy][ix] = f->data.F32[n];
		n++;
	    }
	}

	psStats *stats = psStatsAlloc (PS_STAT_SAMPLE_MEDIAN);

	// scale defines both field and map image sizes (nXfine, nXruff)
	psImageMap *map = psImageMapAlloc (NULL, binning, stats);

	// fit the data to the map
	psImageMapFit (map, NULL, 0, x, y, f, NULL);

	psImage *model = psImageAlloc(field->numCols, field->numRows, PS_TYPE_F32);
	for (int ix = 0; ix < model->numCols; ix++) {
	    for (int iy = 0; iy < model->numRows; iy++) {
		model->data.F32[iy][ix] = psImageUnbinPixel (ix + 0.5, iy + 0.5, map->map, map->binning);
		is_float_tol (model->data.F32[iy][ix], field->data.F32[iy][ix], FLT_EPSILON, "model matches inputs");
	    }
	}

	// SaveImage (NULL, map->map, "map.fits");
	// SaveImage (NULL, field, "field.fits");
	// SaveImage (NULL, model, "model.fits");

	psFree (model);
	psFree (binning);
	psFree (map);
	psFree (stats);
	psFree (field);
	psFree (x);
	psFree (y);
	psFree (f);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }
    # endif

    return exit_status();
}   

#include <stdio.h>
#include <string.h>
#include <pslib.h>

#include "tap.h"
#include "pstap.h"

int SaveImage (psMetadata *header, psImage *image, char *filename) {

    psFits *fits = psFitsOpen (filename, "w");
    psFitsWriteImage (fits, NULL, image, 0, NULL);
    psFitsClose (fits);
    return (TRUE);
}

# define TEST1 0
# define TEST2 1
# define TEST3 0

# define C00 +0.0
# define C01 -2.0
# define C10 +1.0

int main (void)
{

    plan_tests(1);

    // make a model for a well-sampled field of a simple function (f = ax + by + c)
    # if (TEST1)
    {
        psMemId id = psMemGetId();

	// function is defined over the range 0-1000, 0-1000
	psImageBinning *binning = psImageBinningAlloc();
	binning->nXfine = 1000;
	binning->nYfine = 1000;
	binning->nXruff = 10;
	binning->nYruff = 10;

	// generate a grid of test data points
	psVector *x = psVectorAllocEmpty (100, PS_TYPE_F32);
	psVector *y = psVectorAllocEmpty (100, PS_TYPE_F32);
	psVector *f = psVectorAllocEmpty (100, PS_TYPE_F32);

	for (int ix = 0; ix < 1000; ix += 20) {
	    for (int iy = 0; iy < 1000; iy += 20) {
		x->data.F32[x->n] = ix;
		y->data.F32[y->n] = iy;
		f->data.F32[f->n] = C00 + C10*ix + C01*iy;
		psVectorExtend (x, 100, 1);
		psVectorExtend (y, 100, 1);
		psVectorExtend (f, 100, 1);
	    }
	}

	psStats *stats = psStatsAlloc (PS_STAT_SAMPLE_MEDIAN);

	// scale defines both field and map image sizes (nXfine, nXruff)
	psImageMap *map = psImageMapAlloc (NULL, binning, stats);

	// fit the data to the map
	psImageMapFit (map, x, y, f, NULL);

	psImage *field = psImageAlloc(1000, 1000, PS_TYPE_F32);
	for (int ix = 0; ix < 1000; ix++) {
	    for (int iy = 0; iy < 1000; iy++) {
		field->data.F32[iy][ix] = C00 + C10*ix + C01*iy;
	    }
	}

	// measure difference between model (map) and data (field)
	for (int ix = 0; ix < map->map->numCols; ix++) {
	    for (int iy = 0; iy < map->map->numRows; iy++) {
		int xo = psImageBinningGetFineX(map->binning, ix + 0.5);
		int yo = psImageBinningGetFineY(map->binning, iy + 0.5);
		is_float_tol (field->data.F32[yo][xo], map->map->data.F32[iy][ix], 1e-3, "model matches data");
	    }
	}

	// XXX test on the model or don't bother
	psImage *model = psImageAlloc(1000, 1000, PS_TYPE_F32);
	for (int ix = 0; ix < 1000; ix++) {
	    for (int iy = 0; iy < 1000; iy++) {
		model->data.F32[iy][ix] = psImageUnbinPixel (ix + 0.5, iy + 0.5, map->map, map->binning);
	    }
	}

	// SaveImage (NULL, model, "model.fits");
	// SaveImage (NULL, field, "field.fits");
	// SaveImage (NULL, map->map, "map.fits");
	
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

    // make a model for a poorly-sampled field of a simple function (f = ax + by + c)

    // choose a model scale for a poorly-sampled field of a simple function (f = ax + by + c)

    // make a model for a well-sampled field of a high-order polynomial function (f = (a(x-xo)^2 + b(y-yo)^2) + c)
    # if (TEST2)
    {
	// function is defined over the range 0-1000, 0-1000
	psImageBinning *binning = psImageBinningAlloc();
	binning->nXfine = 1000;
	binning->nYfine = 1000;
	binning->nXruff = 5;
	binning->nYruff = 5;

	// generate a grid of test data points
	psVector *x = psVectorAllocEmpty (100, PS_TYPE_F32);
	psVector *y = psVectorAllocEmpty (100, PS_TYPE_F32);
	psVector *f = psVectorAllocEmpty (100, PS_TYPE_F32);

	for (int ix = 0; ix < 1000; ix += 20) {
	    for (int iy = 0; iy < 1000; iy += 20) {
		x->data.F32[x->n] = ix + 0,5;
		y->data.F32[y->n] = iy + 0.5;
		f->data.F32[f->n] = PS_SQR(x->data.F32[x->n]) + PS_SQR(y->data.F32[x->n]);
		psVectorExtend (x, 100, 1);
		psVectorExtend (y, 100, 1);
		psVectorExtend (f, 100, 1);
	    }
	}

	psStats *stats = psStatsAlloc (PS_STAT_SAMPLE_MEDIAN);

	// scale defines both field and map image sizes (nXfine, nXruff)
	psImageMap *map = psImageMapAlloc (NULL, binning, stats);

	// allocate a map, but we have no field image to supply
	// XXX this function needs to correct for the mean superpixel position 
	// which is sampled...
	psImageMapFit (map, NULL, 0, x, y, f, NULL);
	psFree (binning);

	SaveImage (NULL, map->map, "map.fits");
	
	psImage *field = psImageAlloc(1000, 1000, PS_TYPE_F32);
	for (int ix = 0; ix < 1000; ix++) {
	    for (int iy = 0; iy < 1000; iy++) {
		field->data.F32[iy][ix] = PS_SQR(ix+0.5) + PS_SQR(iy+0.5);
	    }
	}
	SaveImage (NULL, field, "field.fits");

	// measure difference between model (map) and data (field)
	for (int ix = 0; ix < map->map->numCols; ix++) {
	    for (int iy = 0; iy < map->map->numRows; iy++) {
		int xo = psImageBinningGetFineX(map->binning, ix + 0.5);
		int yo = psImageBinningGetFineY(map->binning, iy + 0.5);
		float df = field->data.F32[yo][xo] - map->map->data.F32[iy][ix];
		fprintf (stderr, "%d %d -> %d %d  :  %f = %f - %f\n", ix, iy, xo, yo, df, field->data.F32[yo][xo], map->map->data.F32[iy][ix]);
	    }
	}

	// XXX test on the model or don't bother
	psImage *model = psImageAlloc(1000, 1000, PS_TYPE_F32);
	for (int ix = 0; ix < 1000; ix++) {
	    for (int iy = 0; iy < 1000; iy++) {
		model->data.F32[iy][ix] = psImageUnbinPixel (ix + 0.5, iy + 0.5, map->map, map->binning);
	    }
	}
	SaveImage (NULL, model, "model.fits");
    }
    # endif

    // make a model for a well-sampled field of a high-order polynomial function (f = (a(x-xo)^2 + b(y-yo)^2) + c)
    # if (TEST3)
    {
	// function is defined over the range 0-1000, 0-1000
	psImageBinning *binning = psImageBinningAlloc();
	binning->nXfine = 1000;
	binning->nYfine = 1000;
	binning->nXruff = 5;
	binning->nYruff = 5;

	// generate a grid of test data points
	psVector *x = psVectorAllocEmpty (100, PS_TYPE_F32);
	psVector *y = psVectorAllocEmpty (100, PS_TYPE_F32);
	psVector *f = psVectorAllocEmpty (100, PS_TYPE_F32);

	for (int ix = 0; ix < 1000; ix += 20) {
	    for (int iy = 0; iy < 1000; iy += 20) {
		x->data.F32[x->n] = ix;
		y->data.F32[y->n] = iy;
		f->data.F32[f->n] = C10*(ix-500) + PS_SQR(C01*(iy-500));
		psVectorExtend (x, 100, 1);
		psVectorExtend (y, 100, 1);
		psVectorExtend (f, 100, 1);
	    }
	}

	psStats *stats = psStatsAlloc (PS_STAT_SAMPLE_MEDIAN);

	// scale defines both field and map image sizes (nXfine, nXruff)
	psImageMap *map = psImageMapAlloc (NULL, binning, stats);

	// allocate a map, but we have no field image to supply
	// XXX this function needs to correct for the mean superpixel position 
	// which is sampled...
	psImageMapGenerate (map, x, y, f, 0.1);
	psFree (binning);

	fprintf (stderr, "nGood: %d\n", map->nGood);
	fprintf (stderr, "nPoor: %d\n", map->nPoor);
	fprintf (stderr, "nBad:  %d\n", map->nBad);
	
	SaveImage (NULL, map->map, "map.fits");
	
	psImage *field = psImageAlloc(1000, 1000, PS_TYPE_F32);
	for (int ix = 0; ix < 1000; ix++) {
	    for (int iy = 0; iy < 1000; iy++) {
		field->data.F32[iy][ix] = C10*(ix-500) + PS_SQR(C01*(iy-500));
	    }
	}
	SaveImage (NULL, field, "field.fits");

	// measure difference between model (map) and data (field)
	for (int ix = 0; ix < map->map->numCols; ix++) {
	    for (int iy = 0; iy < map->map->numRows; iy++) {
		int xo = psImageBinningGetFineX(map->binning, ix + 0.5);
		int yo = psImageBinningGetFineY(map->binning, iy + 0.5);
		float df = field->data.F32[yo][xo] - map->map->data.F32[iy][ix];
		fprintf (stderr, "%d %d -> %d %d  :  %f = %f - %f\n", ix, iy, xo, yo, df, field->data.F32[yo][xo], map->map->data.F32[iy][ix]);
	    }
	}


	psImage *model = psImageAlloc(1000, 1000, PS_TYPE_F32);
	for (int ix = 0; ix < 1000; ix++) {
	    for (int iy = 0; iy < 1000; iy++) {
		// XXX the binned coordinates are probably off by 0.5 pix, or we are interpolating 
		// to the wrong binned coordinate.  
		// XXX fix edge cases
		model->data.F32[iy][ix] = psImageUnbinPixel (ix + 0.5, iy + 0.5, map->map, map->binning);
	    }
	}
	SaveImage (NULL, model, "model.fits");
    }
    # endif

    // make a model for a poorly-sampled field of a non-polynomial function (f = (a(x-xo)^2 + b(y-yo)^2)^-1)

    // choose a model scale for a poorly-sampled field of a non-polynomial function (f = (a(x-xo)^2 + b(y-yo)^2)^-1)
    
    return exit_status();
}

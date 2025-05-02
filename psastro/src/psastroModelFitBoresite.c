/** @file psastroModelFitBoresite.c
 *
 *  @brief
 *
 *  @ingroup psastroModel
 *
 *  @author IfA
 *  @version $Revision: 1.5 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-02-07 02:03:34 $
 *  Copyright 2009 Institute for Astronomy, University of Hawaii
 */

# include "psastroStandAlone.h"

/** 
 * we now have a set of observed L,M values.  fit these to the boresite model
 */
psVector *psastroModelFitBoresite (psVector *Xo, psVector *Yo, psVector *Po, char *outroot) {

    assert (Xo->n > 2);
    assert (Xo->n == Yo->n);
    assert (Xo->n == Po->n);

    // arrays to hold the data to be fitted
    psArray *x = psArrayAlloc(2*Xo->n);
    psVector *y = psVectorAlloc(2*Xo->n, PS_TYPE_F32);

    int n = 0;
    for (int i = 0; i < Xo->n; i++) {

	psVector *coord = NULL;

	// X coordinate value
	coord = psVectorAlloc (2, PS_TYPE_F32);
	coord->data.F32[1] = 0.0;
	coord->data.F32[0] = Po->data.F32[i];
	x->data[n] = coord;
	y->data.F32[n] = Xo->data.F32[i];
	n++;
	
	// Y coordinate value
	coord = psVectorAlloc (2, PS_TYPE_F32);
	coord->data.F32[1] = 1.0;
	coord->data.F32[0] = Po->data.F32[i];
	x->data[n] = coord;
	y->data.F32[n] = Yo->data.F32[i];
	n++;
    }	
    assert (x->n == n);
    assert (y->n == n);

    psVector *params = psVectorAlloc (6, PS_TYPE_F32);
    
    // create the minimization constraints
    psMinConstraint *constraint = psMinConstraintAlloc();

    // XXX for now, no parameter masks, skip checkLimits
    // constraint->checkLimits = psastroModelBoresiteLimits;

    // make an initial guess:
    psStats *stats = psStatsAlloc (PS_STAT_SAMPLE_MEAN | PS_STAT_MAX | PS_STAT_MIN);

    // center (Xo) = mean(Xo), RX = range / 2
    if (!psVectorStats (stats, Xo, NULL, NULL, 0)) {
	psError(PS_ERR_UNKNOWN, false, "failure to measure stats");
	return NULL;
    }
    params->data.F32[PAR_X0] = stats->sampleMean;
    params->data.F32[PAR_RX] = (stats->max - stats->min) / 2.0;

    // center (Yo) = mean(Yo), RY = range / 2
    if (!psVectorStats (stats, Yo, NULL, NULL, 0)) {
	psError(PS_ERR_UNKNOWN, false, "failure to measure stats");
	return NULL;
    }
    params->data.F32[PAR_Y0] = stats->sampleMean;
    params->data.F32[PAR_RY] = (stats->max - stats->min) / 2.0;

    params->data.F32[PAR_P0] = 0.0;
    params->data.F32[PAR_T0] = 0.0;

    psMinimization *myMin = psMinimizationAlloc (25, 0.001, 1.0);
    psImage *covar = psImageAlloc (params->n, params->n, PS_TYPE_F32);
    
    // fprintf (stderr, "guess values:\n");
    // fprintf (stderr, "Xo:  %f\n", params->data.F32[PAR_X0]);
    // fprintf (stderr, "Yo:  %f\n", params->data.F32[PAR_Y0]);
    // fprintf (stderr, "RX:  %f\n", params->data.F32[PAR_RX]);
    // fprintf (stderr, "RY:  %f\n", params->data.F32[PAR_RY]);
    // fprintf (stderr, "P0:  %f\n", params->data.F32[PAR_P0]);
    // fprintf (stderr, "T0:  %f\n", params->data.F32[PAR_T0]);

    // XXX skip the weights for now
    psMinimizeLMChi2(myMin, covar, params, constraint, x, y, NULL, psastroModelBoresite);

    char filename[256];
    snprintf (filename, 256, "%s.pars", outroot);
    FILE *outfile = fopen (filename, "w");
    if (!outfile) {
        psAbort("cannot open %s for output", filename);
    }

    fprintf (outfile, "# fitted values:\n");
    fprintf (outfile, "Xo:  %f\n", params->data.F32[PAR_X0]);
    fprintf (outfile, "Yo:  %f\n", params->data.F32[PAR_Y0]);
    fprintf (outfile, "RX:  %f\n", params->data.F32[PAR_RX]);
    fprintf (outfile, "RY:  %f\n", params->data.F32[PAR_RY]);
    fprintf (outfile, "P0:  %f\n", params->data.F32[PAR_P0]);
    fprintf (outfile, "T0:  %f\n", params->data.F32[PAR_T0]);
    fclose (outfile);

    return params;
}

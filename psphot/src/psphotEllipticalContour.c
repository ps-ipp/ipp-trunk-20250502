# include "psphotInternal.h"

// model parameters
enum {PAR_PHI, PAR_EPSILON, PAR_RMIN};
psF32 psphotEllipticalContourFunc (psVector *deriv, const psVector *params, const psVector *coord);

bool psphotEllipticalContour (pmSource *source) {

    psAssert (source, "missing source");
    psAssert (source->extpars, "missing extpars");
    psAssert (source->extpars->radFlux, "missing radFlux");

    pmSourceRadialFlux *profile = source->extpars->radFlux;
    pmSourceExtendedPars *extpars = source->extpars;

    // use LMM to fit theta vs radius to an ellipse
    psVector *theta = profile->theta;
    psVector *radius = profile->isophotalRadii;

    // find Rmin and Rmax for the initial guess
    float Rmin = radius->data.F32[0];
    float Rmax = radius->data.F32[0];

    // arrays to hold the data to be fitted
    // we fit x and y vs theta in separate passes.
    psArray *x = psArrayAllocEmpty(2*radius->n);
    psVector *y = psVectorAllocEmpty(2*radius->n, PS_TYPE_F32);
    psVector *yErr = psVectorAllocEmpty(2*radius->n, PS_TYPE_F32);

    int n = 0;
    for (int i = 0; i < radius->n; i++) {
	if (!isfinite(radius->data.F32[i])) continue;

	psVector *coord = NULL;

	// Rx coordinate value
	coord = psVectorAlloc (2, PS_TYPE_F32);
	coord->data.F32[1] = 0.0;
	coord->data.F32[0] = theta->data.F32[i];
	x->data[n] = coord;
	y->data.F32[n] = radius->data.F32[i]*cos(theta->data.F32[i]);
	yErr->data.F32[n] = 1000.0;
	n++;

	// Ry coordinate value
	coord = psVectorAlloc (2, PS_TYPE_F32);
	coord->data.F32[1] = 1.0;
	coord->data.F32[0] = theta->data.F32[i];
	x->data[n] = coord;
	y->data.F32[n] = radius->data.F32[i]*sin(theta->data.F32[i]);
	yErr->data.F32[n] = 1000.0;
	n++;

	// check the radius range
	Rmin = MIN (Rmin, radius->data.F32[i]);
	Rmax = MAX (Rmax, radius->data.F32[i]);
    }	
    x->n = n;
    y->n = n;
    yErr->n = n;

    if (n < 4) {
	psFree (x);
	psFree (y);
	psFree (yErr);
	source->mode2 |= PM_SOURCE_MODE2_ECONTOUR_FEW_PTS;
	return false;
    }

    psVector *params = psVectorAlloc (3, PS_TYPE_F32);
    
    // psTraceSetLevel ("psLib.math.psMinimizeLMChi2", 7);
    
    // create the minimization constraints
    psMinConstraint *constraint = psMinConstraintAlloc();

    // XXX for now, no parameter masks, skip checkLimits
    // XXX might help to add a limit to the angle or re-parameterize the ellipse in terms of Rxx, Ryy, Rxy
    // constraint->checkLimits = psastroModelBoresiteLimits;

    params->data.F32[PAR_PHI]     = 0.0;
    params->data.F32[PAR_EPSILON] = Rmin / Rmax;
    params->data.F32[PAR_RMIN]    = Rmin;

    psMinimization *myMin = psMinimizationAlloc (25, 0.01, 1.00);
    psImage *covar = psImageAlloc (params->n, params->n, PS_TYPE_F32);
    
    // XXX skip the weights for now
    psMinimizeLMChi2(myMin, covar, params, constraint, x, y, yErr, psphotEllipticalContourFunc);

    /// XXX rationalize? if epsilon > 1, flip major and minor axes (rotate by 90 degrees)
    if (params->data.F32[PAR_EPSILON] < 1.0) {
	extpars->axes.major = params->data.F32[PAR_RMIN] / params->data.F32[PAR_EPSILON];
	extpars->axes.minor = params->data.F32[PAR_RMIN];
	extpars->axes.theta = params->data.F32[PAR_PHI];
    } else {
	extpars->axes.major = params->data.F32[PAR_RMIN];
	extpars->axes.minor = params->data.F32[PAR_RMIN] / params->data.F32[PAR_EPSILON];
	extpars->axes.theta = params->data.F32[PAR_PHI] + 0.5*M_PI;
    }

    psTrace ("psphot", 4, "# fitted values:\n");
    psTrace ("psphot", 4, "Phi:   %f\n", extpars->axes.theta*PS_DEG_RAD);
    psTrace ("psphot", 4, "Rmaj:  %f\n", extpars->axes.major);
    psTrace ("psphot", 4, "Rmin:  %f\n", extpars->axes.minor);
    
    // show the results
    // psphotPetrosianVisualEllipticalContour (profile, extpars);

    psFree (x);
    psFree (y);
    psFree (yErr);
    psFree (params);
    psFree (covar);
    psFree (myMin);
    psFree (constraint);

    return true;
}

/**
 * the full chisq is built of two associated sums over coordinates:
 * chisq = sum ((Rx_obs - Rx_fit(t))^2 + (Ry_obs - Ry_fit(t))^2)
 * we use split this into a 2x long vector and use coord[1] to distinguish the X and Y terms:
 * coord[0] = measured X or measured Y
 * coord[1] =          0 or          1
 */
psF32 psphotEllipticalContourFunc (psVector *deriv, const psVector *params, const psVector *coord) {

    psF32 *par = params->data.F32;

    float alpha = coord->data.F32[0];

    float cs_alpha = cos(alpha);
    float sn_alpha = sin(alpha);

    float cs_phi = cos(alpha - par[PAR_PHI]);
    float sn_phi = sin(alpha - par[PAR_PHI]);

    float r     = 1.0 / sqrt(SQ(sn_phi) + SQ(par[PAR_EPSILON]*cs_phi));
    float r3    = pow(r, 3.0);
    float drdE  = -0.5 * r3 * SQ(cs_phi) * 2.0 * par[PAR_EPSILON];
    float drdP  = -0.5 * r3 * (SQ(par[PAR_EPSILON]) - 1) * 2.0 * cs_phi * sn_phi;

    // value is X
    if (coord->data.F32[1] < 0.5) {
	float value = par[PAR_RMIN]*cs_alpha*r;

	if (deriv) {
	    psF32 *dPAR = deriv->data.F32;
	    dPAR[PAR_RMIN]    = r*cs_alpha;
	    dPAR[PAR_EPSILON] = par[PAR_RMIN]*cs_alpha*drdE;
	    dPAR[PAR_PHI]     = 4.0*par[PAR_RMIN]*cs_alpha*drdP;
	}
	return (value);
    }  

    // value is Y
    if (coord->data.F32[1] > 0.5) {
	float value = par[PAR_RMIN]*sn_alpha*r;

	if (deriv) {
	    psF32 *dPAR = deriv->data.F32;
	    dPAR[PAR_RMIN]    = r*sn_alpha;
	    dPAR[PAR_EPSILON] = par[PAR_RMIN]*sn_alpha*drdE;
	    dPAR[PAR_PHI]     = 4.0*par[PAR_RMIN]*sn_alpha*drdP;
	}
	return (value);
    }  

    psAbort ("programming error: invalid coordinate");
}

# include "psphotInternal.h"

psF64 psPolynomial1DSolve(const psPolynomial1D* poly, psF64 y, bool upper);
void psphotSersicModelNorm (pmPCMdata *pcm, const pmSource *source);

static psArray *classArray = NULL;
static float PSFratioMin =  1.0;
static float PSFratioMax =  4.0;
static float ASPratioMin =  0.2;
static float ASPratioMax =  1.0;

// structure to define the sersic model guess information
typedef struct {
    float PSFratioMin;
    float PSFratioMax;
    float ASPratioMin;
    float ASPratioMax;

    float indexMin;
    float indexMax;
    float kronMin;
    float kronMax;

    bool KronUpper;
    psPolynomial1D *KronToIndex;
    psPolynomial1D *IndexToTotalMag;
    psPolynomial1D *IndexToMajor;
    psPolynomial1D *IndexToMinor;
} psphotSersicModelClass;

static void psphotSersicModelClassFree (psphotSersicModelClass *class) {
    psFree(class->KronToIndex);
    psFree(class->IndexToTotalMag);
    psFree(class->IndexToMajor);
    psFree(class->IndexToMinor);
    return;
}

psphotSersicModelClass *psphotSersicModelClassAlloc() {

    psphotSersicModelClass *class = (psphotSersicModelClass *) psAlloc(sizeof(psphotSersicModelClass));
    psMemSetDeallocator(class, (psFreeFunc) psphotSersicModelClassFree);
    
    class->PSFratioMin = NAN;
    class->PSFratioMax = NAN;
    class->ASPratioMin = NAN;
    class->ASPratioMax = NAN;
    
    class->KronUpper = false;
    class->KronToIndex = NULL;
    class->IndexToTotalMag = NULL;
    class->IndexToMajor = NULL;
    class->IndexToMinor = NULL;

    return class;
}

void psphotSersicModelClassSetIndexRange(psphotSersicModelClass *class, float indexMin, float indexMax) {

    class->indexMin = indexMin;
    class->indexMax = indexMax;
    float modelMinX = -0.5 * class->KronToIndex->coeff[1] / class->KronToIndex->coeff[2];
    float modelMinY = psPolynomial1DEval (class->KronToIndex, modelMinX);
    float kronIndexMin = psPolynomial1DEval (class->KronToIndex, class->indexMin);
    float kronIndexMax = psPolynomial1DEval (class->KronToIndex, class->indexMax);

    // if modelMinX is between indexMin and indexMax, use the modelMinY as the min/max kron value
    // else, use the kron values at the min and max positions

    if ((modelMinX > class->indexMin) && (modelMinX < class->indexMax)) {
	if (class->KronToIndex->coeff[2] < 0.0) {
	    class->kronMax = modelMinY - 0.001; // pad slightly to avoid falling off the curve
	    class->kronMin = PS_MIN(kronIndexMin, kronIndexMax);
	} else {
	    class->kronMin = modelMinY + 0.001; // pad slightly to avoid falling off the curve
	    class->kronMax = PS_MAX(kronIndexMin, kronIndexMax);
	}
    } else {
	class->kronMin = PS_MIN(kronIndexMin, kronIndexMax);
	class->kronMax = PS_MAX(kronIndexMin, kronIndexMax);
    }

    // KronUpper specifies which of the 2 quadratic solutions to accept:
    if (modelMinX < class->indexMin) {
	if (class->KronToIndex->coeff[2] < 0.0)	{
	    class->KronUpper = false;
	} else {
	    class->KronUpper = true;
	}	
	return;
    }
    if (modelMinX > class->indexMax) {
	if (class->KronToIndex->coeff[2] < 0.0)	{
	    class->KronUpper = true;
	} else {
	    class->KronUpper = false;
	}	
	return;
    }

    if (fabs(modelMinX - class->indexMin) < fabs(modelMinX - class->indexMax)) {
	if (class->KronToIndex->coeff[2] < 0.0)	{
	    class->KronUpper = false;
	} else {
	    class->KronUpper = true;
	}	
	return;
    }
    if (class->KronToIndex->coeff[2] < 0.0)	{
	class->KronUpper = true;
    } else {
	class->KronUpper = false;
    }	
    return;
}

/* the Sersic model class guess system uses a set of empirically-determined fits to several
 * relationships: index vs psf-kron mags, total mag vs index, major axis vs index, minor axis
 * vs index.  all are functions of the aspect ratio and of the ratio between the measure major
 * axis and the size of the psf.  Below we have the fits for these different ranges.  

 * aspect ratio values: 1.0, 0.5, 0.33, 0.2
 * psf ratio values: 0.5, 1.0, 2.0, 4.0

 * psf ratio ranges: 0.5-0.8,  0.8-1.5, 1.5-3.0, 3.0-4.0
 * apt ratio ranges: 1.0-0.71, 0.71-0.41, 0.41-0.25, 0.25-0.20

 */

void psphotSersicModelClassInit () {

    psphotSersicModelClass *class;

    if (classArray) return;

    // hardwired trends for now (move into the recipe?)
    classArray = psArrayAllocEmpty (4);

    // image.00.00.fit.dat: PSFratio : 0.5, ASPratio : 1.0
    class = psphotSersicModelClassAlloc();

    class->PSFratioMin =  0.5;
    class->PSFratioMax =  0.8;
    class->ASPratioMin = 0.71;
    class->ASPratioMax = 1.00;
    class->KronToIndex = psPolynomial1DAlloc(PS_POLYNOMIAL_ORD, 2);
    class->IndexToTotalMag = psPolynomial1DAlloc(PS_POLYNOMIAL_ORD, 1);
    class->IndexToMajor = psPolynomial1DAlloc(PS_POLYNOMIAL_ORD, 1);
    class->IndexToMinor = psPolynomial1DAlloc(PS_POLYNOMIAL_ORD, 1);

# include "psphotSersicGuess.00.00.h"    
    psphotSersicModelClassSetIndexRange(class, 1.0, 4.0);

    psArrayAdd(classArray, 4, class);
    psFree (class);

    // image.00.01.fit.dat: PSFratio : 1.0, ASPratio : 1.0
    class = psphotSersicModelClassAlloc();

    class->PSFratioMin =  0.8;
    class->PSFratioMax =  1.5;
    class->ASPratioMin = 0.71;
    class->ASPratioMax = 1.00;
    class->KronToIndex = psPolynomial1DAlloc(PS_POLYNOMIAL_ORD, 2);
    class->IndexToTotalMag = psPolynomial1DAlloc(PS_POLYNOMIAL_ORD, 1);
    class->IndexToMajor = psPolynomial1DAlloc(PS_POLYNOMIAL_ORD, 1);
    class->IndexToMinor = psPolynomial1DAlloc(PS_POLYNOMIAL_ORD, 1);

# include "psphotSersicGuess.00.01.h"    
    psphotSersicModelClassSetIndexRange(class, 1.0, 4.0);

    psArrayAdd(classArray, 4, class);
    psFree (class);

    // image.00.02.fit.dat: PSFratio : 2.0, ASPratio : 1.0
    class = psphotSersicModelClassAlloc();

    class->PSFratioMin =  1.5;
    class->PSFratioMax =  3.0;
    class->ASPratioMin = 0.71;
    class->ASPratioMax = 1.00;
    class->KronToIndex = psPolynomial1DAlloc(PS_POLYNOMIAL_ORD, 2);
    class->IndexToTotalMag = psPolynomial1DAlloc(PS_POLYNOMIAL_ORD, 1);
    class->IndexToMajor = psPolynomial1DAlloc(PS_POLYNOMIAL_ORD, 1);
    class->IndexToMinor = psPolynomial1DAlloc(PS_POLYNOMIAL_ORD, 1);

# include "psphotSersicGuess.00.02.h"    
    psphotSersicModelClassSetIndexRange(class, 1.0, 4.0);

    psArrayAdd(classArray, 4, class);
    psFree (class);
    
    // image.00.03.fit.dat: PSFratio : 4.0, ASPratio : 1.0
    class = psphotSersicModelClassAlloc();

    class->PSFratioMin =  3.0;
    class->PSFratioMax =  4.0;
    class->ASPratioMin = 0.71;
    class->ASPratioMax = 1.00;
    class->KronToIndex = psPolynomial1DAlloc(PS_POLYNOMIAL_ORD, 2);
    class->IndexToTotalMag = psPolynomial1DAlloc(PS_POLYNOMIAL_ORD, 1);
    class->IndexToMajor = psPolynomial1DAlloc(PS_POLYNOMIAL_ORD, 1);
    class->IndexToMinor = psPolynomial1DAlloc(PS_POLYNOMIAL_ORD, 1);

# include "psphotSersicGuess.00.03.h"    
    psphotSersicModelClassSetIndexRange(class, 1.0, 4.0);
    
    psArrayAdd(classArray, 4, class);
    psFree (class);

    // image.01.00.fit.dat: PSFratio : 0.5, ASPratio : 0.5
    class = psphotSersicModelClassAlloc();

    class->PSFratioMin =  0.5;
    class->PSFratioMax =  0.8;
    class->ASPratioMin = 0.41;
    class->ASPratioMax = 0.71;
    class->KronToIndex = psPolynomial1DAlloc(PS_POLYNOMIAL_ORD, 2);
    class->IndexToTotalMag = psPolynomial1DAlloc(PS_POLYNOMIAL_ORD, 1);
    class->IndexToMajor = psPolynomial1DAlloc(PS_POLYNOMIAL_ORD, 1);
    class->IndexToMinor = psPolynomial1DAlloc(PS_POLYNOMIAL_ORD, 1);

# include "psphotSersicGuess.01.00.h"    
    psphotSersicModelClassSetIndexRange(class, 1.0, 4.0);
    
    psArrayAdd(classArray, 4, class);
    psFree (class);

    // image.01.01.fit.dat: PSFratio : 1.0, ASPratio : 0.5
    class = psphotSersicModelClassAlloc();

    class->PSFratioMin =  0.8;
    class->PSFratioMax =  1.5;
    class->ASPratioMin = 0.41;
    class->ASPratioMax = 0.71;
    class->KronToIndex = psPolynomial1DAlloc(PS_POLYNOMIAL_ORD, 2);
    class->IndexToTotalMag = psPolynomial1DAlloc(PS_POLYNOMIAL_ORD, 1);
    class->IndexToMajor = psPolynomial1DAlloc(PS_POLYNOMIAL_ORD, 1);
    class->IndexToMinor = psPolynomial1DAlloc(PS_POLYNOMIAL_ORD, 1);

# include "psphotSersicGuess.01.01.h"    
    psphotSersicModelClassSetIndexRange(class, 1.0, 4.0);

    psArrayAdd(classArray, 4, class);
    psFree (class);
    
    // image.01.02.fit.dat: PSFratio : 2.0, ASPratio : 0.5
    class = psphotSersicModelClassAlloc();

    class->PSFratioMin =  1.5;
    class->PSFratioMax =  3.0;
    class->ASPratioMin = 0.41;
    class->ASPratioMax = 0.71;
    class->KronToIndex = psPolynomial1DAlloc(PS_POLYNOMIAL_ORD, 2);
    class->IndexToTotalMag = psPolynomial1DAlloc(PS_POLYNOMIAL_ORD, 1);
    class->IndexToMajor = psPolynomial1DAlloc(PS_POLYNOMIAL_ORD, 1);
    class->IndexToMinor = psPolynomial1DAlloc(PS_POLYNOMIAL_ORD, 1);

# include "psphotSersicGuess.01.02.h"    
    psphotSersicModelClassSetIndexRange(class, 1.0, 4.0);
    
    psArrayAdd(classArray, 4, class);
    psFree (class);

    // image.01.03.fit.dat: PSFratio : 4.0, ASPratio : 0.5
    class = psphotSersicModelClassAlloc();

    class->PSFratioMin =  3.0;
    class->PSFratioMax =  4.0;
    class->ASPratioMin = 0.41;
    class->ASPratioMax = 0.71;
    class->KronToIndex = psPolynomial1DAlloc(PS_POLYNOMIAL_ORD, 2);
    class->IndexToTotalMag = psPolynomial1DAlloc(PS_POLYNOMIAL_ORD, 1);
    class->IndexToMajor = psPolynomial1DAlloc(PS_POLYNOMIAL_ORD, 1);
    class->IndexToMinor = psPolynomial1DAlloc(PS_POLYNOMIAL_ORD, 1);

# include "psphotSersicGuess.01.03.h"    
    psphotSersicModelClassSetIndexRange(class, 1.0, 4.0);
    
    psArrayAdd(classArray, 4, class);
    psFree (class);

    // image.02.00.fit.dat: PSFratio : 0.5, ASPratio : 0.33
    class = psphotSersicModelClassAlloc();

    class->PSFratioMin =  0.5;
    class->PSFratioMax =  0.8;
    class->ASPratioMin = 0.25;
    class->ASPratioMax = 0.41;
    class->KronToIndex = psPolynomial1DAlloc(PS_POLYNOMIAL_ORD, 2);
    class->IndexToTotalMag = psPolynomial1DAlloc(PS_POLYNOMIAL_ORD, 1);
    class->IndexToMajor = psPolynomial1DAlloc(PS_POLYNOMIAL_ORD, 1);
    class->IndexToMinor = psPolynomial1DAlloc(PS_POLYNOMIAL_ORD, 1);

# include "psphotSersicGuess.02.00.h"    
    psphotSersicModelClassSetIndexRange(class, 1.0, 4.0);
    
    psArrayAdd(classArray, 4, class);
    psFree (class);

    // image.02.01.fit.dat: PSFratio : 1.0, ASPratio : 0.33
    class = psphotSersicModelClassAlloc();

    class->PSFratioMin =  0.8;
    class->PSFratioMax =  1.5;
    class->ASPratioMin = 0.25;
    class->ASPratioMax = 0.41;
    class->KronToIndex = psPolynomial1DAlloc(PS_POLYNOMIAL_ORD, 2);
    class->IndexToTotalMag = psPolynomial1DAlloc(PS_POLYNOMIAL_ORD, 1);
    class->IndexToMajor = psPolynomial1DAlloc(PS_POLYNOMIAL_ORD, 1);
    class->IndexToMinor = psPolynomial1DAlloc(PS_POLYNOMIAL_ORD, 1);

# include "psphotSersicGuess.02.01.h"    
    psphotSersicModelClassSetIndexRange(class, 1.0, 4.0);
    
    psArrayAdd(classArray, 4, class);
    psFree (class);

    // image.02.02.fit.dat: PSFratio : 2.0, ASPratio : 0.33
    class = psphotSersicModelClassAlloc();

    class->PSFratioMin =  1.5;
    class->PSFratioMax =  3.0;
    class->ASPratioMin = 0.25;
    class->ASPratioMax = 0.41;
    class->KronToIndex = psPolynomial1DAlloc(PS_POLYNOMIAL_ORD, 2);
    class->IndexToTotalMag = psPolynomial1DAlloc(PS_POLYNOMIAL_ORD, 1);
    class->IndexToMajor = psPolynomial1DAlloc(PS_POLYNOMIAL_ORD, 1);
    class->IndexToMinor = psPolynomial1DAlloc(PS_POLYNOMIAL_ORD, 1);

# include "psphotSersicGuess.02.02.h"    
    psphotSersicModelClassSetIndexRange(class, 1.0, 4.0);
    
    psArrayAdd(classArray, 4, class);
    psFree (class);

    // image.02.03.fit.dat: PSFratio : 4.0, ASPratio : 0.33
    class = psphotSersicModelClassAlloc();

    class->PSFratioMin =  3.0;
    class->PSFratioMax =  4.0;
    class->ASPratioMin = 0.25;
    class->ASPratioMax = 0.41;
    class->KronToIndex = psPolynomial1DAlloc(PS_POLYNOMIAL_ORD, 2);
    class->IndexToTotalMag = psPolynomial1DAlloc(PS_POLYNOMIAL_ORD, 1);
    class->IndexToMajor = psPolynomial1DAlloc(PS_POLYNOMIAL_ORD, 1);
    class->IndexToMinor = psPolynomial1DAlloc(PS_POLYNOMIAL_ORD, 1);

# include "psphotSersicGuess.02.03.h"    
    psphotSersicModelClassSetIndexRange(class, 1.0, 4.0);
    
    psArrayAdd(classArray, 4, class);
    psFree (class);

    // image.03.00.fit.dat: PSFratio : 0.5, ASPratio : 0.2
    class = psphotSersicModelClassAlloc();

    class->PSFratioMin =  0.5;
    class->PSFratioMax =  0.8;
    class->ASPratioMin = 0.20;
    class->ASPratioMax = 0.25;
    class->KronToIndex = psPolynomial1DAlloc(PS_POLYNOMIAL_ORD, 2);
    class->IndexToTotalMag = psPolynomial1DAlloc(PS_POLYNOMIAL_ORD, 1);
    class->IndexToMajor = psPolynomial1DAlloc(PS_POLYNOMIAL_ORD, 1);
    class->IndexToMinor = psPolynomial1DAlloc(PS_POLYNOMIAL_ORD, 1);

# include "psphotSersicGuess.03.00.h"    
    psphotSersicModelClassSetIndexRange(class, 1.0, 4.0);
    
    psArrayAdd(classArray, 4, class);
    psFree (class);

    // image.03.01.fit.dat: PSFratio : 1.0, ASPratio : 0.2
    class = psphotSersicModelClassAlloc();

    class->PSFratioMin =  0.8;
    class->PSFratioMax =  1.5;
    class->ASPratioMin = 0.20;
    class->ASPratioMax = 0.25;
    class->KronToIndex = psPolynomial1DAlloc(PS_POLYNOMIAL_ORD, 2);
    class->IndexToTotalMag = psPolynomial1DAlloc(PS_POLYNOMIAL_ORD, 1);
    class->IndexToMajor = psPolynomial1DAlloc(PS_POLYNOMIAL_ORD, 1);
    class->IndexToMinor = psPolynomial1DAlloc(PS_POLYNOMIAL_ORD, 1);

# include "psphotSersicGuess.03.01.h"    
    psphotSersicModelClassSetIndexRange(class, 1.0, 4.0);
    
    psArrayAdd(classArray, 4, class);
    psFree (class);

    // image.03.02.fit.dat: PSFratio : 2.0, ASPratio : 0.2
    class = psphotSersicModelClassAlloc();

    class->PSFratioMin =  1.5;
    class->PSFratioMax =  3.0;
    class->ASPratioMin = 0.20;
    class->ASPratioMax = 0.25;
    class->KronToIndex = psPolynomial1DAlloc(PS_POLYNOMIAL_ORD, 2);
    class->IndexToTotalMag = psPolynomial1DAlloc(PS_POLYNOMIAL_ORD, 1);
    class->IndexToMajor = psPolynomial1DAlloc(PS_POLYNOMIAL_ORD, 1);
    class->IndexToMinor = psPolynomial1DAlloc(PS_POLYNOMIAL_ORD, 1);

# include "psphotSersicGuess.03.02.h"    
    psphotSersicModelClassSetIndexRange(class, 1.0, 4.0);
    
    psArrayAdd(classArray, 4, class);
    psFree (class);

    // image.03.03.fit.dat: PSFratio : 4.0, ASPratio : 0.2
    class = psphotSersicModelClassAlloc();

    class->PSFratioMin =  3.0;
    class->PSFratioMax =  4.0;
    class->ASPratioMin = 0.20;
    class->ASPratioMax = 0.25;
    class->KronToIndex = psPolynomial1DAlloc(PS_POLYNOMIAL_ORD, 2);
    class->IndexToTotalMag = psPolynomial1DAlloc(PS_POLYNOMIAL_ORD, 1);
    class->IndexToMajor = psPolynomial1DAlloc(PS_POLYNOMIAL_ORD, 1);
    class->IndexToMinor = psPolynomial1DAlloc(PS_POLYNOMIAL_ORD, 1);

# include "psphotSersicGuess.03.03.h"    
    psphotSersicModelClassSetIndexRange(class, 1.0, 4.0);
    
    psArrayAdd(classArray, 4, class);
    psFree (class);
}
 
void psphotSersicModelClassCleanup () {
    psFree (classArray);
}

# define TIMING false

bool psphotSersicModelClassGuessPCM (pmPCMdata *pcm, pmSource *source) {

    float t1, t2, t4, t5;
    if (TIMING) { psTimerStart ("SersicGuess"); }

    psF32 *PAR = pcm->modelConv->params->data.F32;

    // XXX require: moments, psfMag, moments->kronFlux

    // we attempt to guess the Sersic model parameters based on a number of non-parametric
    // measurements: moments, kron mag, etc

    // * first choose the model class based on (a) ratio of Moments Major axis to PSF major
    // * axis and (b) moments axial ratio

    // convert the moments to Major,Minor,Theta
    psEllipseMoments moments;

    moments.x2 = source->moments->Mxx;
    moments.y2 = source->moments->Myy;
    moments.xy = source->moments->Mxy;
    
    // limit axis ratio < 20.0
    psEllipseAxes momentAxes = psEllipseMomentsToAxes (moments, 20.0);

    // convert the PSF shape to Major,Minor,Theta
    psEllipseShape shape;

    // XXX make sure this is consistent with the re-definition of PM_PAR_SXX
    shape.sx  = source->modelPSF->params->data.F32[PM_PAR_SXX] / M_SQRT2;
    shape.sy  = source->modelPSF->params->data.F32[PM_PAR_SYY] / M_SQRT2;
    shape.sxy = source->modelPSF->params->data.F32[PM_PAR_SXY];
    psEllipseAxes psfAxes = psEllipseShapeToAxes (shape, 20.0);
    
    // get the PSFratio and the ASPratio : we use these to choose the model class
    float PSFratio = source->moments->Mrf / (2.35 * psfAxes.major) ;
    float ASPratio = momentAxes.minor / momentAxes.major;

    // saturate PSFratio and ASPratio to min / max values
    PSFratio = PS_MAX(PSFratio, PSFratioMin);
    PSFratio = PS_MIN(PSFratio, PSFratioMax);
    ASPratio = PS_MAX(ASPratio, ASPratioMin);
    ASPratio = PS_MIN(ASPratio, ASPratioMax);

    // find the containing model class:

    if (TIMING) { t1 = psTimerMark ("SersicGuess"); }

    psphotSersicModelClass *class = NULL;    
    for (int i = 0; i < classArray->n; i++) {
	psphotSersicModelClass *thisClass = classArray->data[i];
	if (PSFratio < thisClass->PSFratioMin) continue;
	if (PSFratio > thisClass->PSFratioMax) continue;
	if (ASPratio < thisClass->ASPratioMin) continue;
	if (ASPratio > thisClass->ASPratioMax) continue;
	class = thisClass;
	break;
    }
	
    if (TIMING) { t2 = psTimerMark ("SersicGuess"); }

    psAssert (class, "PSFratio and ASPratio must be in range");

    // get the index guess from the KronMag - psfMag:

    // get dKronMag & saturate dKronMag at limits of valid range
    float dKronMag = -2.5*log10(source->moments->KronFlux) - source->psfMag;
    dKronMag = PS_MIN(dKronMag, class->kronMax);
    dKronMag = PS_MAX(dKronMag, class->kronMin);

    // get index (saturate at valid ends)
    float index = psPolynomial1DSolve (class->KronToIndex, dKronMag, class->KronUpper);
    index = PS_MIN (index, class->indexMax);
    index = PS_MAX (index, class->indexMin);
    
    // float totalMagOffset = psPolynomial1DEval (class->IndexToTotalMag, index);
    // float totalMag = -2.5*log10(source->moments->KronFlux) + totalMagOffset;

    // need to go from totalMag to Io in the sersic model
    
    float majorAxisFactor = psPolynomial1DEval (class->IndexToMajor, index);
    float minorAxisFactor = psPolynomial1DEval (class->IndexToMinor, index);

    momentAxes.major *= (1.0 + majorAxisFactor);
    momentAxes.minor *= (1.0 + minorAxisFactor);

    psEllipseShape extShapeGuess = psEllipseAxesToShape (momentAxes);
    if (!isfinite(extShapeGuess.sx))  return false;
    if (!isfinite(extShapeGuess.sy))  return false;
    if (!isfinite(extShapeGuess.sxy)) return false;

    // set the actual model parameters:

    // index is standard sersic index (n = 1-4), but PAR7 = 1/2n
    PAR[PM_PAR_7] = 0.5 / index;

    // sky is zero (no longer fitted, but not yet deprecated)
    PAR[PM_PAR_SKY]  = 0.0;

    // set the model position
    if (!pmModelSetPosition(&PAR[PM_PAR_XPOS], &PAR[PM_PAR_YPOS], source)) {
      return false;
    }
    PAR[PM_PAR_SXX] = extShapeGuess.sx * M_SQRT2;
    PAR[PM_PAR_SXY] = extShapeGuess.sxy;
    PAR[PM_PAR_SYY] = extShapeGuess.sy * M_SQRT2;

    // XXX this is a bit of a waste: calculate the flux with Io = 1.0, then renormalize.
    PAR[PM_PAR_I0]  = 1.0;

    if (TIMING) { t4 = psTimerMark ("SersicGuess"); }

    // set the normalization by linear fit between model and data
    psphotSersicModelNorm (pcm, source);
    if (!isfinite(PAR[PM_PAR_I0])) {
        fprintf(stderr, "psphotSersicModelClassGuessPCM: psphotSerisicModelNorm set PM_PAR_I0 to NAN\n");
        return false;
    }

    if (TIMING) { t5 = psTimerMark ("SersicGuess"); }

    if (TIMING) {
    	fprintf (stderr, "guess, t1: %6.4f, t2: %6.4f, t4: %6.4f, t5: %6.4f\n", t1, t2, t4, t5);
    }

    // float flux = pcm->modelConv->modelFlux(pcm->modelConv->params);
    // float normMag = -2.5*log10(flux);
    // float Io = pow(10.0, -0.4*(totalMag - normMag));
    // PAR[PM_PAR_I0] = Io;

    return true;
}

void psphotSersicModelNorm (pmPCMdata *pcm, const pmSource *source) {

    psVector *params = pcm->modelConv->params;
    // XXX : not needed? psAssert (params->data.F32[PM_PAR_I0] == 1.0, "not normalized?");

    // generate the convolved model image
    // working vector to store local coordinate
    psVector *coord = psVectorAlloc(2, PS_TYPE_F32);

    // create the convolved model in situ
    psAssert (pcm->modelConvFlux, "not already allocated?");
    psImageInit (pcm->modelConvFlux, 0.0);

    // fill in the coordinate and value entries
    for (psS32 i = 0; i < source->pixels->numRows; i++) {
        for (psS32 j = 0; j < source->pixels->numCols; j++) {

            // Convert i/j to image space:
            coord->data.F32[0] = (psF32) (j + source->pixels->col0);
            coord->data.F32[1] = (psF32) (i + source->pixels->row0);

            pcm->modelConvFlux->data.F32[i][j] = pcm->modelConv->class->modelFunc (NULL, params, coord);
        }
    }
    psFree(coord);
    
    psImageSmooth (pcm->modelConvFlux, pcm->sigma, pcm->nsigma);

    float YYmod = 0.0;
    float Ymod2 = 0.0;
    bool usePoisson = false;

    for (psS32 i = 0; i < source->pixels->numRows; i++) {
        for (psS32 j = 0; j < source->pixels->numCols; j++) {
            // XXX are we doing the right thing with the mask?
            // skip masked points
            if (source->maskObj->data.PS_TYPE_IMAGE_MASK_DATA[i][j]) {
                continue;
            }
            // skip zero-variance points
            if (source->variance->data.F32[i][j] == 0) {
                continue;
            }
            // skip nan value points
            if (!isfinite(source->pixels->data.F32[i][j])) {
                continue;
            }

            float ymodel  = pcm->modelConvFlux->data.F32[i][j];
            float yweight = (usePoisson) ? 1.0 / source->variance->data.F32[i][j] : 1.0;

	    YYmod += ymodel * source->pixels->data.F32[i][j] * yweight;
	    Ymod2 += PS_SQR(ymodel) * yweight;
        }
    }

    float Io = YYmod / Ymod2;

    params->data.F32[PM_PAR_I0] *= Io;

    return;
}

// Given a polynomial y = C0 + C1 x + C2 x^2 etc, solve for x given y
// actually: this only solves 1st and 2nd order polynomials
// for 2nd order, it always returns the (positive/negative) term

psF64 psPolynomial1DSolve(
    const psPolynomial1D* poly,
    psF64 y,
    bool upper)
{
    psF64 x, C;
    switch (poly->nX) {
      case 1:
	// y = coeff[0] + coeff[1]*x 
	// x = (y - coeff[0]) / coeff[1]
	x = (y - poly->coeff[0]) / poly->coeff[1];
	return x;
	
      case 2:
	// y = coeff[0] + coeff[1]*x + coeff[2]*x^2
	// x = -coeff[1] +/- sqrt(coeff[1]^2 - 4*coeff[0]*coeff[2]) / (2 coeff[0])
	C = poly->coeff[0] - y;
	if (upper) {
	    x = (-poly->coeff[1] + sqrt(PS_SQR(poly->coeff[1]) - 4*poly->coeff[2]*C)) / (2.0 * poly->coeff[2]);
	} else {
	    x = (-poly->coeff[1] - sqrt(PS_SQR(poly->coeff[1]) - 4*poly->coeff[2]*C)) / (2.0 * poly->coeff[2]);
	}
	return x;

      default:
	psAbort("invalid polynomial for 1D solver");
    }
    psAbort("invalid polynomial for 1D solver");
}

/** Diagnostic plots for pmSubtraction
 * @author Chris Beaumont, IfA
 */

/* Include Files   */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <strings.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <pslib.h>

#include "pmKapaPlots.h"
#include "pmFPA.h"
#include "pmSubtractionTypes.h"
#include "pmSubtraction.h"
#include "pmSubtractionStamps.h"
#include "pmSubtractionEquation.h"
#include "pmSubtractionKernels.h"
#include "pmSubtractionVisual.h"

#include "pmVisual.h"
#include "pmVisualUtils.h"

#include "pmHDU.h"
#include "pmFPA.h"
#include "pmAstrometryObjects.h"

# if (HAVE_KAPA)
# include <kapa.h>

//variables to determine when things are plotted
static bool plotConvKernels      = true;
static bool plotStamps           = true;
static bool plotLeastSquares     = true;
static bool plotImage            = true;

// variables to store plotting window indices
static int kapa1 = -1;
static int kapa2 = -1;
static int kapa3 = -1;
static int kapa4 = -1;

/** function prototypes*/
static bool plotStampLocations(pmSubtractionStampList *stamps, pmReadout *ro);

// Initialization Routines



/** destroy windows at the end of a run*/
bool pmSubtractionVisualClose(void)
{
    if(kapa1 != -1) KapaClose(kapa1);
    if(kapa2 != -1) KapaClose(kapa2);
    return true;
}

// Plotting Routines

/** Display images of the convolution kernels
 *  @param convKernels the kernels to plot
 *    @return true for success */
bool pmSubtractionVisualPlotConvKernels(pmSubtractionKernels *kernels) {

    if (!pmVisualTestLevel("ppsub.kernels", 1)) return true;

    if (!plotConvKernels) return true;

    if (!pmVisualInitWindow(&kapa4, "kernels")) {
        return false;
    }

    psImage *convKernels = pmSubtractionKernelsImageMosaic(kernels);
    pmVisualScaleImage(kapa4, convKernels, "Convolution_Kernels", 0, true);
    pmVisualAskUser(&plotConvKernels);
    psFree(convKernels);
    return true;
}


/** Display the postage stamps used to determine the convolution kernels
    @param stamps to display
    @return true for success */
bool pmSubtractionVisualPlotStamps(pmSubtractionStampList *stamps, pmReadout *ro) {

    if (!pmVisualTestLevel("ppsub.stamps", 1)) return true;

    if (!plotStamps) return true;

    if (!pmVisualInitWindow (&kapa1, "ppSub:Images")) {
        return false;
    }
    if (!pmVisualInitWindow (&kapa2, "ppSub:StampMasterImage")) {
        return false;
    }

    //Plot Location of stamps
    plotStampLocations(stamps, ro);

    //Find the stamp size
    int imageMax = -1;
    int numStamps = 0;
    psElemType type = PS_TYPE_F32; // will be overwritten
    for(int i = 0; i < stamps->num; i++) {
        pmSubtractionStamp *stamp = stamps->stamps->data[i];
        if (stamp == NULL) continue;

        psKernel *k1 = stamp->image1;
        if (k1 == NULL) continue;

        psImage *i1 = k1->image;
        if (i1 == NULL) continue;

        imageMax = PS_MAX(imageMax, i1->numCols);
        imageMax = PS_MAX(imageMax, i1->numRows);
        numStamps++;
        type = i1->type.type;
    }
    if (imageMax == -1) return false;

    int border = 30;
    int tileRowCount = (int) ceil(sqrt(numStamps));
    int canvasX = tileRowCount * (imageMax * 2 + border);
    int canvasY = tileRowCount * (imageMax + border);
    psImage *canvas = psImageAlloc (canvasX, canvasY, type);
    psImageInit (canvas, NAN);

    //overlay the images
    int stampNum = 0;
    int stampListNum = 0;
    while (stampNum < numStamps) {
        int x0 = (2 * imageMax + border) * (stampNum % tileRowCount);
        int y0 = (imageMax + border) * (stampNum / tileRowCount);

        pmSubtractionStamp *stamp = stamps->stamps->data[stampListNum++];
        if (stamp == NULL) continue;

        psKernel *k1 = stamp->image1;
        psKernel *k2 = stamp->image2;

        if (k1 == NULL) continue;
        psImage *i1 = k1->image;
        psImage *i2 = k2->image;

        if (i1 == NULL) continue;
        psImageOverlaySection(canvas, i1, x0 + 0 * imageMax, y0, "=");
        psImageOverlaySection(canvas, i2, x0 + 1 * imageMax + border / 4, y0, "=");
        stampNum++;
    }
    pmVisualScaleImage(kapa1, canvas, "Subtraction_Stamps", 0, true);
    psFree(canvas);

    pmVisualAskUser(&plotStamps);
    return true;
}

/** Plot the least-squares matrix of each stamp */
bool pmSubtractionVisualPlotLeastSquares (pmSubtractionStampList *stamps) {

    if (!pmVisualTestLevel("ppsub.chisq", 1)) return true;

    if (!plotLeastSquares) return true;

    if (!pmVisualInitWindow (&kapa1, "PPSub:Images")) {
        return false;
    }

    //Find the stamp size
    int imageMax = -1;
    int numStamps = 0;
    psElemType type = PS_TYPE_F64;

    for(int i = 0; i < stamps->num; i++) {
        pmSubtractionStamp *stamp = stamps->stamps->data[i];
        if (stamp == NULL) continue;

        psImage *im = stamp->matrix;
        if (im == NULL) continue;

        imageMax = PS_MAX(imageMax, im->numCols);
        imageMax = PS_MAX(imageMax, im->numRows);
        numStamps++;
        type = im->type.type;
    }
    if (imageMax == -1) return false;

    int border = 15;
    imageMax += border;
    int tileRowCount = (int) ceil(sqrt(numStamps));
    int canvasX = tileRowCount * (imageMax);
    int canvasY = tileRowCount * (imageMax);
    psImage *canvas = psImageAlloc (canvasX, canvasY, type);
    psImageInit (canvas, NAN);

    //overlay the images
    int stampNum = 0;
    int stampListNum = 0;
    while (stampNum < numStamps) {
        int x0 = (imageMax) * (stampNum % tileRowCount);
        int y0 = (imageMax) * (stampNum / tileRowCount);

        pmSubtractionStamp *stamp = stamps->stamps->data[stampListNum++];
        if (stamp == NULL) continue;

        psImage *im = stamp->matrix;
        if (im == NULL) continue;

        psImageOverlaySection(canvas, im, x0, y0, "=");
        stampNum++;

	// renormalize the section
	float maxValue = 0;
	for (int iy = 0; iy < im->numRows; iy++) {
	    for (int ix = 0; ix < im->numCols; ix++) {
		maxValue = PS_MAX(maxValue, im->data.F64[iy][ix]);
	    }
	}
	if (maxValue == 0.0) continue;
	for (int iy = 0; iy < im->numRows; iy++) {
	    for (int ix = 0; ix < im->numCols; ix++) {
		canvas->data.F64[y0 + iy][x0 + ix] /= maxValue;
	    }
	}
    }

    psImage *canvas32 = pmVisualImageToFloat(canvas);
    pmVisualScaleImage(kapa1, canvas32, "Least_Squares", 0, true);

    if (0) {
	static int count = 0;
	char filename[64];
	sprintf (filename, "chisq.%02d.fits", count);
	count ++;
	psFits *fits = psFitsOpen (filename, "w");
	psFitsWriteImage (fits, NULL, canvas32, 0, NULL);
	psFitsClose (fits);
    }

    pmVisualAskUser(&plotLeastSquares);
    psFree(canvas);
    psFree(canvas32);
    return true;
}

/** Plot the least-squares matrix of each stamp */
bool pmSubtractionVisualPlotLeastSquaresResid (const pmSubtractionStampList *stamps, psImage *matrixIn, int nUsed) {

    if (!pmVisualTestLevel("ppsub.chisq", 1)) return true;

    if (!plotLeastSquares) return true;

    if (!pmVisualInitWindow (&kapa1, "PPSub:Images")) {
        return false;
    }

    psImage *matrixNorm = psImageCopy(NULL, matrixIn, PS_TYPE_F64);
    {
	// renormalize the matrix
	float maxValue = 0;
	for (int iy = 0; iy < matrixNorm->numRows; iy++) {
	    for (int ix = 0; ix < matrixNorm->numCols; ix++) {
		maxValue = PS_MAX(maxValue, matrixNorm->data.F64[iy][ix]);
	    }
	}
	for (int iy = 0; iy < matrixNorm->numRows; iy++) {
	    for (int ix = 0; ix < matrixNorm->numCols; ix++) {
		matrixNorm->data.F64[iy][ix] /= maxValue;
	    }
	}
    }

    // Find the stamp size
    int imageMax = -1;
    int numStamps = 0;
    psElemType type = PS_TYPE_F64;

    for(int i = 0; i < stamps->num; i++) {
        pmSubtractionStamp *stamp = stamps->stamps->data[i];
        if (stamp == NULL) continue;

        psImage *im = stamp->matrix;
        if (im == NULL) continue;

        imageMax = PS_MAX(imageMax, im->numCols);
        imageMax = PS_MAX(imageMax, im->numRows);
        numStamps++;
        type = im->type.type;
    }
    if (imageMax == -1) {
	psFree (matrixNorm);
	return false;
    }

    int border = 15;
    imageMax += border;
    int tileRowCount = (int) ceil(sqrt(numStamps));
    int canvasX = tileRowCount * (imageMax);
    int canvasY = tileRowCount * (imageMax);
    psImage *canvas = psImageAlloc (canvasX, canvasY, type);
    psImageInit (canvas, NAN);

    // overlay the images
    int stampNum = 0;
    int stampListNum = 0;
    while (stampNum < numStamps) {
        int x0 = (imageMax) * (stampNum % tileRowCount);
        int y0 = (imageMax) * (stampNum / tileRowCount);

        pmSubtractionStamp *stamp = stamps->stamps->data[stampListNum++];
        if (stamp == NULL) continue;

        psImage *im = stamp->matrix;
        if (im == NULL) continue;

        stampNum++;

	if (stamp->status != PM_SUBTRACTION_STAMP_USED) continue;

	// renormalize the section
	float maxValue = 0;
	for (int iy = 0; iy < im->numRows; iy++) {
	    for (int ix = 0; ix < im->numCols; ix++) {
		maxValue = PS_MAX(maxValue, im->data.F64[iy][ix]);
	    }
	}
	if (maxValue == 0.0) continue;
	for (int iy = 0; iy < im->numRows; iy++) {
	    for (int ix = 0; ix < im->numCols; ix++) {
		canvas->data.F64[y0 + iy][x0 + ix] = (im->data.F64[iy][ix] / maxValue - matrixNorm->data.F64[iy][ix]) / matrixNorm->data.F64[iy][ix];
	    }
	}
    }

    psImage *canvas32 = pmVisualImageToFloat(canvas);
    pmVisualRangeImage(kapa2, canvas32, "Least_Squares", 0, -100.0, 100.0);

    if (0) {
	static int count = 0;
	char filename[64];
	sprintf (filename, "chisq.%02d.fits", count);
	count ++;
	psFits *fits = psFitsOpen (filename, "w");
	psFitsWriteImage (fits, NULL, canvas32, 0, NULL);
	psFitsClose (fits);
    }

    pmVisualAskUser(&plotLeastSquares);
    psFree(canvas);
    psFree(canvas32);
    psFree (matrixNorm);
    return true;
}

bool pmSubtractionVisualShowSubtraction(psImage *image, psImage *ref, psImage *sub) {

    if (!pmVisualTestLevel("ppsub.images.sub", 1)) return true;

    if (!plotImage) return true;

    if (!pmVisualInitWindow (&kapa1, "PPSub:Images")) {
        return false;
    }

    pmVisualScaleImage(kapa1, image, "Image", 0, true);
    pmVisualScaleImage(kapa1, ref, "Reference", 1, true);
    pmVisualScaleImage(kapa1, sub, "Subtraction", 2, true);
    pmVisualAskUser(&plotImage);
    return true;
}

bool pmSubtractionVisualShowKernels(pmSubtractionKernels *kernels) {

    if (!pmVisualTestLevel("ppsub.kernels.final", 1)) return true;

    if (!pmVisualInitWindow (&kapa1, "PPSub:Images")) {
        return false;
    }

    // get the kernel sizes
    int footprint = kernels->size;

    // output image is a grid of NXsub by NYsub sub-images
    int NXsub = sqrt(kernels->num);
    int NYsub = kernels->num / NXsub;
    if (kernels->num % NXsub) NYsub++;

    int NXpix = NXsub * (2*footprint + 1 + 3);
    int NYpix = NYsub * (2*footprint + 1 + 3);

    psImage *output = psImageAlloc(NXpix, NYpix, PS_TYPE_F32);
    psImageInit (output, 0.0);

    for (int i = 0; i < kernels->num; i++) {
	pmSubtractionKernelPreCalc *preCalc = kernels->preCalc->data[i];
	psKernel *kernel = preCalc->kernel;

	int xSub = i % NXsub;
	int ySub = i / NXsub;

	int xPix = xSub * (2*footprint + 1 + 3) + footprint;
	int yPix = ySub * (2*footprint + 1 + 3) + footprint;

	double sum = 0.0;
	for (int y = -footprint; y <= footprint; y++) {
	    for (int x = -footprint; x <= footprint; x++) {
		output->data.F32[y + yPix][x + xPix] = kernel->kernel[y][x];
		sum += kernel->kernel[y][x];
	    }
	}
	// fprintf (stderr, "kernel %d, sum %f\n", i, sum);
    }							 
	
    pmVisualScaleImage(kapa1, output, "Image", 0, true);
    psFree(output);
    pmVisualAskUser(&plotImage);
    return true;
}

bool pmSubtractionVisualShowBasis(pmSubtractionStampList *stamps) {

    if (!pmVisualTestLevel("ppsub.basis", 1)) return true;

    if (!pmVisualInitWindow (&kapa2, "ppSub:StampMasterImage")) {
        return false;
    }

    // clear the overlay (red at least!)
    KiiEraseOverlay (kapa2, "red");

    // get the kernel sizes
    int footprint = stamps->footprint;

    // choose the brightest stamp
    pmSubtractionStamp *maxStamp = NULL;
    float maxFlux = NAN;
    for (int i = 0; i < stamps->num; i++) {
	pmSubtractionStamp *stamp = stamps->stamps->data[i];
	if (!isfinite(stamp->flux)) continue;
	if (!stamp->convolutions1 && !stamp->convolutions2) continue;
	// fprintf (stderr, "flux: %f, maxFlux: %f  ", stamp->flux, maxFlux);
	if (!maxStamp) {
	    maxFlux = stamp->flux;
	    maxStamp = stamp;
	    // fprintf (stderr, "maxStamp %d\n", i);
	    continue;
	} else {
	    // fprintf (stderr, "\n");
	}
	if (stamp->flux > maxFlux) {
	    maxFlux = stamp->flux;
	    maxStamp = stamp;
	}
    }

    if (!isfinite(maxStamp->flux)) {
	fprintf (stderr, "no valid stamps?\n");
    }

    int nKernels = 0;

    // paste in the kernel images, scaled by sum2
    if (maxStamp->convolutions1) {
	// output image is a grid of NXsub by NYsub sub-images
	nKernels = maxStamp->convolutions1->n;
	int NXsub = sqrt(nKernels);
	int NYsub = nKernels / NXsub;
	if (nKernels % NXsub) NYsub++;

	int NXpix = NXsub * (2*footprint + 1 + 3);
	int NYpix = NYsub * (2*footprint + 1 + 3);

	psImage *output = psImageAlloc(NXpix, NYpix, PS_TYPE_F32);
	psImageInit (output, 0.0);

	for (int i = 0; i < nKernels; i++) {
            psKernel *kernel = maxStamp->convolutions1->data[i];
	    
	    int xSub = i % NXsub;
	    int ySub = i / NXsub;
	    
	    int xPix = xSub * (2*footprint + 1 + 3) + footprint;
	    int yPix = ySub * (2*footprint + 1 + 3) + footprint;
	    
	    double sum2 = 0.0;
	    for (int y = -footprint; y <= footprint; y++) {
		for (int x = -footprint; x <= footprint; x++) {
		    sum2 += PS_SQR(kernel->kernel[y][x]);
		}
	    }
	    float scale = sqrt(sum2) / PS_SQR(2*footprint + 1);
	    for (int y = -footprint; y <= footprint; y++) {
		for (int x = -footprint; x <= footprint; x++) {
		    output->data.F32[y + yPix][x + xPix] = kernel->kernel[y][x] / scale;
		}
	    }
	}		
	pmVisualScaleImage(kapa2, output, "Image", 0, true);

	if (0) {
	    psFits *fits = psFitsOpen("basis.1.fits", "w");
	    psFitsWriteImage(fits, NULL, output, 0, NULL);
	    psFitsClose(fits);
	}
	psFree (output);
    }
	
    if (maxStamp->convolutions2) {
	// output image is a grid of NXsub by NYsub sub-images
	nKernels = maxStamp->convolutions2->n;
	int NXsub = sqrt(nKernels);
	int NYsub = nKernels / NXsub;
	if (nKernels % NXsub) NYsub++;

	int NXpix = NXsub * (2*footprint + 1 + 3);
	int NYpix = NYsub * (2*footprint + 1 + 3);

	psImage *output = psImageAlloc(NXpix, NYpix, PS_TYPE_F32);
	psImageInit (output, 0.0);

	for (int i = 0; i < nKernels; i++) {
            psKernel *kernel = maxStamp->convolutions2->data[i];
	    
	    int xSub = i % NXsub;
	    int ySub = i / NXsub;
	    
	    int xPix = xSub * (2*footprint + 1 + 3) + footprint;
	    int yPix = ySub * (2*footprint + 1 + 3) + footprint;
	    
	    double sum2 = 0.0;
	    for (int y = -footprint; y <= footprint; y++) {
		for (int x = -footprint; x <= footprint; x++) {
		    sum2 += PS_SQR(kernel->kernel[y][x]);
		}
	    }
	    float scale = sqrt(sum2) / PS_SQR(2*footprint + 1);
	    for (int y = -footprint; y <= footprint; y++) {
		for (int x = -footprint; x <= footprint; x++) {
		    output->data.F32[y + yPix][x + xPix] = kernel->kernel[y][x] / scale;
		}
	    }
	}		
	pmVisualScaleImage(kapa2, output, "Image", 1, true);

	if (0) {
	    psFits *fits = psFitsOpen("basis.2.fits", "w");
	    psFitsWriteImage(fits, NULL, output, 0, NULL);
	    psFitsClose(fits);
	}
	psFree(output);
    }					 
	
    pmVisualAskUser(&plotImage);
    return true;
}

static bool plotStampLocations(pmSubtractionStampList *stamps, pmReadout *ro) {

    if (!pmVisualScaleImage(kapa2, ro->image, "Stamp_master_image", 0, true)) {
        fprintf(stderr, "Cannot display postage stamp master image. Skipping \n");
        return false;
    }

    int Noverlay;
    KiiOverlay *overlay;

    // note: this uses the Ohana allocation tools:
    // ALLOCATE (overlay, KiiOverlay, 3*peaks->n + 1);
    ALLOCATE (overlay, KiiOverlay, stamps->num);

    Noverlay = 0;
    char boxID[PS_SMALLWORD];
    for (int i = 0; i < stamps->num; i++) {

        pmSubtractionStamp *stamp = stamps->stamps->data[i];
        if (stamp == NULL) continue;

        overlay[Noverlay].type = KII_OVERLAY_BOX;
	if ((stamp->x < 1.0) && (stamp->y < 1.0)) {
	    // fprintf (stderr, "stamp zero: %f %f\n", stamp->x, stamp->y);
	    continue;
	}
	if (!isfinite(stamp->x) && !isfinite(stamp->y)) {
	    // fprintf (stderr, "stamp nan: %f %f\n", stamp->x, stamp->y);
	    continue;
	}
        overlay[Noverlay].x = stamp->x;
        overlay[Noverlay].y = stamp->y;
        overlay[Noverlay].dx = 40.0;
        overlay[Noverlay].dy = 40.0;
        overlay[Noverlay].angle = 0.0;
        ps_snprintf_nowarn(boxID, PS_SMALLWORD, "%d", i);
        overlay[Noverlay].text = boxID;
        Noverlay ++;
    }

    KiiLoadOverlay (kapa2, overlay, Noverlay, "red");
    FREE (overlay);
    return true;
}

bool pmVisualShowImage(int kapaFD, psImage *inImage, const char *name, int channel, float min, float max) {

    KiiImage image;
    KapaImageData data;
    Coords coords;
    strcpy (coords.ctype, "RA---TAN");
    
    image.data2d = inImage->data.F32;
    image.Nx = inImage->numCols;
    image.Ny = inImage->numRows;
    strcpy (data.name, name);
    strcpy (data.file, name);
    
    data.zero  = min;
    data.range = max;
    data.logflux = 0;
    
    KiiSetChannel (kapaFD, channel);
    KiiNewPicture2D (kapaFD, &image, &data, &coords);
    return true;
}

static int footprint = 0;
static int NX = 0;
static int NY = 0;
static psImage *sourceImage      = NULL;
static psImage *targetImage      = NULL;
static psImage *residualImage    = NULL;
static psImage *fresidualImage   = NULL;
static psImage *differenceImage  = NULL;
static psImage *convolutionImage = NULL;

bool pmSubtractionVisualShowFit(pmSubtractionStampList *stamps, pmSubtractionKernels *kernels) {

    if (!pmVisualTestLevel("ppsub.fit", 1)) return true;

    if (!pmVisualInitWindow(&kapa1, "ppSub:Images")) return false;
    if (!pmVisualInitWindow(&kapa2, "ppSub:Misc")) return false;

    // set up holding images for the visualization
    pmSubtractionVisualShowFitInit (stamps);

    int numKernels = kernels->num;      // Number of kernels

    psImage *polyValues = NULL;         // Polynomial values
    psKernel *residual = psKernelAlloc(-stamps->footprint, stamps->footprint, -stamps->footprint, stamps->footprint); // Residual image

    double norm = p_pmSubtractionSolutionNorm(kernels); // Normalisation

    for (int i = 0; i < stamps->num; i++) {
        pmSubtractionStamp *stamp = stamps->stamps->data[i]; // The stamp of interest
        if (stamp->status != PM_SUBTRACTION_STAMP_USED) { continue; }

        // Calculate coefficients of the kernel basis functions
        polyValues = p_pmSubtractionPolynomial(polyValues, kernels->spatialOrder, stamp->xNorm, stamp->yNorm);
        double background = p_pmSubtractionSolutionBackground(kernels, polyValues); // Difference in background

        psImageInit(residual->image, 0.0);

        if (kernels->mode != PM_SUBTRACTION_MODE_DUAL) {
	    psKernel *target;           // Target postage stamp
	    psKernel *source;           // Source postage stamp
	    psArray *convolutions;      // Convolution postage stamps for each kernel basis function
            switch (kernels->mode) {
              case PM_SUBTRACTION_MODE_1:
		target = stamp->image2;
		source = stamp->image1;
		convolutions = stamp->convolutions1;
		break;
	      case PM_SUBTRACTION_MODE_2:
		target = stamp->image1;
		source = stamp->image2;
		convolutions = stamp->convolutions2;
		break;
	      default:
		psAbort("Unsupported subtraction mode: %x", kernels->mode);
	    }

	    for (int j = 0; j < numKernels; j++) {
		psKernel *convolution = convolutions->data[j]; // Convolution
		double coefficient = p_pmSubtractionSolutionCoeff(kernels, polyValues, j, false); // Coefficient
		for (int y = - footprint; y <= footprint; y++) {
		    for (int x = - footprint; x <= footprint; x++) {
			residual->kernel[y][x] += convolution->kernel[y][x] * coefficient;
		    }
		}
	    }
	    // visualize the target, source, convolution and residual
	    pmSubtractionVisualShowFitAddStamp (target, source, residual, background, norm, i);
	} else {
	    // Dual convolution
	    psArray *convolutions1 = stamp->convolutions1; // Convolutions of the first image
	    psArray *convolutions2 = stamp->convolutions2; // Convolutions of the second image
	    psKernel *image1 = stamp->image1; // The first image
	    psKernel *image2 = stamp->image2; // The second image

	    for (int j = 0; j < numKernels; j++) {
		psKernel *conv1 = convolutions1->data[j]; // Convolution of first image
		psKernel *conv2 = convolutions2->data[j]; // Convolution of second image
		double coeff1 = p_pmSubtractionSolutionCoeff(kernels, polyValues, j, false); // Coefficient 1
		double coeff2 = p_pmSubtractionSolutionCoeff(kernels, polyValues, j, true); // Coefficient 2

		for (int y = - footprint; y <= footprint; y++) {
		    for (int x = - footprint; x <= footprint; x++) {
			residual->kernel[y][x] += conv2->kernel[y][x] * coeff2 + conv1->kernel[y][x] * coeff1;
		    }
		}
	    }
	    // visualize the target, source, convolution and residual
	    pmSubtractionVisualShowFitAddStamp (image2, image1, residual, background, norm, i);
	}
	psFree(polyValues);
    }
    pmSubtractionVisualShowFitImage(norm);

    psFree (residual);
    return true;
}

// generate 4 storage images large enough to hold the stamps:
bool pmSubtractionVisualShowFitInit(pmSubtractionStampList *stamps) {

    footprint = stamps->footprint;

    float NXf = sqrt(stamps->num);
    NX = (int) NXf == NXf ? NXf : NXf + 1.0;
    
    float NYf = stamps->num / NX;
    NY = (int) NYf == NY ? NYf : NYf + 1.0;

    int NXpix = (2*footprint + 1) * NX;
    NXpix += (NX > 1) ? 3 * NX : 0;

    int NYpix = (2*footprint + 1) * NY;
    NYpix += (NY > 1) ? 3 * NY : 0;

    sourceImage      = psImageAlloc (NXpix, NYpix, PS_TYPE_F32);
    targetImage      = psImageAlloc (NXpix, NYpix, PS_TYPE_F32);
    residualImage    = psImageAlloc (NXpix, NYpix, PS_TYPE_F32);
    fresidualImage   = psImageAlloc (NXpix, NYpix, PS_TYPE_F32);
    differenceImage  = psImageAlloc (NXpix, NYpix, PS_TYPE_F32);
    convolutionImage = psImageAlloc (NXpix, NYpix, PS_TYPE_F32);
    
    psImageInit (sourceImage,      0.0);
    psImageInit (targetImage,      0.0);
    psImageInit (residualImage,    0.0);
    psImageInit (fresidualImage,   0.0);
    psImageInit (differenceImage,  0.0);
    psImageInit (convolutionImage, 0.0);

    return true;
}

bool pmSubtractionVisualShowFitAddStamp(psKernel *target, psKernel *source, psKernel *convolution, double background, double norm, int index) {

    double sum;

    int NXoff = index % NX;
    int NYoff = index / NX;

    int NXpix = NXoff * (2*footprint + 1 + 3) + footprint;
    int NYpix = NYoff * (2*footprint + 1 + 3) + footprint;

    // insert the (target) kernel into the (target) image:
    sum = 0.0;
    for (int y = -footprint; y <= footprint; y++) {
	for (int x = -footprint; x <= footprint; x++) {
	    targetImage->data.F32[y + NYpix][x + NXpix] = target->kernel[y][x];
	    sum += targetImage->data.F32[y + NYpix][x + NXpix];
	}
    }
    targetImage->data.F32[footprint + 1 + NYpix][NXpix] = sum;

    // insert the (source) kernel into the (source) image:
    sum = 0.0;
    for (int y = -footprint; y <= footprint; y++) {
	for (int x = -footprint; x <= footprint; x++) {
	    sourceImage->data.F32[y + NYpix][x + NXpix] = source->kernel[y][x];
	    sum += sourceImage->data.F32[y + NYpix][x + NXpix];
	}
    }
    sourceImage->data.F32[footprint + 1 + NYpix][NXpix] = sum;

    // insert the (convolution) kernel into the (convolution) image:
    sum = 0.0;
    for (int y = -footprint; y <= footprint; y++) {
	for (int x = -footprint; x <= footprint; x++) {
	    convolutionImage->data.F32[y + NYpix][x + NXpix] = convolution->kernel[y][x];
	    sum += convolutionImage->data.F32[y + NYpix][x + NXpix];
	}
    }
    convolutionImage->data.F32[footprint + 1 + NYpix][NXpix] = sum;
    
    // insert the (difference) kernel into the (difference) image:
    sum = 0.0;
    for (int y = -footprint; y <= footprint; y++) {
	for (int x = -footprint; x <= footprint; x++) {
	    differenceImage->data.F32[y + NYpix][x + NXpix] = target->kernel[y][x] - background - source->kernel[y][x] * norm;
	    sum += differenceImage->data.F32[y + NYpix][x + NXpix];
	}
    }
    differenceImage->data.F32[footprint + 1 + NYpix][NXpix] = sum;

    // insert the (residual) kernel into the (residual) image:
    sum = 0.0;
    for (int y = -footprint; y <= footprint; y++) {
	for (int x = -footprint; x <= footprint; x++) {
	    residualImage->data.F32[y + NYpix][x + NXpix] = target->kernel[y][x] - background - source->kernel[y][x] * norm - convolution->kernel[y][x];
	    sum += residualImage->data.F32[y + NYpix][x + NXpix];
	}
    }
    residualImage->data.F32[footprint + 1 + NYpix][NXpix] = sum;

    // insert the (fresidual) kernel into the (fresidual) image:
    for (int y = -footprint; y <= footprint; y++) {
	for (int x = -footprint; x <= footprint; x++) {
	    fresidualImage->data.F32[y + NYpix][x + NXpix] = residualImage->data.F32[y + NYpix][x + NXpix] / sqrt(PS_MAX(target->kernel[y][x], 100.0));
	}
    }
    return true;
}

bool pmSubtractionVisualShowFitImage(double norm) {

    KiiEraseOverlay (kapa1, "red");
    KiiEraseOverlay (kapa2, "red");

    pmVisualShowImage(kapa1, targetImage, "Target", 0, -200.0, 400.0);
    pmVisualShowImage(kapa1, sourceImage, "Source", 1, -200.0, 400.0);
    pmVisualShowImage(kapa1, convolutionImage, "Convolution", 2, -200.0, 400.0);
    KiiCenter (kapa1, 0.5*targetImage->numCols, 0.5*targetImage->numRows, 1);

    pmVisualScaleImage(kapa2, fresidualImage, "Frac.Residual", 2, true);
    pmVisualShowImage(kapa2, differenceImage, "Difference", 0, -200.0, 400.0);
    pmVisualShowImage(kapa2, residualImage, "Residual", 1, -200.0, 400.0);
    KiiCenter (kapa2, 0.5*residualImage->numCols, 0.5*residualImage->numRows, 1);

    pmVisualAskUser(NULL);

    psFree(targetImage);
    psFree(sourceImage);
    psFree(convolutionImage);
    psFree(differenceImage);
    psFree(residualImage);
    psFree(fresidualImage);

    targetImage = NULL;
    sourceImage = NULL;
    convolutionImage = NULL;
    differenceImage = NULL;
    residualImage = NULL;
    fresidualImage = NULL;

    return true;
}

bool pmSubtractionVisualPlotFit(const pmSubtractionKernels *kernels) {

    Graphdata graphdata;

    if (!pmVisualTestLevel("ppsub.fit", 1)) return true;

    if (!pmVisualInitWindow(&kapa3, "ppSub:plots")) return false;

    KapaClearSections (kapa3);
    KapaInitGraph (&graphdata);

    psVector *x = psVectorAllocEmpty (kernels->num, PS_TYPE_F32);
    psVector *y = psVectorAllocEmpty (kernels->num, PS_TYPE_F32);
    psVector *dy = psVectorAllocEmpty (kernels->num, PS_TYPE_F32);

    graphdata.xmin = -1.0;
    graphdata.xmax = kernels->num + 1.0;
    graphdata.ymin = +32.0;
    graphdata.ymax = -32.0;

    psImage *polyValues = p_pmSubtractionPolynomial(NULL, kernels->spatialOrder, 0.0, 0.0);

    // construct the plot vectors
    for (int i = 0; i < kernels->num; i++) {
        x->data.F32[i] = i;
	y->data.F32[i] = p_pmSubtractionSolutionCoeff(kernels, polyValues, i, false);
	dy->data.F32[i] = kernels->solution1err->data.F64[i];
        graphdata.ymin = PS_MIN(graphdata.ymin, y->data.F32[i]);
        graphdata.ymax = PS_MAX(graphdata.ymax, y->data.F32[i]);
    }
    x->n = y->n = dy->n = kernels->num;

    float range;
    range = graphdata.xmax - graphdata.xmin;
    graphdata.xmax += 0.05*range;
    graphdata.xmin -= 0.05*range;
    range = graphdata.ymax - graphdata.ymin;
    graphdata.ymax += 0.05*range;
    graphdata.ymin -= 0.05*range;

    KapaSetLimits (kapa3, &graphdata);

    KapaSetFont (kapa3, "helvetica", 14);
    KapaBox (kapa3, &graphdata);
    KapaSendLabel (kapa3, "kernel number", KAPA_LABEL_XM);
    KapaSendLabel (kapa3, "coeff", KAPA_LABEL_YM);

    graphdata.color = KapaColorByName ("black");
    graphdata.ptype = KAPA_POINT_CROSS;
    graphdata.size = 0.5;
    graphdata.style = KAPA_PLOT_POINTS;
    graphdata.etype |= 0x01;

    KapaPrepPlot   (kapa3, x->n, &graphdata);
    KapaPlotVector (kapa3, x->n, x->data.F32, "x");
    KapaPlotVector (kapa3, x->n, y->data.F32, "y");
    KapaPlotVector (kapa3, x->n, dy->data.F32, "dym");
    KapaPlotVector (kapa3, x->n, dy->data.F32, "dyp");

    psFree (x);
    psFree (y);
    psFree (dy);
    psFree (polyValues);

    pmVisualAskUser(NULL);
    return true;
}

// plot log(flux) vs log(chisq), log(flux) vs log(moments), log(chisq) vs log(moments)
bool pmSubtractionVisualPlotChisqAndMoments(psVector *fluxes, psVector *chisq, psVector *moments) {

    Graphdata graphdata;
    KapaSection section;

    if (!pmVisualTestLevel("ppsub.fit", 1)) return true;

    if (!pmVisualInitWindow(&kapa3, "ppSub:plots")) return false;

    KapaClearSections (kapa3);
    KapaInitGraph (&graphdata);
    KiiResize(kapa3, 1500, 500);

    psVector *lchi = psVectorAlloc (fluxes->n, PS_TYPE_F32);
    psVector *lflx = psVectorAlloc (fluxes->n, PS_TYPE_F32);
    psVector *lMxx = psVectorAlloc (fluxes->n, PS_TYPE_F32);

    // construct the plot vectors
    for (int i = 0; i < fluxes->n; i++) {
        lchi->data.F32[i] = log10(chisq->data.F32[i]);
        lflx->data.F32[i] = log10(fluxes->data.F32[i]);
        lMxx->data.F32[i] = log10(moments->data.F32[i]);
    }

    section.bg = KapaColorByName ("none"); // XXX probably should be 'none'

    // section 1: lflux vs lchi
    section.dx = 0.33;
    section.dy = 1.00;
    section.x  = 0.00;
    section.y  = 0.00;
    section.name = psStringCopy ("flux.v.chi");
    KapaSetSection (kapa3, &section);
    psFree (section.name);

    graphdata.color = KapaColorByName ("black");
    pmVisualScaleGraphdata(&graphdata, lflx, lchi, false);
    KapaSetLimits (kapa3, &graphdata);

    KapaSetFont (kapa3, "helvetica", 14);
    KapaBox (kapa3, &graphdata);
    KapaSendLabel (kapa3, "log(flux)", KAPA_LABEL_XM);
    KapaSendLabel (kapa3, "log(chisq)", KAPA_LABEL_YM);

    graphdata.color = KapaColorByName ("black");
    graphdata.ptype = KAPA_POINT_CROSS;
    graphdata.size = 0.5;
    graphdata.style = KAPA_PLOT_POINTS;

    KapaPrepPlot   (kapa3, lflx->n, &graphdata);
    KapaPlotVector (kapa3, lflx->n, lflx->data.F32, "x");
    KapaPlotVector (kapa3, lflx->n, lchi->data.F32, "y");

    // section 2: lflux vs lMxx
    section.dx = 0.33;
    section.dy = 1.00;
    section.x  = 0.33;
    section.y  = 0.00;
    section.name = psStringCopy ("flux.v.mom");
    KapaSetSection (kapa3, &section);
    psFree (section.name);

    graphdata.color = KapaColorByName ("black");
    pmVisualScaleGraphdata(&graphdata, lflx, lMxx, false);
    KapaSetLimits (kapa3, &graphdata);

    KapaSetFont (kapa3, "helvetica", 14);
    KapaBox (kapa3, &graphdata);
    KapaSendLabel (kapa3, "log(flux)", KAPA_LABEL_XM);
    KapaSendLabel (kapa3, "log(moments)", KAPA_LABEL_YM);

    graphdata.color = KapaColorByName ("black");
    graphdata.ptype = KAPA_POINT_CROSS;
    graphdata.size = 0.5;
    graphdata.style = KAPA_PLOT_POINTS;

    KapaPrepPlot   (kapa3, lflx->n, &graphdata);
    KapaPlotVector (kapa3, lflx->n, lflx->data.F32, "x");
    KapaPlotVector (kapa3, lflx->n, lMxx->data.F32, "y");

    // section 1: lflux vs lchi
    section.dx = 0.33;
    section.dy = 1.00;
    section.x  = 0.66;
    section.y  = 0.00;
    section.name = psStringCopy ("chi.v.mom");
    KapaSetSection (kapa3, &section);
    psFree (section.name);

    graphdata.color = KapaColorByName ("black");
    pmVisualScaleGraphdata(&graphdata, lchi, lMxx, false);
    KapaSetLimits (kapa3, &graphdata);

    KapaSetFont (kapa3, "helvetica", 14);
    KapaBox (kapa3, &graphdata);
    KapaSendLabel (kapa3, "log(chisq)", KAPA_LABEL_XM);
    KapaSendLabel (kapa3, "log(moments)", KAPA_LABEL_YM);

    graphdata.color = KapaColorByName ("black");
    graphdata.ptype = KAPA_POINT_CROSS;
    graphdata.size = 0.5;
    graphdata.style = KAPA_PLOT_POINTS;

    KapaPrepPlot   (kapa3, lflx->n, &graphdata);
    KapaPlotVector (kapa3, lflx->n, lchi->data.F32, "x");
    KapaPlotVector (kapa3, lflx->n, lMxx->data.F32, "y");

    psFree (lflx);
    psFree (lchi);
    psFree (lMxx);

    pmVisualAskUser(NULL);
    return true;
}

#else
bool pmSubtractionVisualClose(void) {return true;}
bool pmSubtractionVisualPlotConvKernels(psImage *convKernels) {return true;}
bool pmSubtractionVisualPlotStamps(pmSubtractionStampList *stamps, pmReadout *ro) {return true;}
bool pmSubtractionVisualPlotLeastSquares(pmSubtractionStampList *stamps) {return true;}
bool pmSubtractionVisualShowSubtraction(psImage *image, psImage *ref, psImage *sub) {return true;}
bool pmSubtractionVisualShowFitInit(pmSubtractionStampList *stamps) {return true;}
bool pmSubtractionVisualShowFitAddStamp(psKernel *target, psKernel *source, psKernel *convolution, double background, double norm, int index) {return true;}
bool pmSubtractionVisualShowFit() {return true;}
bool pmSubtractionVisualPlotFit(const pmSubtractionKernels *kernels);
#endif

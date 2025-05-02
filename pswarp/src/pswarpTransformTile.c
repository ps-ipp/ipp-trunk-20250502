/** @file pswarpTransformTile.c
 *
 *  @brief
 *
 *  @ingroup pswarp
 *
 *  @author IfA
 *  @version $Revision: 1.9 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-02-06 03:10:36 $
 *  Copyright 2009 Institute for Astronomy, University of Hawaii
 */

#include "pswarp.h"

static void transformTileArgsFree(pswarpTransformTileArgs *args)
{
    psFree(args->input);
    psFree(args->output);
    psFree(args->grid);
    psFree(args->interp);
    psFree(args->region);
    psFree(args->covariance);
    return;
}

pswarpTransformTileArgs *pswarpTransformTileArgsAlloc(void)
{
    pswarpTransformTileArgs *args = psAlloc(sizeof(pswarpTransformTileArgs));
    psMemSetDeallocator(args, (psFreeFunc)transformTileArgsFree);

    args->input = NULL;
    args->output = NULL;
    args->grid = NULL;
    args->interp = NULL;
    args->region = NULL;

    args->gridX = 0;
    args->gridY = 0;

    args->goodPixels = 0;
    args->xMin = PS_MAX_S32;
    args->xMax = PS_MIN_S32;
    args->yMin = PS_MAX_S32;
    args->yMax = PS_MIN_S32;
    args->covariance = NULL;
    args->jacobian = NAN;

    args->background_warping = false;
    args->offset_x = 0.0;
    args->offset_y = 0.0;
    
    return args;
}

bool pswarpTransformTile(pswarpTransformTileArgs *args)
{
    pmReadout *input = args->input;        // Input readout
    psImage *inImage = input->image, *inMask = input->mask; // Input images
    pmReadout *output = args->output;      // Output readout
    psImage *outImage = output->image, *outVariance = output->variance, *outMask = output->mask; // Outputs

    int inNumCols = inImage->numCols, inNumRows = inImage->numRows; ///< Size of input image
    int outNumCols = outImage->numCols, outNumRows = outImage->numRows; ///< Size of output image
    int outCol0 = outImage->col0, outRow0 = outImage->row0; ///< Offset of output image

    psPlane minPt, maxPt;               ///< Minimum and maximum points for this tile
    pswarpMapGridCoordRange(args->grid, args->gridX, args->gridY, &minPt, &maxPt);

    // Dereference images for convenience
    psF32 **outImageData     = outImage->data.F32;
    psF32 **outVarData       = outVariance ? outVariance->data.F32 : NULL;
    psImageMaskType **outMaskData = outMask ? outMask->data.PS_TYPE_IMAGE_MASK_DATA : NULL;
    psImageMaskType **inMaskData  = inMask ? inMask->data.PS_TYPE_IMAGE_MASK_DATA : NULL;

    pswarpMap *map = args->grid->maps[args->gridX][args->gridY]; ///< Map for this tile
    psImage *region = args->region;     ///< Region to transform

    /** Bounds for iteration */
    int xMin = PS_MAX(minPt.x, 0);
    int xMax = PS_MIN(maxPt.x, outNumCols);
    int yMin = PS_MAX(minPt.y, 0);
    int yMax = PS_MIN(maxPt.y, outNumRows);

    double jacobian = fabs(map->Xx * map->Yy - map->Yx * map->Xy); // Jacobian of transformation
    double jacobian2 = PS_SQR(jacobian);                     // Square Jacobian

    // Iterate over the output image pixels (parent frame)
    long goodPixels = 0;                ///< Number of input pixels landing on the output image

    for (int y = yMin; y < yMax; y++) {

      int yOut = y - outRow0; ///< Position on output image

        for (int x = xMin; x < xMax; x++) {
            // Only transform those pixels requested
            if (region && region->data.PS_TYPE_IMAGE_MASK_DATA[y][x]) {
                continue;
            }

	    int xOut = x - outCol0;

	    // XXX the existing image may already have valid data -- probably should keep 
	    // the best, but for the moment, just keep the pixel which is not NAN
	    if (isfinite(outImageData[yOut][xOut])) continue;

            // pswarpMapApply converts the output coordinate (x,y) to the input coordinate.
            // both are in the parent frames of the input and output images.
            double xIn, yIn;            // Input pixel coordinates
            pswarpMapApply(&xIn, &yIn, map, x + 0.5, y + 0.5);

	    // This needs to use a more reliable method to do this offset and limiting
	    //	    if (args->interp->mode == 8) {
	    if (args->background_warping) {
	      //	      double xOffset = 177.0 / 400.0; // (modelsize * modelbinning - xsize) / 2.0
	      //	      double yOffset = 166.0 / 400.0; // (modelsize * modelbinning - ysize) / 2.0
	      xIn += args->offset_x;
	      yIn += args->offset_y;

	      if ((xIn > inNumCols - args->offset_x)||
		  (yIn > inNumRows - args->offset_y)||
		  (xIn < args->offset_x)||
		  (yIn < args->offset_y)) {
		continue;
	      }
	    }
	    
            if (xIn < 0 || xIn >= inNumCols || yIn < 0 || yIn >= inNumRows) {
                continue;
            }

            // psImagePixelInterpolate determines the value at pixel coordinate (x,y) in child coordinates
            double imageValue, varValue; // Value of image and variance map
            psImageMaskType maskValue = inMaskData ? inMaskData[(int)yIn][(int)xIn] : 0; // Value of mask

	    if (!psImageInterpolate(&imageValue, &varValue, &maskValue, xIn, yIn, args->interp)) {
                psError(psErrorCodeLast(), false, "Unable to interpolate image.");
                return false;
            }

            if (outImageData) {
	      // XXX TEST outImageData[yOut][xOut] = value;
	      outImageData[yOut][xOut] = imageValue * jacobian;
            }
            if (outVarData) {
                outVarData[yOut][xOut] = varValue * jacobian2;
            }
            if (outMaskData) {
                outMaskData[yOut][xOut] = maskValue;
            }

            goodPixels++;
        }
    }

    if (goodPixels > 0) {
        float xOut = 0.5 * (xMin + xMax), yOut = 0.5 * (yMin + yMax); // Position of interest on output
        double xIn, yIn;                // Position of interest on input
        pswarpMapApply(&xIn, &yIn, map, xOut + 0.5, yOut + 0.5);
        psKernel *kernel = psImageInterpolationKernel(xIn, yIn, args->interp->mode); // Interpolation kernel
        args->covariance = psImageCovarianceCalculate(kernel, args->input->covariance);
        args->jacobian = sqrt(jacobian);
        psFree(kernel);
    }

    args->goodPixels = goodPixels;
    args->xMin = xMin;
    args->xMax = xMax;
    args->yMin = yMin;
    args->yMax = yMax;

    return true;
}

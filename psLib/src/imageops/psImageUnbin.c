/** @file  psImageUnbin.c
 *
 *  @brief Functions to perform unbinning (resampling) of images.
 *
 *  @ingroup Image
 *
 *  @author Eugene Magnier, IfA
 *
 *  @version $Revision: 1.11 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2008-12-19 00:24:45 $
 *
 *  Copyright 2007 Institute for Astronomy, University of Hawaii
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdio.h>
#include "psError.h"
#include "psAssert.h"
#include "psRegion.h"
#include "psImage.h"
#include "psImageBinning.h"
#include "psImageUnbin.h"

# define UNBIN_TOL 1e-4

// interpolate from model to background (~bresenham linear interpolation)
// DX, DY are the binning factor
// dx, dy is the distance is high-res pixels to the 0,0 corner of the first
// binned pixel.
// XXX check that this is still consistent with psphotImageMedian...
psImage *psImageUnbin(psImage *out, const psImage *in, const psImageBinning *binning)
{
    PS_ASSERT_IMAGE_NON_NULL(in, NULL);
    PS_ASSERT_IMAGE_NON_NULL(out, NULL);

    int DX = binning->nXbin;
    int DY = binning->nYbin;
    // int dx = binning->nXskip;
    // int dy = binning->nYskip;

    PS_ASSERT_INT_POSITIVE(DX, NULL);
    PS_ASSERT_INT_POSITIVE(DY, NULL);
    // PS_ASSERT_INT_LESS_THAN_OR_EQUAL(dx, DX, NULL);
    // PS_ASSERT_INT_LESS_THAN_OR_EQUAL(dy, DY, NULL);

    long nx = in->numCols;
    long ny = in->numRows;
    long Nx = out->numCols;
    long Ny = out->numRows;

    // XXX validate the binning structure vs the input and output images?

    psF32 **vIn = in->data.F32;
    psF32 **vOut = out->data.F32;

    int col0 = out->col0;
    int row0 = out->row0;

    // loop over all input pixels excluding the last
    for (int Iy = 0; Iy < ny-1; Iy ++) {
        for (int Ix = 0; Ix < nx-1; Ix ++) {

            float V00 = vIn[Iy+0][Ix+0];
            float V01 = vIn[Iy+0][Ix+1];
            float V10 = vIn[Iy+1][Ix+0];
            float V11 = vIn[Iy+1][Ix+1];

            // a single binned pixel quad
            // (Xs,Ys) : (Xe,Ye) : binned pixel centers in unbinned coords
            // corresponding to (Ix,Iy), (Ix+1,Iy+1)
            // XXX should this be "+ dx" and + dy?
	    // Ix,Iy are in input pixels coords; result of GetFine is in the 
	    // data space of the output, so we need to subtract col0,row0 to get raw pixels
            int Xs = PS_MAX (0, PS_MIN (Nx, psImageBinningGetFineX(binning, Ix + 0.5) - col0));
            int Ys = PS_MAX (0, PS_MIN (Ny, psImageBinningGetFineY(binning, Iy + 0.5) - row0));
            int Xe = PS_MAX (0, PS_MIN (Nx, Xs + DX));
            int Ye = PS_MAX (0, PS_MIN (Ny, Ys + DY));
	    // fprintf (stderr, "%f, %f, %f, %f : %d, %d : %d - %d, %d - %d\n", V00, V01, V10, V11, Ix, Iy, Xs, Xe, Ys, Ye);

            for (int iy = Ys; (iy < Ye) && (iy < Ny); iy++) {
                float dY = (iy - Ys) / (float) DY;
                float rY = 1.0 - dY;
                float Vxs = V10*dY + V00*rY;
                float Vxe = V11*dY + V01*rY;

                // Vxs = (V10 - V00)*(iy - Ys) / DY + V00;
                // Vxe = (V11 - V01)*(iy - Ys) / DY + V01;

                // dVx = Vxs_1 - Vxs_1
                // dVx = (V10*dY_1 + V00*rY_1) - (V10*dY_0 + V00*rY_0);
                // dY_0 = (iy - Ys)/DY     = iy/DY - Ys/DY;
                // dY_1 = (iy + 1 - Ys)/DY = iy/DY - Ys/DY + 1/DY;
                // rY_0 = 1 - dY_0;
                // rY_1 = 1 - dY_1;
                // dVx = V10*(ddY) + V00*(drY);
                // ddY = 1/DY;
                // drY = -1/DY;

                // dVxs = (V10 - V00)/DY;
                // dVxe = (V11 - V01)/DY;

                // ddV = (Vxe_1 - Vxs_1)/DX - (Vxe_0 - Vxs_0)/DX;
                // ddV = (Vxe_1 - Vxe_0)/DX - (Vxs_1 - Vxs_0)/DX;
                // ddV = (V11 - V01 - V10 + V00)/(DX*DY);

                float dV = (Vxe - Vxs) / DX;
                float V  = Vxs;
                for (int ix = Xs; (ix < Xe) && (ix < Nx); ix++) {
                    vOut[iy][ix] = V;
                    // assert (fabs(V - psImageUnbinPixel(ix, iy, in, binning)) < UNBIN_TOL*fabs(V));
                    V += dV;
                }
            }
        }
    }

    // side pixels
    int Xs = PS_MAX (0, PS_MIN (Nx, psImageBinningGetFineX(binning, 0  + 0.5) - col0));
    int Xe = PS_MAX (0, PS_MIN (Nx, psImageBinningGetFineX(binning, nx - 0.5) - col0));
    for (int Iy = 0; Iy < ny - 1; Iy++) {

        int Ys = PS_MAX (0, PS_MIN (Ny, psImageBinningGetFineY(binning, Iy + 0.5) - row0));
        int Ye = PS_MAX (0, PS_MIN (Ny, Ys + DY));

        // leading edge
        float V0 = vIn[Iy+0][0];
        float V1 = vIn[Iy+1][0];
        float dV = (V1 - V0) / DY;
        float V = V0;
        for (int iy = Ys; (iy < Ye) && (iy < Ny); iy++) {
            for (int ix = 0; ix < Xs; ix++) {
                vOut[iy][ix] = V;
                // assert (fabs(V - psImageUnbinPixel(ix, iy, in, binning)) < UNBIN_TOL*fabs(V));
            }
            V += dV;
        }

        // trailing edge
        V0 = vIn[Iy+0][nx-1];
        V1 = vIn[Iy+1][nx-1];
        dV = (V1 - V0) / DY;
        V = V0;
        for (int iy = Ys; (iy < Ye) && (iy < Ny); iy++) {
            for (int ix = Xe; ix < Nx; ix++) {
                vOut[iy][ix] = V;
                // assert (fabs(V - psImageUnbinPixel(ix, iy, in, binning)) < UNBIN_TOL*fabs(V));
            }
            V += dV;
        }
    }

    // top and bottom pixels
    int Ys = PS_MAX (0, PS_MIN (Ny, psImageBinningGetFineY(binning, 0  + 0.5) - row0));
    int Ye = PS_MAX (0, PS_MIN (Ny, psImageBinningGetFineY(binning, ny - 0.5) - row0));
    for (int Ix = 0; Ix < nx - 1; Ix++) {

        int Xs = PS_MAX (0, PS_MIN (Nx, psImageBinningGetFineX(binning, Ix + 0.5) - col0));
        int Xe = PS_MAX (0, PS_MIN (Nx, Xs + DX));

        // top edge
        float V0 = vIn[0][Ix+0];
        float V1 = vIn[0][Ix+1];
        float dV = (V1 - V0) / DX;
        for (int iy = 0; iy < Ys; iy++) {
            float V = V0;
            for (int ix = Xs; (ix < Xe) && (ix < Nx); ix++) {
                vOut[iy][ix] = V;
                // assert (fabs(V - psImageUnbinPixel(ix, iy, in, binning)) < UNBIN_TOL*fabs(V));
                V += dV;
            }
        }

        // bottom edge
        V0 = vIn[ny-1][Ix+0];
        V1 = vIn[ny-1][Ix+1];
        dV = (V1 - V0) / DX;
        for (int iy = Ye; iy < Ny; iy++) {
            float V = V0;
            for (int ix = Xs; (ix < Xe) && (ix < Nx); ix++) {
                vOut[iy][ix] = V;
                // assert (fabs(V - psImageUnbinPixel(ix, iy, in, binning)) < UNBIN_TOL*fabs(V));
                V += dV;
            }
        }
    }
    // return out;

    // the four corners
    {
        float V;
        // center of last pixel
        int Xs = PS_MAX (0, PS_MIN (Nx, psImageBinningGetFineX(binning, 0  + 0.5) - col0));
        int Xe = PS_MAX (0, PS_MIN (Nx, psImageBinningGetFineX(binning, nx - 0.5) - col0));
        int Ys = PS_MAX (0, PS_MIN (Ny, psImageBinningGetFineY(binning, 0  + 0.5) - row0));
        int Ye = PS_MAX (0, PS_MIN (Ny, psImageBinningGetFineY(binning, ny - 0.5) - row0));

        // 0,0
        V = vIn[0][0];
        // assert (fabs(V - psImageUnbinPixel(0, 0, in, binning)) < UNBIN_TOL*fabs(V));

        for (int iy = 0; iy < Ys; iy++)
        {
            for (int ix = 0; ix < Xs; ix++) {
                vOut[iy][ix] = V;
            }
        }
        // Nx,0
        V = vIn[0][nx-1];
        // assert (fabs(V - psImageUnbinPixel(Nx-1, 0, in, binning)) < UNBIN_TOL*fabs(V));

        for (int iy = 0; iy < Ys; iy++)
        {
            for (int ix = Xe; ix < Nx; ix++) {
                vOut[iy][ix] = V;
            }
        }
        // 0,Ny
        V = vIn[ny-1][0];
        // assert (fabs(V - psImageUnbinPixel(0, Ny-1, in, binning)) < UNBIN_TOL*fabs(V));

        for (int iy = Ye; iy < Ny; iy++)
        {
            for (int ix = 0; ix < Xs; ix++) {
                vOut[iy][ix] = V;
            }
        }
        // Nx,Ny
        V = vIn[ny-1][nx-1];
        // assert (fabs(V - psImageUnbinPixel(Nx-1, Ny-1, in, binning)) < UNBIN_TOL*fabs(V));

        for (int iy = Ye; iy < Ny; iy++)
        {
            for (int ix = Xe; ix < Nx; ix++) {
                vOut[iy][ix] = V;
            }
        }
    }

    return out;
}

/************************************************************************************************************/
/*
 * Get the value of a single unbinned pixel from the binned representation
 */

double psImageUnbinPixel(const double xFine, const double yFine, // desired Unbinned point (parent coords)
                         const psImage *in, // binned image
                         const psImageBinning *binning)   //!< Overhang
{
    PS_ASSERT_IMAGE_NON_NULL(in, NAN);
    assert (in->type.type == PS_TYPE_F32);

    const float xRuff = psImageBinningGetRuffX (binning, xFine);
    const float yRuff = psImageBinningGetRuffY (binning, yFine);

    const double value = psImageInterpolatePixelBilinear (xRuff, yRuff, in);

    return value;
}

// fast & simple API to interpolate to a subpixel position using bilinear interpolation
// x,y in parent image coordinates (pixel centers at 0.5, 0.5)
double psImageInterpolatePixelBilinear (const double xIn, const double yIn, const psImage *in) {

    PS_ASSERT_PTR_NON_NULL(in, PS_ERR_BAD_PARAMETER_VALUE);
    assert (in->type.type == PS_TYPE_F32);

    const double x = xIn - in->col0;
    const double y = yIn - in->row0;

    // allow extrapolation a small distance beyond the edge of valid pixels, but no
    // further (this allows the nXskip,nYskip boundary areas to be used as well)
    float nXedge = 0.125*in->numCols;
    float nYedge = 0.125*in->numRows;

    if ((x < -nXedge) || (x > in->numCols + nXedge) || (y < -nYedge) || (y > in->numRows + nYedge)) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, "Point (%lf,%lf) lies outside binned image", x, y);
        return NAN;
    }

    // limiting cases: Nx == 1 and/or Ny == 1

    // if we have a single pixel, there is no spatial information
    if ((in->numCols == 1) && (in->numRows == 1)) {
        const double value = in->data.F32[0][0];
        return value;
    }

    // handle edge cases with extrapolation

    const int ix = x - 0.5; // index of reference pixel
    const int iy = y - 0.5; // index of reference pixel

    // do numCols,Rows first so we are never < 0
    const int Xs = PS_MAX (PS_MIN (ix, in->numCols - 2), 0);
    const int Ys = PS_MAX (PS_MIN (iy, in->numRows - 2), 0);

    const int Xe = Xs + 1;
    const int Ye = Ys + 1;

    // dx,dy range from 0.0 to 1.0 for interpolated pixels, and -0.5 to 1.5 for extrapolation
    const double dx = x - 0.5 - Xs;
    const double dy = y - 0.5 - Ys;

    const double rx = 1.0 - dx;
    const double ry = 1.0 - dy;

    // if Nx == 1, we have no x-dir spatial information
    if (in->numCols == 1) {
        double V0 = in->data.F32[Ys][Xs];
        double V1 = in->data.F32[Ye][Xs];

        const double value = V0*ry + V1*dy;
        return value;
    }

    // if Ny == 1, we have no y-dir spatial information
    if (in->numRows == 1) {
        double V0 = in->data.F32[Ys][Xs];
        double V1 = in->data.F32[Ys][Xe];

        const double value = V0*rx + V1*dx;
        return value;
    }

    // Vxy
    double V00 = in->data.F32[Ys][Xs];
    double V01 = in->data.F32[Ye][Xs];
    double V10 = in->data.F32[Ys][Xe];
    double V11 = in->data.F32[Ye][Xe];

    double value;

    // corners
    if ((dx < 0.0) && (dy < 0.0)) {
        return V00;
    }
    if ((dx > 1.0) && (dy < 0.0)) {
        return V10;
    }
    if ((dx < 0.0) && (dy > 1.0)) {
        return V01;
    }
    if ((dx > 1.0) && (dy > 1.0)) {
        return V11;
    }

    // sides
    if (dx < 0.0) {
        value = V00*ry + V01*dy;
        return value;
    }
    if (dy < 0.0) {
        value = V00*rx + V10*dx;
        return value;
    }
    if (dx > 1.0) {
        value = V10*ry + V11*dy;
        return value;
    }
    if (dy > 1.0) {
        value = V01*rx + V11*dx;
        return value;
    }

    // bilinear interpolation
    value = V00*rx*ry + V10*dx*ry + V01*rx*dy + V11*dx*dy;
    return value;
}

# if (0)
/* Old version of this function
 * N.b. This code only works for the central part of the image; the edge
 * cases should be added
 * XXXX this is bilinear interpolation, but written sub-optimally
 * XXXX this function should be taking float input coordinates!!!
 */
double psImageUnbinPixel(const int ix, const int iy, // desired Unbinned point (parent coords)
                         const psImage *in, // binned image
                         const psImageBinning *binning)   //!< Overhang
{

    int DX = binning->nXbin;
    int DY = binning->nYbin;
    int dx = binning->nXskip;
    int dy = binning->nYskip;

    PS_ASSERT_IMAGE_NON_NULL(in, NAN);
    assert (in->type.type == PS_TYPE_F32);

    PS_ASSERT_INT_POSITIVE(DX, NAN);
    PS_ASSERT_INT_POSITIVE(DY, NAN);
    PS_ASSERT_INT_LESS_THAN_OR_EQUAL(dx, DX, NAN);
    PS_ASSERT_INT_LESS_THAN_OR_EQUAL(dy, DY, NAN);
    /*
     * Find which binned pixel we're in
     */

    const int Xs = (ix - dx)/DX; // index of binned pixel
    const int Ys = (iy - dy)/DY;
    if (Xs < 0 || Xs >= in->numCols || Ys < 0 || Ys >= in->numRows) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, "Point (%d,%d) lies outside binned image", ix, iy);
        return NAN;
    }
    const int Xe = PS_MIN (Xs + 1, in->numCols - 1);
    const int Ye = PS_MIN (Ys + 1, in->numRows - 1);

    double V00 = in->data.F32[Ys][Xs];
    double V01 = in->data.F32[Ys][Xe];
    double V10 = in->data.F32[Ye][Xs];
    double V11 = in->data.F32[Ye][Xe];

    const int xs = Xs*DX - dx; // centre of bottom left corner of binned pixel
    const int ys = Ys*DY - dy; // (i.e. [Xs][Ys]) in unbinned coordinates

    const double Vxs = (V10 - V00)*(iy - ys)/DY + V00;
    const double Vxe = (V11 - V01)*(iy - ys)/DY + V01;

    return Vxs + (Vxe - Vxs)*(ix - xs)/DX; // value at [iy][ix]
}

# endif


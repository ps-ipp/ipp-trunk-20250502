/// @file  psImageConvolve2dCache.c -- specialized 2D convolution for psf matching
/// @author Eugene Magnier, IfA
///
/// @date $Date: 2009-02-05 23:56:14 $
/// Copyright 2004-2007 Institute for Astronomy, University of Hawaii

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>
#include <math.h>
#include "psAbort.h"
#include "psMemory.h"
#include "psLogMsg.h"
#include "psError.h"
#include "psAssert.h"
#include "psScalar.h"
#include "psBinaryOp.h"
#include "psImageFFT.h"
#include "psImageStructManip.h"
#include "psImagePixelManip.h"
#include "psTrace.h"
#include "psThread.h"

#include "psImageConvolve.h"

// fixed set of radii and number of entries in circularly symmetric profile
# define NRAD_MAX 87
# define  RAD_MAX 15
static float radii2[NRAD_MAX] = {   0.0,   1.0,   2.0,   4.0,   5.0, 
				    8.0,   9.0,  10.0,  13.0,  16.0, 
				    17.0,  18.0,  20.0,  25.0,  26.0, 

				    29.0,  32.0,  34.0,  36.0,  37.0, 
				    40.0,  41.0,  45.0,  49.0,  50.0, 
				    52.0,  53.0,  58.0,  61.0,  64.0,

				    65.0,  68.0,  72.0,  73.0,  74.0, 
				    80.0,  81.0,  82.0,  85.0,  89.0,
				    90.0,  97.0,  98.0, 100.0, 101.0, 

				    104.0, 106.0, 109.0, 113.0, 116.0, 
				    117.0, 121.0, 122.0, 125.0, 128.0, 
				    130.0, 136.0, 137.0, 144.0, 145.0, 

				    146.0, 148.0, 149.0, 153.0, 157.0, 
				    160.0, 162.0, 164.0, 169.0, 170.0, 
				    173.0, 178.0, 180.0, 181.0, 185.0, 

				    193.0, 194.0, 196.0, 197.0, 200.0, 
				    202.0, 205.0, 208.0, 212.0, 218.0, 
				    221.0, 225.0
};

static int   radiiN[NRAD_MAX] = {    1,  4, 4,  4,  8,
				     4,  4, 8,  8,  4,
				     8,  4, 8, 12,  8, 

				     8,  4, 8,  4,  8,
				     8,  8, 8,  4, 12,
				     8,  8, 8,  8,  4, 

				     16, 8, 4,  8,  8, 
				     8, 4, 8, 16,  8, 
				     8, 8, 4, 12, 8,

				     8, 8, 8, 8, 8, 
				     8, 4, 8, 16, 4, 
				     16, 8, 8, 8, 16, 

				     8, 8, 8, 8, 8, 
				     8, 4, 8, 12, 16,
				     8, 8, 8, 8, 16,

				     8, 8, 4, 8, 12,
				     8, 16, 8, 8, 8, 
				     16, 12
};

# define ADD_AXIS(RAD,DD) s += radflux[RAD]*(vi[iy - DD][ix] + \
					     vi[iy + DD][ix] + \
					     vi[iy][ix - DD] + \
					     vi[iy][ix + DD]); 

# define ADD_DIAG(RAD,DD) s += radflux[RAD]*(vi[iy - DD][ix - DD] + \
					     vi[iy - DD][ix + DD] + \
					     vi[iy + DD][ix - DD] + \
					     vi[iy + DD][ix + DD]);

# define ADD_RAND(RAD,DX,DY) s += radflux[RAD]*(vi[iy - DY][ix - DX] + \
						vi[iy - DY][ix + DX] + \
						vi[iy + DY][ix - DX] + \
						vi[iy + DY][ix + DX] + \
						vi[iy - DX][ix - DY] + \
						vi[iy - DX][ix + DY] + \
						vi[iy + DX][ix - DY] + \
						vi[iy + DX][ix + DY]);

# if (0)
# define ADD_AXIS(RAD,DD) s += radflux[RAD]*(vi[p - DD] + vi[p + DD] + vi[p - DD*Nx] + vi[p + DD*Nx]);
# define ADD_DIAG(RAD,DD) s += radflux[RAD]*(vi[p - DD - DD*Nx] + vi[p + DD - DD*Nx] + vi[p - DD + DD*Nx] + vi[p + DD + DD*Nx]);
# define ADD_RAND(RAD,DX,DY) s += radflux[RAD]*(vi[p - DX - DY*Nx] + vi[p + DX - DY*Nx] + vi[p - DX + DY*Nx] + vi[p + DX + DY*Nx] + 
 						vi[p - DY - DX*Nx] + vi[p + DY - DX*Nx] + vi[p - DY + DX*Nx] + vi[p + DY + DX*Nx]);
# endif

void psImageSmooth2dCacheDataFree (psImageSmooth2dCacheData *smdata) {

    if (smdata->radflux == NULL) return;
    psFree (smdata->radflux);
}

// allocate the psImageSmooth2dCache data structure, but do not define the kernel
psImageSmooth2dCacheData *psImageSmooth2dCacheAlloc (float Nsigma) {

    psAssert (isfinite(Nsigma) && (Nsigma > 0.5), "Nsigma is not valid");

    psImageSmooth2dCacheData *smdata = psAlloc(sizeof(psImageSmooth2dCacheData));
    psMemSetDeallocator(smdata, (psFreeFunc) psImageSmooth2dCacheDataFree);

    smdata->radflux = NULL;
    smdata->Nsigma = Nsigma;
    smdata->Ns = -1;

    return smdata;
}

// generate a 2D smoothing kernel for supplied sigma & kappa (PS1_V1 profile).  
bool psImageSmooth2dCacheKernel_PS1_V1 (psImageSmooth2dCacheData *smdata, float sigma, float kappa) {

    // check for NULL structure elements?
    int Ns = (int)(smdata->Nsigma * sigma);
    Ns = PS_MAX (3, PS_MIN (Ns, RAD_MAX));
    smdata->Ns = Ns;

    int Ns2 = Ns * Ns;

    // we are going to use a hard-wired set of radial points
    smdata->radflux = psAlloc(sizeof(float)*NRAD_MAX);

    double sum = 0.0;
    for (int i = 0; i < NRAD_MAX; i++) {
	if (radii2[i] > Ns2) {
	    smdata->radflux[i] = 0.0;
	    continue;
	}
	float z = 0.5 * radii2[i] / PS_SQR(sigma);
	smdata->radflux[i] = 1.0 / (1.0 + kappa*z + pow(z,1.666));
	sum += radiiN[i] * smdata->radflux[i];
    }
    for (int i = 0; i < NRAD_MAX; i++) {
	smdata->radflux[i] = smdata->radflux[i] / sum;
    }

    return true;
}

// generate a 2D smoothing kernel for supplied sigma & kappa (PS1_V1 profile).  
bool psImageSmooth2dCacheKernel_Gauss (psImageSmooth2dCacheData *smdata, float sigma) {

    // check for NULL structure elements?
    int Ns = (int)(smdata->Nsigma * sigma);
    Ns = PS_MAX (3, PS_MIN (Ns, RAD_MAX));
    smdata->Ns = Ns;

    int Ns2 = Ns * Ns;

    // we are going to use a hard-wired set of radial points
    smdata->radflux = psAlloc(sizeof(float)*NRAD_MAX);

    float sum = 0.0;
    for (int i = 0; i < NRAD_MAX; i++) {
	if (radii2[i] > Ns2) {
	    smdata->radflux[i] = 0.0;
	    continue;
	}
	float z = 0.5 * radii2[i] / PS_SQR(sigma);
	smdata->radflux[i] = exp(-z);
	sum += radiiN[i] * smdata->radflux[i];
    }
    for (int i = 0; i < NRAD_MAX; i++) {
	smdata->radflux[i] = smdata->radflux[i] / sum;
    }

    return true;
}

// we can use the same DATA structure on multiple images of the same size
bool psImageSmooth2dCache_F32(psImage *image, psImageSmooth2dCacheData *smdata)
{
    PS_ASSERT_IMAGE_NON_NULL(image, false);
    PS_ASSERT_PTR_NON_NULL(smdata->radflux, false);
    // assert on data type

    // relevant terms
    int Ns = smdata->Ns;    // Number of pixels either side for convolution kernel
       
    int Nx = image->numCols;
    int Ny = image->numRows;

    int Nxtmp = Nx + 2*Ns;
    int Nytmp = Ny + 2*Ns;

    // copy input image into a buffer padded by Ns on either side
    float **vi = (float **) psAlloc(sizeof(float *)*Nytmp);
    for (int iy = 0; iy < Nytmp; iy ++) {
	int Iy = iy - Ns;
	vi[iy] = (float *) psAlloc(sizeof(float)*Nxtmp);
	memset (vi[iy], 0, sizeof(float)*Nxtmp);
	if (Iy < 0) continue;
	if (Iy >= image->numRows) continue;
	for (int Ix = 0; Ix < image->numCols; Ix ++) {
	    int ix = Ix + Ns;
	    vi[iy][ix] = image->data.F32[Iy][Ix];
	}
    }

    // set up the output buffer (different from input)
    float **vo = (float **) psAlloc(sizeof(float *)*Nytmp);
    for (int iy = 0; iy < Nytmp; iy ++) {
	vo[iy] = (float *) psAlloc(sizeof(float)*Nxtmp);
	memset (vo[iy], 0, sizeof(float)*Nxtmp);
    }

    float *radflux = smdata->radflux;

    // smooth in 2D (ix,iy is the coordinate in the input and output images, only need to
    // transform the region inside the padding.
    for (int iy = Ns; iy < Ny + Ns; iy ++) {
	for (int ix = Ns; ix < Nx + Ns; ix ++) {

	    float s = radflux[0] * vi[iy][ix];

	    // r <= 1.0
	    ADD_AXIS (1,  1);      // r^2 = 1

	    // r <= 2.0
	    ADD_DIAG (2,  1);      // r^2 = 2
	    ADD_AXIS (3,  2);      // r^2 = 4

	    // r <= 3.0
	    ADD_RAND (4,  1,  2);  // r^2 = 5
	    ADD_DIAG (5,  2);      // r^2 = 8
	    ADD_AXIS (6,  3);      // r^2 = 9
	    if (Ns <= 3) goto finish;

	    // r <= 4.0
	    ADD_RAND (7,  1,  3);  // r^2 = 10
	    ADD_RAND (8,  2,  3);  // r^2 = 13
	    ADD_AXIS (9,  4);      // r^2 = 16
	    if (Ns <= 4) goto finish;

	    // r <= 5.0
	    ADD_RAND (10,  1,  4); // r^2 = 17
	    ADD_DIAG (11,  3);     // r^2 = 18
	    ADD_RAND (12,  2,  4); // r^2 = 20
	    ADD_RAND (13,  3,  4); // r^2 = 25
	    ADD_AXIS (13,  5);     // r^2 = 25
	    if (Ns <= 5) goto finish;

	    // r <= 6.0
	    ADD_RAND (14,  1,  5); // r^2 = 26
	    ADD_RAND (15,  2,  5); // r^2 = 29
	    ADD_DIAG (16,  4);     // r^2 = 32
	    ADD_RAND (17,  3,  5); // r^2 = 34
	    ADD_AXIS (18,  6);     // r^2 = 36
	    if (Ns <= 6) goto finish;

	    // r <= 7.0
	    ADD_RAND (19,  1,  6); // r^2 = 37
	    ADD_RAND (20,  2,  6); // r^2 = 40
	    ADD_RAND (21,  4,  5); // r^2 = 41
	    ADD_RAND (22,  3,  6); // r^2 = 45
	    ADD_AXIS (23,  7);     // r^2 = 49
	    if (Ns <= 7) goto finish;

	    // r <= 8.0
	    ADD_RAND (24,  1,  7); // r^2 = 50
	    ADD_DIAG (24,  5);     // r^2 = 50
	    ADD_RAND (25,  4,  6); // r^2 =   52
	    ADD_RAND (26,  2,  7); // r^2 =   53
	    ADD_RAND (27,  3,  7); // r^2 =   58
	    ADD_RAND (28,  5,  6); // r^2 =   61
	    ADD_AXIS (29,  8);     // r^2 =   64
	    if (Ns <= 8) goto finish;

	    ADD_RAND (30,  1,  8); // r^2 =   65 *
	    ADD_RAND (30,  4,  7); // r^2 =   65 *
	    ADD_RAND (31,  2,  8); // r^2 =   68
	    ADD_DIAG (32,  6);     // r^2 =   72
	    ADD_RAND (33,  3,  8); // r^2 =   73
	    ADD_RAND (34,  5,  7); // r^2 =   74
	    ADD_RAND (35,  4,  8); // r^2 =   80
	    ADD_AXIS (36,  9);     // r^2 =   81
	    if (Ns <= 9) goto finish;

	    ADD_RAND (37,  1,  9); // r^2 =   82
	    ADD_RAND (38,  2,  9); // r^2 =   85 *
	    ADD_RAND (38,  6,  7); // r^2 =   85 *
	    ADD_RAND (39,  5,  8); // r^2 =   89
	    ADD_RAND (40,  3,  9); // r^2 =   90
	    ADD_RAND (41,  4,  9); // r^2 =   97
	    ADD_DIAG (42,  7);     // r^2 =   98
	    ADD_RAND (43,  6,  8); // r^2 =  100 *
	    ADD_AXIS (43, 10);     // r^2 =  100 *
	    if (Ns <= 10) goto finish;

	    ADD_RAND ( 44,  1, 10);  // r^2 = 101
	    ADD_RAND ( 45,  2, 10);  // r^2 = 104
	    ADD_RAND ( 46,  5,  9);  // r^2 = 106
	    ADD_RAND ( 47,  3, 10);  // r^2 = 109
	    ADD_RAND ( 48,  7,  8);  // r^2 = 113
	    ADD_RAND ( 49,  4, 10);  // r^2 = 116
	    ADD_RAND ( 50,  6,  9);  // r^2 = 117
	    ADD_AXIS ( 51, 11)    ;  // r^2 = 121
	    if (Ns <= 11) goto finish;

	    ADD_RAND ( 52,  1, 11);  // r^2 = 122
	    ADD_RAND ( 53,  2, 11);  // r^2 = 125 *
	    ADD_RAND ( 53,  5, 10);  // r^2 = 125 *
	    ADD_DIAG ( 54,  8)    ;  // r^2 = 128
	    ADD_RAND ( 55,  3, 11);  // r^2 = 130 *
	    ADD_RAND ( 55,  7,  9);  // r^2 = 130 *
	    ADD_RAND ( 56,  6, 10);  // r^2 = 136
	    ADD_RAND ( 57,  4, 11);  // r^2 = 137
	    ADD_AXIS ( 58, 12)    ;  // r^2 = 144
	    if (Ns <= 12) goto finish;

	    ADD_RAND ( 59,  1, 12);  // r^2 = 145 *
	    ADD_RAND ( 59,  8,  9);  // r^2 = 145 *
	    ADD_RAND ( 60,  5, 11);  // r^2 = 146
	    ADD_RAND ( 61,  2, 12);  // r^2 = 148
	    ADD_RAND ( 62,  7, 10);  // r^2 = 149
	    ADD_RAND ( 63,  3, 12);  // r^2 = 153
	    ADD_RAND ( 64,  6, 11);  // r^2 = 157
	    ADD_RAND ( 65,  4, 12);  // r^2 = 160
	    ADD_DIAG ( 66,  9)    ;  // r^2 = 162
	    ADD_RAND ( 67,  8, 10);  // r^2 = 164
	    ADD_RAND ( 68,  5, 12);  // r^2 = 169 *
	    ADD_AXIS ( 68, 13)    ;  // r^2 = 169 *
	    if (Ns <= 13) goto finish;

	    ADD_RAND ( 69,  1, 13);  // r^2 = 170 *
	    ADD_RAND ( 69,  7, 11);  // r^2 = 170 *
	    ADD_RAND ( 70,  2, 13);  // r^2 = 173
	    ADD_RAND ( 71,  3, 13);  // r^2 = 178
	    ADD_RAND ( 72,  6, 12);  // r^2 = 180
	    ADD_RAND ( 73,  9, 10);  // r^2 = 181
	    ADD_RAND ( 74,  4, 13);  // r^2 = 185 *
	    ADD_RAND ( 74,  8, 11);  // r^2 = 185 *
	    ADD_RAND ( 75,  7, 12);  // r^2 = 193
	    ADD_RAND ( 76,  5, 13);  // r^2 = 194
	    ADD_AXIS ( 77, 14)    ;  // r^2 = 196
	    if (Ns <= 14) goto finish;

	    ADD_RAND ( 78,  1, 14);  // r^2 = 197
	    ADD_DIAG ( 79, 10)    ;  // r^2 = 200 *
	    ADD_RAND ( 79,  2, 14);  // r^2 = 200 *
	    ADD_RAND ( 80,  9, 11);  // r^2 = 202
	    ADD_RAND ( 81,  3, 14);  // r^2 = 205 *
	    ADD_RAND ( 81,  6, 13);  // r^2 = 205 *
	    ADD_RAND ( 82,  8, 12);  // r^2 = 208
	    ADD_RAND ( 83,  4, 14);  // r^2 = 212
	    ADD_RAND ( 84,  7, 13);  // r^2 = 218
	    ADD_RAND ( 85, 10, 11);  // r^2 = 221 *
	    ADD_RAND ( 85,  5, 14);  // r^2 = 221 *
	    ADD_RAND ( 86,  9, 12);  // r^2 = 225 *
	    ADD_AXIS ( 86, 15);      // r^2 = 225 *
	    if (Ns <= 15) goto finish;

	finish:
	    vo[iy][ix] = s;
	}
    }
    
    for (int iy = 0; iy < Ny; iy ++) {
	int Iy = iy + Ns;
	for (int ix = 0; ix < Nx; ix ++) {
	    int Ix = ix + Ns;
	    image->data.F32[iy][ix] = vo[Iy][Ix];
	}
    }

    for (int iy = 0; iy < Nytmp; iy ++) {
	psFree (vi[iy]);
	psFree (vo[iy]);
    }
    psFree (vi);
    psFree (vo);
	
    return true;
}

/* 
101, 104, 106, 109, 113, 
116, 117, 121, 122, 125, 
128, 130, 136, 137, 144, 
145, 146, 148, 149, 153, 
157, 160, 162, 164, 169, 
170, 173, 178, 180, 181, 
185, 193, 194, 196, 197, 
200, 202, 205, 208, 212, 
218, 221, 225
*/


/** @file  psImageBinning.c
 *
 *  @brief Functions to define the binning strategy and to perform image binning / unbinning
 *  (resampling).
 *
 *  @ingroup Image
 *
 *  @author Eugene Magnier, IfA
 *
 *  @version $Revision: 1.6 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-01-13 00:54:26 $
 *
 *  Copyright 2007 Institute for Astronomy, University of Hawaii
 */

#ifndef PS_IMAGE_BINNING_H
#define PS_IMAGE_BINNING_H

/// @addtogroup ImageOps Image Operations
/// @{

// description of the image binning relationship between a binned and an unbinned
// image. binning is defined for a specific input size and output image size.
typedef struct {
    int nXfine;                         // width of the hi-res image
    int nYfine;                         // height of the hi-res image
    int nXruff;                         // width of the lo-res image
    int nYruff;                         // height of the lo-res image
    int nXbin;                          // X binning factor (~ nXfine/nXruff)
    int nYbin;                          // Y binning factor (~ nYfine/nYruff)
    int nXoff;                          // offset in fine pixels to start of ruff 0 pixel (x_fine - nXoff) / nXbin = x_ruff
    int nYoff;                          // offset in fine pixels to start of ruff 0 pixel (x_fine - nXoff) / nXbin = x_ruff
    int nXskip;                         // offset in fine pixels from start of fine parent 0 pixel to ruff 0 pixel (nXskip = col0 - nXoff)
    int nYskip;                         // offset in fine pixels from start of fine parent 0 pixel to ruff 0 pixel (nYskip = row0 - nYoff)
} psImageBinning;

typedef enum {
    PS_IMAGE_BINNING_LEFT,
    PS_IMAGE_BINNING_CENTER,
    PS_IMAGE_BINNING_RIGHT,
} psImageBinningAlign;


psImageBinning *psImageBinningAlloc(void) PS_ATTR_MALLOC;
bool psMemCheckBinning(psPtr ptr);

void psImageBinningSetRuffSize(psImageBinning *binning, psImageBinningAlign align);
void psImageBinningSetFineSize(psImageBinning *binning, psImageBinningAlign align);
void psImageBinningSetScale(psImageBinning *binning, psImageBinningAlign align);
void psImageBinningSetSkip(psImageBinning *binning, const psImage *image);
void psImageBinningSetSkipByOffset(psImageBinning *binning, int col0, int row0);

psRegion psImageBinningSetFineRegion (psImageBinning *binning, psRegion ruffRegion);
psRegion psImageBinningSetRuffRegion (psImageBinning *binning, psRegion fineRegion);

double psImageBinningGetRuffX (const psImageBinning *binning, const double xFine);
double psImageBinningGetRuffY (const psImageBinning *binning, const double yFine);
double psImageBinningGetFineX (const psImageBinning *binning, const double xRuff);
double psImageBinningGetFineY (const psImageBinning *binning, const double yRuff);

/// @}
#endif // #ifndef PS_IMAGE_GEOM_MANIP_H

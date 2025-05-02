#ifndef PM_SUBTRACTION_VISUAL_H
#define PM_SUBTRACTION_VISUAL_H

bool pmSubtractionVisualClose(void);
bool pmSubtractionVisualPlotConvKernels(pmSubtractionKernels *kernels);
bool pmSubtractionVisualPlotStamps(pmSubtractionStampList *stamps, pmReadout *ro);
bool pmSubtractionVisualPlotLeastSquares(pmSubtractionStampList *stamps);
bool pmSubtractionVisualPlotLeastSquaresResid (const pmSubtractionStampList *stamps, psImage *matrixIn, int nUsed);
bool pmSubtractionVisualShowSubtraction(psImage *image, psImage *ref, psImage *sub);

bool pmSubtractionVisualShowFit(pmSubtractionStampList *stamps, pmSubtractionKernels *kernels);
bool pmSubtractionVisualShowFitInit(pmSubtractionStampList *stamps);
bool pmSubtractionVisualShowFitAddStamp(psKernel *target, psKernel *source, psKernel *convolution, double background, double norm, int index);
bool pmSubtractionVisualShowFitImage(double norm);

bool pmSubtractionVisualPlotFit(const pmSubtractionKernels *kernels);
bool pmSubtractionVisualShowKernels(pmSubtractionKernels *kernels);
bool pmSubtractionVisualShowBasis(pmSubtractionStampList *stamps);
bool pmSubtractionVisualPlotChisqAndMoments(psVector *fluxes, psVector *chisq, psVector *moments);

#endif

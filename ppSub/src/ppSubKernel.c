/** @file ppSubKernel.c
 *
 *  @brief
 *
 *  @ingroup ppSub
 *
 *  @author IfA
 *  @version $Revision: 1.3 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-02-06 01:37:17 $
 *  Copyright 2009 Institute for Astronomy, University of Hawaii
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <pslib.h>
#include <psmodules.h>

#define KERNEL_MOSAIC 2                 ///< Half-number of kernel instances in the mosaic image


int main(int argc, char *argv[])
{
    if (argc != 3) {
        fprintf(stderr, "Usage: %s INPUT_KERNEL OUTPUT_IMAGE\n\n", argv[0]);
        exit(PS_EXIT_SYS_ERROR);
    }

    (void) psTraceSetLevel("psModules.imcombine", 7);

    const char *inName = argv[1];       // Input file name
    const char *outName = argv[2];      // Output file name

    psFits *inFile = psFitsOpen(inName, "r"); // Input file
    if (!inFile) {
        psErrorStackPrint(stderr, "Unable to open input kernel %s", inName);
        exit(PS_EXIT_DATA_ERROR);
    }

    pmReadout *ro = pmReadoutAlloc(NULL); // Readout to hold kernel
    if (!pmReadoutReadSubtractionKernels(ro, inFile)) {
        psErrorStackPrint(stderr, "Unable to read input kernel");
        psFree(ro);
        psFitsClose(inFile);
        exit(PS_EXIT_DATA_ERROR);
    }
    psFitsClose(inFile);

    pmSubtractionKernels *kernels = psMetadataLookupPtr(NULL, ro->analysis, PM_SUBTRACTION_ANALYSIS_KERNEL);
    if (!psMemIncrRefCounter(kernels)) {
        psErrorStackPrint(stderr, "Unable to find input kernel");
        psFree(ro);
        exit(PS_EXIT_PROG_ERROR);
    }
    psFree(ro);

    pmSubtractionMode mode = kernels->mode; // Mode for subtraction
    int fullSize = 2 * kernels->size + 1 + 1;    // Full size of kernel
    int imageSize = (2 * KERNEL_MOSAIC + 1) * fullSize; // Size of image
    psImage *image = psImageAlloc((mode == PM_SUBTRACTION_MODE_DUAL ? 2 : 1) * imageSize - 1 +
                                  (mode == PM_SUBTRACTION_MODE_DUAL ? 4 : 0), imageSize - 1, PS_TYPE_F32);
    psImageInit(image, NAN);
    for (int j = -KERNEL_MOSAIC; j <= KERNEL_MOSAIC; j++) {
        for (int i = -KERNEL_MOSAIC; i <= KERNEL_MOSAIC; i++) {
            psImage *kernel = pmSubtractionKernelImage(kernels, (float)i / (float)KERNEL_MOSAIC,
                                                       (float)j / (float)KERNEL_MOSAIC,
                                                       false); // Image of the kernel
            if (!kernel) {
                psError(PS_ERR_UNKNOWN, false, "Unable to generate kernel image.");
                psFree(image);
                psFree(kernel);
                psFree(kernels);
                exit(PS_EXIT_SYS_ERROR);
            }

            if (psImageOverlaySection(image, kernel, (i + KERNEL_MOSAIC) * fullSize,
                                      (j + KERNEL_MOSAIC) * fullSize, "=") == 0) {
                psError(PS_ERR_UNKNOWN, false, "Unable to overlay kernel image.");
                psFree(kernel);
                psFree(image);
                psFree(kernels);
                exit(PS_EXIT_PROG_ERROR);
            }
            psFree(kernel);

            if (mode == PM_SUBTRACTION_MODE_DUAL) {
                kernel = pmSubtractionKernelImage(kernels, (float)i / (float)KERNEL_MOSAIC,
                                                  (float)j / (float)KERNEL_MOSAIC,
                                                  true); // Image of the kernel
                if (!kernel) {
                    psError(PS_ERR_UNKNOWN, false, "Unable to generate kernel image.");
                    psFree(image);
                    psFree(kernels);
                    exit(PS_EXIT_SYS_ERROR);
                }

                if (psImageOverlaySection(image, kernel,
                                          (2 * KERNEL_MOSAIC + 1 + i + KERNEL_MOSAIC) * fullSize + 4,
                                          (j + KERNEL_MOSAIC) * fullSize, "=") == 0) {
                    psError(PS_ERR_UNKNOWN, false, "Unable to overlay kernel image.");
                    psFree(kernel);
                    psFree(image);
                    psFree(kernels);
                    exit(PS_EXIT_PROG_ERROR);
                }
                psFree(kernel);
            }
        }
    }
    psFree(kernels);

    psFits *outFile = psFitsOpen(outName, "w"); // Output file
    if (!outFile) {
        psErrorStackPrint(stderr, "Unable to open output file %s", outName);
        psFree(image);
        exit(PS_EXIT_SYS_ERROR);
    }
    if (!psFitsWriteImage(outFile, NULL, image, 0, NULL)) {
        psErrorStackPrint(stderr, "Unable to write output file");
        psFree(image);
        psFitsClose(outFile);
        exit(PS_EXIT_SYS_ERROR);
    }
    psFitsClose(outFile);
    psFree(image);

    exit(PS_EXIT_SUCCESS);
}

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>

#include "pslib.h"
#include "psmodules.h"

int main(int argc, char *argv[])
{
    psLibInit(NULL);


//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ppImageConfig.c
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
    pmConfig *config = pmConfigRead(&argc, argv, "PPIMAGE");
    if (! config) {
        psErrorStackPrint(stderr, "Can't find site configuration!\n");
        exit(EXIT_FAILURE);
    }

    // Parse other command-line arguments
    psMetadataAddStr(config->arguments, PS_LIST_TAIL, "-key", 0, "exposure ID", "");
    psMetadataAddStr(config->arguments, PS_LIST_TAIL, "-bias", 0, "Name of the bias image", "");
    psMetadataAddStr(config->arguments, PS_LIST_TAIL, "-dark", 0, "Name of the dark image", "");
    psMetadataAddStr(config->arguments, PS_LIST_TAIL, "-flat", 0, "Name of the flat-field image", "");
    psMetadataAddStr(config->arguments, PS_LIST_TAIL, "-mask", 0, "Name of the mask image", "");
    psMetadataAddStr(config->arguments, PS_LIST_TAIL, "-fringe", 0, "Name of the fringe image", "");
    psMetadataAddS32(config->arguments, PS_LIST_TAIL, "-chip", 0, "Chip number to process (if positive)", -1);

    if (! psArgumentParse(config->arguments, &argc, argv) || argc != 3) {
        printf("\nPan-STARRS Phase 2 processing\n\n");
        printf("Usage: %s INPUT.fits OUTPUT.fits\n\n", argv[0]);
        psArgumentHelp(config->arguments);
        psFree(config->arguments);
        exit(EXIT_FAILURE);
    }

    // Add the input and output images (which remain on the command-line) to the arguments list
    psMetadataAddStr(config->arguments, PS_LIST_TAIL, "-input",  0, "Name of the input image", argv[1]);
    psMetadataAddStr(config->arguments, PS_LIST_TAIL, "-output", 0, "Name of the output image", argv[2]);

    // Define database handle, if used
#if 0
    config->database = pmConfigDB(config->site);
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ppImageParseCamera.c extract with some alterations
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

    const char *inName = psMetadataLookupStr(NULL, config->arguments, "-input");
    psLogMsg("ppImage", PS_LOG_INFO, "Opening input image: %s\n", inName);
    psFits *inFile = psFitsOpen(inName, "r"); // File handle for FITS file
    if (! inFile) {
        // There's no point in continuing if we can't open the input
        psErrorStackPrint(stderr, "Can't open input image: %s\n", inName);
        exit(EXIT_FAILURE);
    }
    psMetadata *phu = psFitsReadHeader(NULL, inFile); // FITS primary header

    psMetadata *cameraFormat = pmConfigCameraFormatFromHeader(NULL, NULL, config, phu, true);
    if (! config->camera) {
        cameraFormat = pmConfigCameraFormatFromHeader(NULL, NULL, config, phu, true);
        if (! config->camera) {
             // There's no point in continuing if we can't recognise what we've got
            psErrorStackPrint(stderr, "Can't find camera configuration!\n");
            exit(EXIT_FAILURE);
        }
    }
    // Determine the correct recipe to use
    if (! config->recipes && !pmConfigReadRecipes(config, PM_RECIPE_SOURCE_CAMERA | PM_RECIPE_SOURCE_CL)) {
        // There's no point in continuing if we can't work out what recipes to use
        psErrorStackPrint(stderr, "Can't find recipe configuration!\n");
        exit(EXIT_FAILURE);
    }

#if 1
    const char *outName = psMetadataLookupStr(NULL, config->arguments, "-output");
    psLogMsg("ppImage", PS_LOG_INFO, "Opening output image: %s\n", outName);
    psFits *outFile = psFitsOpen(outName, "w");
    if (!outFile) {
        // There's no point in continuing if we can't open the output
        psErrorStackPrint(stderr, "Can't open output image: %s\n", outName);
        exit(EXIT_FAILURE);
    }
#endif

    // Construct camera in preparation for reading
    pmFPA *fpa = pmFPAConstruct(config->camera, config->cameraName);
    pmFPAview *view = pmFPAAddSourceFromHeader(fpa, phu, cameraFormat);
    printf("View chip: %d\n", view->chip);
    printf("View cell: %d\n", view->cell);

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// The action happens here
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

#if 0
    pmFPAPrint(stdout, fpa, false, false);
    exit(0);
#endif

#if 0
    // A mosaic
    psMetadata *mosaicCamera = psMetadataConfigRead(NULL, NULL, "/home/mithrandir/price/ipp/config/mcshort_mosaic/camera.config", true);
    pmFPA *mosaicFPA = pmFPAConstruct(mosaicCamera);
    psMetadata *mosaicFormat = psMetadataConfigRead(NULL, NULL, "/home/mithrandir/price/ipp/config/mcshort_mosaic/format_mosaic.config", true);
    pmFPAview *mosaicView = pmFPAviewAlloc(0);
    pmFPAAddSourceFromView(mosaicFPA, mosaicView, mosaicFormat);
    psFree(mosaicView);
#endif

    // Read the FPA
    pmFPARead(fpa, inFile, NULL);

#if 1
    psArray *chips = fpa->chips;
    pmFPAWrite(fpa, outFile, NULL, true, false);
    for (int i = 0; i < chips->n; i++) {
        pmChip *chip = chips->data[i];
#if 1
        //pmChipRead(chip, inFile, NULL);
        pmChipWrite(chip, outFile, NULL, true, false);
        psArray *cells = chip->cells;
        for (int j = 0; j < cells->n; j++) {
            pmCell *cell = cells->data[j];
            pmCellWrite(cell, outFile, NULL, true);
#if 0 // Read bit by bit
            pmReadout *readout = pmReadoutAlloc(cell);
            for (int z = 0; pmReadoutReadNext(readout, inFile, z, 512); z++) {
                do {
                    printf("Chip %d, Cell %d, Plane %d, row0 = %d\n", i, j, z, readout->row0);
                    pmReadoutWriteNext(readout, outFile, z);
                    //pmFPAPrint(stdout, fpa, false, true);
                } while (pmReadoutReadNext(readout, inFile, z, 512));
            }
            psFree(readout);
#else // Read the whole cell at once
            //            pmCellRead(cell, inFile, NULL);
            //            pmCellWrite(cell, outFile, NULL, false);
            //            pmCellFreeData(cell);
#endif
        }
        pmChipWrite(chip, outFile, NULL, false, false);
#endif

#if 0
        pmChipMosaic(mosaicFPA->chips->data[i], chip);
        pmChipWrite(mosaicFPA->chips->data[i], outFile, NULL, true, true);
        pmChipFreeData(chip);
        pmChipFreeData(mosaicFPA->chips->data[i]);
#endif
    }
    pmFPAWrite(fpa, outFile, NULL, false, false);
#endif

#if 0
    pmFPAMosaic(mosaicFPA, fpa);
    pmFPAWrite(mosaicFPA, outFile, NULL, true, true);
    //pmFPAPrint(stdout, mosaicFPA, true, true);
    psFree(mosaicCamera);
    psFree(mosaicFormat);
    psFree(mosaicView);
    //psFree(mosaicFPA);
#endif

#if 1
    pmFPAPrint(stdout, fpa, false, false);
#endif

#if 1
    psFitsClose(outFile);
#endif

    psFree(view);
    psFree(phu);
    psFree(fpa);
    psFree(config);
    psFitsClose(inFile);
    psFree(cameraFormat);

    pmConceptsDone();
    pmConfigDone();
    psLibFinalize();

    // Pau.
}

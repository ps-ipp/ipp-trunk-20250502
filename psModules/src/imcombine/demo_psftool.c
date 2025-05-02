// This is a very quick and dirty test program for pmPSFEnvelope
//
// gcc -g demo_psftool.c -o demo_psftool `psmodules-config --cflags --libs` --std=gnu99 -Wall
//
// ./demo_psftool test MOPS.skycell.0198767.wrp*.psf
//

#include <stdio.h>
#include <pslib.h>
#include <psmodules.h>

// Add a single filename to the arguments as an array, so that it can be used with pmFPAfileBindFromArgs, etc
static void fileList(const char *file, // The symbolic name for the file
                     const char *name, // The name of the file
                     const char *comment, // Description of the file
                     pmConfig *config // Configuration
    )
{
    psArray *files = psArrayAlloc(1); // Array with file names
    files->data[0] = psStringCopy(name);
    psMetadataAddArray(config->arguments, PS_LIST_TAIL, file, 0, comment, files);
    psFree(files);
    return;
}



int main(int argc, char *argv[])
{
    psLibInit(NULL);
    pmConfig *config = pmConfigRead(&argc, argv, "PSF");
    if (!config) {
        psErrorStackPrint(stderr, "Error reading configuration.");
        exit(PS_EXIT_CONFIG_ERROR);
    }

    psTraceSetLevel("psModules.imcombine", 5);
    psTraceSetLevel("psModules.objects", 0);
    psTraceSetLevel("psLib.math", 0);

    pmModelClassInit();

    psMetadataAddStr(config->arguments, PS_LIST_TAIL, "OUTPUT", 0, "Name of the output", argv[1]);

    psArray *files = psArrayAlloc(argc - 2);
    for (int i = 2; i < argc; i++) {
        psString name = NULL;           // Name of file list
        psStringAppend(&name, "INPUT_%d", i);

        fileList(name, argv[i], "Input PSF", config);

        pmFPAfile *file = pmFPAfileDefineFromArgs(NULL, config, "PSPHOT.PSF.LOAD", name);
        psFree(name);
        if (!file) {
            psErrorStackPrint(stderr, "Can't define PSF file from %s --> %s", name, argv[i]);
            psFree(files);
            psFree(config);
            exit(PS_EXIT_SYS_ERROR);
        }

        files->data[i - 2] = psMemIncrRefCounter(file);
    }

    pmFPAfile *outFile = pmFPAfileDefineOutput(config, NULL, "PSPHOT.PSF.SAVE");
    if (!outFile) {
        psErrorStackPrint(stderr, "Can't define output PSF file");
        psFree(files);
        psFree(config);
        exit(PS_EXIT_SYS_ERROR);
    }
    outFile->save = true;

    // XXX This is a bit dodgy; should be more rigorous for a real system
    {
        pmFPAview *phuView = pmFPAviewAlloc(0);
        phuView->chip = 0;
        if (!pmFPAAddSourceFromView(outFile->fpa, "Envelope PSF", phuView, outFile->format)) {
            psError(PS_ERR_UNKNOWN, false, "Unable to add PHU to output.");
            psFree(phuView);
            return false;
        }
        psFree(phuView);
    }

    pmFPAview *view = pmFPAviewAlloc(0);

    if (!pmFPAfileIOChecks(config, view, PM_FPA_BEFORE)) {
        psErrorStackPrint(stderr, "Problem in I/O");
        psFree(view);
        psFree(files);
        psFree(config);
        exit(PS_EXIT_SYS_ERROR);
    }

    pmChip *chip;
    while ((chip = pmFPAviewNextChip(view, outFile->fpa, 1)) != NULL) {
        if (!pmFPAfileIOChecks(config, view, PM_FPA_BEFORE)) {
            psErrorStackPrint(stderr, "Problem in I/O");
            psFree(view);
            psFree(files);
            psFree(config);
            exit(PS_EXIT_SYS_ERROR);
        }

#if 0
        pmFPAfileActivate(config->files, "PSPHOT.PSF.SAVE", false);
        pmCell *cell;
        while ((cell = pmFPAviewNextCell(view, outFile->fpa, 1)) != NULL) {
            if (!pmFPAfileIOChecks(config, view, PM_FPA_BEFORE)) {
                psErrorStackPrint(stderr, "Problem in I/O");
                psFree(view);
                psFree(files);
                psFree(config);
                exit(PS_EXIT_SYS_ERROR);
            }
        }
#endif

        psArray *inputs = psArrayAlloc(files->n);
        int numCols = 4501, numRows = 4751;
        for (int i = 0; i < files->n; i++) {
            pmFPAfile *file = files->data[i];
            pmChip *chip = pmFPAviewThisChip(view, file->fpa);

            pmPSF *psf = psMetadataLookupPtr(NULL, chip->analysis, "PSPHOT.PSF");
            if (!psf) {
                psErrorStackPrint(stderr, "Can't find PSF in file %d", i);
                psFree(inputs);
                psFree(files);
                psFree(config);
                exit(PS_EXIT_PROG_ERROR);
            }

#if 0
            pmHDU *hdu = pmHDUGetLowest(file->fpa, chip, NULL);
            int imaxis1 = psMetadataLookupS32(NULL, hdu->header, "IMAXIS1");
            int imaxis2 = psMetadataLookupS32(NULL, hdu->header, "IMAXIS2");
            if (imaxis1 == 0 || imaxis2 == 0) {
                psErrorStackPrint(stderr, "Size of image %d can't be determined.", i);
                psFree(inputs);
                psFree(files);
                psFree(config);
                exit(PS_EXIT_SYS_ERROR);
            }
            if (numCols == 0 && numRows == 0) {
                numCols = imaxis1;
                numRows = imaxis2;
            } else if (imaxis1 != numCols || imaxis2 != numRows) {
                psErrorStackPrint(stderr, "Image %d differs in size: %dx%d vs %dx%d",
                                  i, imaxis1, imaxis2, numCols, numRows);
                psFree(inputs);
                psFree(files);
                psFree(config);
                exit(PS_EXIT_SYS_ERROR);
            }
#endif

            inputs->data[i] = psMemIncrRefCounter(psf);
        }

        pmPSF *psf = pmPSFEnvelope(numCols, numRows, inputs, 5, 20, "PS_MODEL_RGAUSS", 3, 3);
        psFree(inputs);
        if (!psf) {
            psErrorStackPrint(stderr, "Can't generate envelope PSF.");
            psFree(config);
            exit(PS_EXIT_SYS_ERROR);
        }

        psMetadataAddPtr(chip->analysis, PS_LIST_TAIL, "PSPHOT.PSF", PS_DATA_UNKNOWN, "Envelope PSF", psf);
        psFree(psf);
        chip->data_exists = true;

        if (!pmFPAfileIOChecks(config, view, PM_FPA_AFTER)) {
            psErrorStackPrint(stderr, "Problem in I/O");
            psFree(view);
            psFree(files);
            psFree(config);
            exit(PS_EXIT_SYS_ERROR);
            return false;
        }
    }

    if (!pmFPAfileIOChecks(config, view, PM_FPA_AFTER)) {
        psErrorStackPrint(stderr, "Problem in I/O");
        psFree(view);
        psFree(files);
        psFree(config);
        exit(PS_EXIT_SYS_ERROR);
    }

    psFree(config);

    exit(PS_EXIT_SUCCESS);
}

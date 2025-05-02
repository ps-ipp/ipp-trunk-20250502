#include <stdio.h>
#include <assert.h>
#include <pslib.h>

static bool subtract(psFits *inFile, psFits *outFile, const char *extname, psMetadata *header)
{
    assert(inFile);
    assert(outFile);
    assert(extname);
    assert(header);

    int naxis = psMetadataLookupS32(NULL, header, "NAXIS"); // NAXIS from the header
    if (naxis == 0) {
        // No image attached
        return psFitsWriteBlank(outFile, header, extname);
    }

    if (naxis != 3) {
        psWarning("Extension %s has NAXIS == %d --- unable to subtract pedestal.\n", extname, naxis);
        return false;
    }

    int naxis3 = psMetadataLookupS32(NULL, header, "NAXIS3"); // NAXIS3 from the header
    if (naxis3 != 2) {
        psWarning("Extension %s has NAXIS3 == %d --- unable to subtract pedestal.\n", extname, naxis3);
        return false;
    }

    psFitsMoveExtName(inFile, extname);
    psArray *cube = psFitsReadImageCube(inFile, psRegionSet(0, 0, 0, 0)); // Image cube
    assert(cube);
    assert(cube->n == 2);               // We checked NAXIS3 earlier

    psImage *image = cube->data[0];     // The image data
    psImage *pedestal = cube->data[1];  // The pedestal data

    image = (psImage*)psBinaryOp(image, image, "-", pedestal);

    bool status = psFitsWriteImage(outFile, header, image, 0, extname); // Status of write
    psFree(cube);

    return status;
}

int main(int argc, char *argv[])
{
    if (argc != 3) {
        printf("Subtract pedestal from FITS images.\n\n"
               "Usage: %s IN.fits OUT.fits\n\n", argv[0]);
        exit(argc == 1 ? PS_EXIT_SUCCESS : PS_EXIT_UNKNOWN_ERROR);
    }

    const char *inName = argv[1];       // Input file name
    psFits *inFile = psFitsOpen(inName, "r"); // Input file
    if (!inFile) {
        psErrorStackPrint(stderr, "Unable to open input file %s", inName);
        exit(PS_EXIT_DATA_ERROR);
    }

    const char *outName = argv[2];      // Output file name
    psFits *outFile = psFitsOpen(outName, "w"); // Output file
    if (!outFile) {
        psErrorStackPrint(stderr, "Unable to open output file %s", outName);
        exit(PS_EXIT_DATA_ERROR);
    }

    psMetadata *headers = psFitsReadHeaderSet(NULL, inFile); // Headers in the input

    psMetadata *phu = psMetadataLookupMetadata(NULL, headers, "PHU"); // The PHU
    subtract(inFile, outFile, "PHU", phu);
    psMetadataRemoveKey(headers, "PHU");

    psMetadataIterator *iter = psMetadataIteratorAlloc(headers, PS_LIST_HEAD, NULL); // Iterator for headers
    psMetadataItem *item;               // Item from iteration
    while ((item = psMetadataGetAndIncrement(iter))) {
        assert(item->type == PS_DATA_METADATA);
        subtract(inFile, outFile, item->name, item->data.V);
    }
    psFree(iter);
    psFree(headers);

    psFitsClose(inFile);
    psFitsClose(outFile);

    return PS_EXIT_SUCCESS;
}

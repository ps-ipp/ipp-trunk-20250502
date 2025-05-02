#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <strings.h>
#include <string.h>
#include <pslib.h>

#include "pmConfig.h"
#include "pmConfigMask.h"
#include "pmHDU.h"
#include "pmFPA.h"
#include "pmFPALevel.h"
#include "pmFPAview.h"
#include "pmDetrendDB.h"
#include "pmFPAfile.h"
#include "pmHDUUtils.h"
#include "pmHDUGenerate.h"
#include "pmConceptsWrite.h"

#include "pmFPAWrite.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Definitions
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Specify what to read
typedef enum {
    FPA_WRITE_TYPE_IMAGE,               // Write image
    FPA_WRITE_TYPE_MASK,                // Write mask
    FPA_WRITE_TYPE_VARIANCE             // Write variance map
} fpaWriteType;

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// File-static (private) functions
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Return the appropriate image array for the given type
static psArray **appropriateImageArray(pmHDU *hdu, // HDU containing the image arrays
                                       fpaWriteType type // Type to write
                                      )
{
    switch (type) {
    case FPA_WRITE_TYPE_IMAGE:
        return &hdu->images;
    case FPA_WRITE_TYPE_MASK:
        return &hdu->masks;
    case FPA_WRITE_TYPE_VARIANCE:
        return &hdu->variances;
    default:
        psAbort("Unknown write type: %x\n", type);
    }
    return NULL;
}

// Run the appropriate HDU write function
static bool appropriateWriteFunc(pmHDU *hdu, // HDU to write
                                 psFits *fits, // FITS file to which to write
                                 const pmConfig *config, // Configuration
                                 fpaWriteType type // Type to write
                                )
{
    switch (type) {
    case FPA_WRITE_TYPE_IMAGE:
        return pmHDUWrite(hdu, fits, config);
    case FPA_WRITE_TYPE_MASK:
        return pmHDUWriteMask(hdu, fits, config);
    case FPA_WRITE_TYPE_VARIANCE:
        return pmHDUWriteVariance(hdu, fits, config);
    default:
        psAbort("Unknown write type: %x\n", type);
    }
    return false;
}

// Indicate whether a covariance matrix is defined
static bool readoutSearchCovariances(pmReadout *ro)
{
    return ro->covariance ? true : false;
}

// Search for a covariance matrix
#define SEARCH_COVARIANCES(NAME, PARENT, CHILD, CHILDREN, TESTFUNC) \
static bool NAME(PARENT *parent) \
{ \
    if (!parent || !parent->CHILDREN) { \
        return false; \
    } \
    psArray *children = parent->CHILDREN; /* Array of children */ \
    for (int i = 0; i < children->n; i++) { \
        CHILD *child = children->data[i]; /* Child of interest */ \
        if (child && TESTFUNC(child)) { \
            return true; \
        } \
    } \
    return false; \
}

SEARCH_COVARIANCES(cellSearchCovariances, pmCell, pmReadout, readouts, readoutSearchCovariances);
SEARCH_COVARIANCES(chipSearchCovariances, pmChip, pmCell,    cells,    cellSearchCovariances);
SEARCH_COVARIANCES(fpaSearchCovariances,  pmFPA,  pmChip,    chips,    chipSearchCovariances);

// Some type-specific additions to the header
static bool writeUpdateHeader(pmFPA *fpa, // FPA of interest
                              pmChip *chip, // Chip of interest, or NULL
                              pmCell *cell, // Cell of interest, or NULL
                              fpaWriteType type, // Type to write
                              pmConfig *config // Configuration
                              )
{
    switch (type) {
      case FPA_WRITE_TYPE_MASK: {
          pmHDU *phu = pmHDUGetHighest(fpa, chip, cell); // Primary header
          if (!pmConfigMaskWriteHeader(config, phu->header)) {
              psError(PS_ERR_UNKNOWN, false, "Unable to set the mask names in the PHU header");
              return false;
          }
          break;
      }
      case FPA_WRITE_TYPE_VARIANCE: {
          bool covar = false;           // Are covariances present?
          if ((cell && cellSearchCovariances(cell)) ||
              (!cell && ((chip && chipSearchCovariances(chip)) ||
                         (!chip && fpa && fpaSearchCovariances(fpa))))) {
              covar = true;
          }

          pmHDU *hdu = pmHDUGetLowest(fpa, chip, cell); // Header being written
          psMetadataAddBool(hdu->header, PS_LIST_TAIL, PM_HDU_COVARIANCE_KEYWORD, PS_META_REPLACE,
                            "Is a covariance matrix present?", covar);
          break;
      }
      default:
        break;
    }

    return true;
}


// Write a cell image/mask/variance
static bool cellWrite(pmCell *cell,     // Cell to write
                      psFits *fits,     // FITS file to which to write
                      pmConfig *config, // Configuration
                      bool blank,       // Write a blank PHU?
                      fpaWriteType type // Type to write
                     )
{
    assert(cell);
    assert(fits);

    psTrace ("pmFPAWrite", 5, "writing to Cell (%d)\n", blank);

    pmHDU *hdu = cell->hdu;             // The HDU
    if (!hdu || !cell->data_exists) {
        return true;                    // We wrote every HDU that exists
    }

    psArray **imageArray = appropriateImageArray(hdu, type); // Array of images in the HDU

    // XXX detect missing variance & mask images...

    // Generate the HDU if needed --- this is required after a pmFPACopy, or similar, which does not
    // generate the HDU, but only copies the structure.
    if (!blank && !hdu->blankPHU && !*imageArray) {
        if (!pmHDUGenerateForCell(cell)) {
            psError(PS_ERR_UNKNOWN, false, "Unable to generate HDU for cell --- likely programming error.");
            return false;
        }
        if (!*imageArray) {
            if (type == FPA_WRITE_TYPE_IMAGE) {
                psError(PS_ERR_UNKNOWN, false, "Expected to write an image, but it is missing...programming error?.");
                return false;
            }
            if (type == FPA_WRITE_TYPE_MASK) {
                psWarning("No mask image for this cell; skipping");
            }
            if (type == FPA_WRITE_TYPE_VARIANCE) {
                psWarning("No variance image for this cell; skipping");
            }
            return true;
        }
    }

    // We only write out a blank PHU if it's specifically requested.
    bool writeBlank = blank && hdu->blankPHU && !*imageArray; // Write a blank PHU?
    bool writeImage = !blank && !hdu->blankPHU && *imageArray; // Write an image?

    if (writeBlank || writeImage) {
        if (!pmConceptsWriteCell(cell, true, config)) {
            psError(PS_ERR_IO, false, "Unable to write concepts for cell.");
            return false;
        }
        if (!writeUpdateHeader(NULL, NULL, cell, type, config)) {
            psError(PS_ERR_UNKNOWN, false, "Unable to update header for writing");
            return false;
        }
        if (!appropriateWriteFunc(hdu, fits, config, type)) {
            psError(PS_ERR_IO, false, "Unable to write HDU for cell.\n");
            return false;
        }
    }
    // No lower levels to which to recurse

    return true;
}

// Write a chip image/mask/variance
static bool chipWrite(pmChip *chip,     // Chip to write
                      psFits *fits,     // FITS file to which to write
                      pmConfig *config, // Configuration
                      bool blank,       // Write a blank PHU?
                      bool recurse,     // Recurse to lower levels?
                      fpaWriteType type // Type to write
                     )
{
    assert(chip);
    assert(fits);

    pmHDU *hdu = chip->hdu;             // The HDU

    psTrace ("pmFPAWrite", 5, "writing to Chip (%d, %d)\n", blank, recurse);

    // If we have data at this level, try to write it out
    if (hdu && chip->data_exists) {
        psArray **imageArray = appropriateImageArray(hdu, type); // Array of images in HDU

        // Generate the HDU if needed --- this is required after a pmFPACopy, or similar, which does not
        // generate the HDU, but only copies the structure.
        if (!blank && !hdu->blankPHU && !*imageArray && (!pmHDUGenerateForChip(chip) || !*imageArray)) {
            psError(PS_ERR_UNKNOWN, false,
                    "Unable to generate HDU for chip --- likely programming error.\n");
            return false;
        }

        // We only write out a blank PHU if it's specifically requested.
        bool writeBlank = blank && hdu->blankPHU && !*imageArray; // Write a blank HDU?
        bool writeImage = !blank && !hdu->blankPHU && *imageArray; // Write an image?

        if (writeBlank || writeImage) {
            if (!pmConceptsWriteChip(chip, true, true, config)) {
                psError(PS_ERR_IO, false, "Unable to write concepts for chip.\n");
                return false;
            }

            if (!writeUpdateHeader(NULL, chip, NULL, type, config)) {
                psError(PS_ERR_UNKNOWN, false, "Unable to update header for writing");
                return false;
            }

            if (!appropriateWriteFunc(hdu, fits, config, type)) {
                psError(PS_ERR_IO, false, "Unable to write HDU for chip.\n");
                return false;
            }
        }
    }

    // Recurse to lower level if specifically requested.
    // XXX recursion implies blank == false (must be called on correct level?)
    if (recurse) {
        psArray *cells = chip->cells;       // Array of cells
        for (int i = 0; i < cells->n; i++) {
            pmCell *cell = cells->data[i];  // The cell of interest
            if (!cellWrite(cell, fits, config, false, type)) {
                psError(PS_ERR_IO, false, "Unable to write Chip.\n");
                return false;
            }
        }
    }

    return true;
}


// Write an FPA image/mask/variance
static bool fpaWrite(pmFPA *fpa,        // FPA to write
                     psFits *fits,      // FITS file to which to write
                     pmConfig *config,  // Configuration
                     bool blank,        // Write a blank PHU?
                     bool recurse,      // Recurse to lower levels?
                     fpaWriteType type  // Type to write
                    )
{
    assert(fpa);
    assert(fits);

    pmHDU *hdu = fpa->hdu;              // The HDU

    psTrace ("pmFPAWrite", 5, "writing to FPA (%d, %d)\n", blank, recurse);

    // If we have data at this level, try to write it out
    if (hdu) {
        psArray **imageArray = appropriateImageArray(hdu, type); // Array of images in HDU

        // Generate the HDU if needed --- this is required after a pmFPACopy, or similar, which does not
        // generate the HDU, but only copies the structure.
        if (!blank && !hdu->blankPHU && !*imageArray && (!pmHDUGenerateForFPA(fpa) || !*imageArray)) {
            psError(PS_ERR_UNKNOWN, false,
                    "Unable to generate HDU for FPA --- likely programming error.\n");
            return false;
        }

        // We only write out a blank PHU if it's specifically requested.
        bool writeBlank = blank && hdu->blankPHU && !*imageArray; // Write a blank PHU?
        bool writeImage = !blank && !hdu->blankPHU && *imageArray; // Write an image?

        if (writeBlank || writeImage) {
            if (!pmConceptsWriteFPA(fpa, true, config)) {
                psError(PS_ERR_IO, false, "Unable to write concepts for FPA.\n");
                return false;
            }
            if (!writeUpdateHeader(fpa, NULL, NULL, type, config)) {
                psError(PS_ERR_UNKNOWN, false, "Unable to update header for writing");
                return false;
            }
            if (!appropriateWriteFunc(hdu, fits, config, type))  {
                psError(PS_ERR_IO, false, "Unable to write HDU for FPA.\n");
                return false;
            }
        }
    }

    // Recurse to lower levels if requested
    if (recurse) {
        psArray *chips = fpa->chips;        // Array of chips
        for (int i = 0; i < chips->n; i++) {
            pmChip *chip = chips->data[i];  // The chip of interest
            if (!chipWrite(chip, fits, config, false, true, type)) {
                psError(PS_ERR_IO, false, "Unable to write FPA.\n");
                return false;
            }
        }
    }

    return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Public functions
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Update the CHIP.NAME and CELL.NAME in the FITS header, if required
bool pmFPAUpdateNames(pmFPA *fpa, pmChip *chip, pmCell *cell, psS64 imageId, psS64 sourceId)
{
    pmHDU *hduHigh = pmHDUGetHighest(fpa, chip, cell); // Highest HDU, i.e., the PHU
    if (!hduHigh) {
        psError(PS_ERR_UNEXPECTED_NULL, false, "Unable to find PHU.\n");
        return false;
    }
    if (!hduHigh->header) {
        hduHigh->header = psMetadataAlloc();
    }
    if (!pmHDUWriteIdentifiers(hduHigh, imageId, sourceId)) {
        psError(PS_ERR_UNKNOWN, false, "Unable to write identifiers to header.");
        return false;
    }

    pmHDU *hduLow = pmHDUGetLowest(fpa, chip, cell); // Lowest HDU, i.e., the extension
    if (!hduLow) {
        psError(PS_ERR_UNEXPECTED_NULL, false, "Unable to find HDU.\n");
        return false;
    }
    if (!hduLow->header) {
        hduLow->header = psMetadataAlloc();
    }
    if (hduLow != hduHigh && !pmHDUWriteIdentifiers(hduLow, imageId, sourceId)) {
        psError(PS_ERR_UNKNOWN, false, "Unable to write identifiers to header.");
        return false;
    }

    bool mdok;                          // Status of MD lookup
    psMetadata *fileData = psMetadataLookupMetadata(&mdok, hduHigh->format, "FILE"); // File information
    if (!mdok || !fileData) {
        psError(PS_ERR_UNKNOWN, true, "Unable to find FILE information in camera format.\n");
        return false;
    }

    if (fpa && !fpa->hdu && (chip || cell)) {
        const char *rule = psMetadataLookupStr(NULL, fileData, "CONTENT.RULE"); // How to define the CONTENT
        if (!rule) {
            psError(PS_ERR_UNKNOWN, false, "Unable to find CONTENT.RULE in FILE in camera format.");
            return false;
        }

        pmFPAview *view = pmFPAviewGenerate(fpa, chip, cell, NULL); // View for fpa, chip, cell
        psString content = pmFPANameFromRule(rule, fpa, view); // Content of this file, specified by the rule
        psFree(view);

        const char *contentKey = psMetadataLookupStr(NULL, fileData, "CONTENT"); // The CONTENT header keyword
        if (!contentKey) {
            psError(PS_ERR_UNKNOWN, false, "Unable to find CONTENT in FILE in the camera format.");
            psFree(content);
            return false;
        }

        psMetadataAddStr(hduHigh->header, PS_LIST_TAIL, contentKey, PS_META_REPLACE,
                         "Content of file", content);
        psFree(content);                // Drop reference
    }

    return true;
}

bool pmReadoutWriteNext(pmReadout *readout, psFits *fits, int z)
{
    PS_ASSERT_PTR_NON_NULL(readout, false);
    PS_ASSERT_PTR_NON_NULL(fits, false);

    pmHDU *hdu = pmHDUFromReadout(readout); // The HDU to which to write
    if (!hdu) {
        psError(PS_ERR_IO, false, "Unable to find HDU for readout.\n");
        return false;
    }
    psMetadata *header = hdu->header;   // The FITS header
    if (!header) {
        psError(PS_ERR_IO, true, "No FITS header available in the HDU.\n");
        return false;
    }

    // We have to rely to a great extent on the FITS header, because otherwise we simply don't know how many
    // image planes there are (NAXIS3) or the size of the original image (NAXIS1, NAXIS2).
    bool mdok = true;                   // Status of MD lookup
    int naxis1 = psMetadataLookupS32(&mdok, header, "NAXIS1"); // Number of columns
    if (!mdok || naxis1 <= 0) {
        psError(PS_ERR_IO, true, "Can't find NAXIS1 in header.\n");
        return false;
    }
    int naxis2 = psMetadataLookupS32(&mdok, header, "NAXIS2"); // Number of rows
    if (!mdok || naxis2 <= 0) {
        psError(PS_ERR_IO, true, "Can't find NAXIS2 in header.\n");
        return false;
    }
    int naxis3 = psMetadataLookupS32(&mdok, header, "NAXIS3"); // Number of image planes
    if (!mdok || naxis3 <= 0) {
        naxis3 = 1;
    }
    if (z >= naxis3) {
        psError(PS_ERR_IO, true, "Specified a plane number (%d) greater than NAXIS3 allows.\n", z);
        return false;
    }

    if (!hdu->images) {
        psError(PS_ERR_IO, true, "No images allocated in HDU.\n");
        return false;
    }
    psImage *image = readout->image;    // The image from the HDU to write
    //    psImage *mask = readout->mask;        // Corresponding mask image
    if (readout->row0 == 0 && readout->col0 == 0 && z == 0) {
        // Then we can assume that nothing has been written to the FITS file for now
        if (naxis1 == image->numCols && naxis2 == image->numRows) {
            // We can write the whole lot at once
            return psFitsWriteImage(fits, header, image, z, hdu->extname);
        }
        // Create a dummy image so we can write something larger than we actually have
        psImage *dummy = psImageAlloc(naxis1, naxis2, image->type.type); // Dummy image
        psImageInit(dummy, 0);
        psImageOverlaySection(dummy, image, 0, 0, "=");
        bool result = psFitsWriteImage(fits, header, dummy, z, hdu->extname);
        psFree(dummy);
        return result;
    }

    // We can simply update an existing HDU
    if (hdu->blankPHU && !psFitsMoveExtNum(fits, 0, false)) {
        psError(PS_ERR_IO, false, "Unable to move to PHU\n");
        return false;
    }
    return psFitsUpdateImage(fits, image, readout->col0, readout->row0, z);
}


bool pmCellWrite(pmCell *cell, psFits *fits, pmConfig *config, bool blank)
{
    PS_ASSERT_PTR_NON_NULL(cell, false);
    PS_ASSERT_PTR_NON_NULL(fits, false);
    return cellWrite(cell, fits, config, blank, FPA_WRITE_TYPE_IMAGE);
}

bool pmChipWrite(pmChip *chip, psFits *fits, pmConfig *config, bool blank, bool recurse)
{
    PS_ASSERT_PTR_NON_NULL(chip, false);
    PS_ASSERT_PTR_NON_NULL(fits, false);
    return chipWrite(chip, fits, config, blank, recurse, FPA_WRITE_TYPE_IMAGE);
}

bool pmFPAWrite(pmFPA *fpa, psFits *fits, pmConfig *config, bool blank, bool recurse)
{
    PS_ASSERT_PTR_NON_NULL(fpa, false);
    PS_ASSERT_PTR_NON_NULL(fits, false);
    return fpaWrite(fpa, fits, config, blank, recurse, FPA_WRITE_TYPE_IMAGE);
}


bool pmCellWriteMask(pmCell *cell, psFits *fits, pmConfig *config, bool blank)
{
    PS_ASSERT_PTR_NON_NULL(cell, false);
    PS_ASSERT_PTR_NON_NULL(fits, false);
    return cellWrite(cell, fits, config, blank, FPA_WRITE_TYPE_MASK);
}

bool pmChipWriteMask(pmChip *chip, psFits *fits, pmConfig *config, bool blank, bool recurse)
{
    PS_ASSERT_PTR_NON_NULL(chip, false);
    PS_ASSERT_PTR_NON_NULL(fits, false);
    return chipWrite(chip, fits, config, blank, recurse, FPA_WRITE_TYPE_MASK);
}

bool pmFPAWriteMask(pmFPA *fpa, psFits *fits, pmConfig *config, bool blank, bool recurse)
{
    PS_ASSERT_PTR_NON_NULL(fpa, false);
    PS_ASSERT_PTR_NON_NULL(fits, false);
    return fpaWrite(fpa, fits, config, blank, recurse, FPA_WRITE_TYPE_MASK);
}


bool pmCellWriteVariance(pmCell *cell, psFits *fits, pmConfig *config, bool blank)
{
    PS_ASSERT_PTR_NON_NULL(cell, false);
    PS_ASSERT_PTR_NON_NULL(fits, false);
    if (!cellWrite(cell, fits, config, blank, FPA_WRITE_TYPE_VARIANCE)) {
        return false;
    }
    if (!pmCellWriteCovariance(fits, cell)) {
        return false;
    }
    return true;
}

bool pmChipWriteVariance(pmChip *chip, psFits *fits, pmConfig *config, bool blank, bool recurse)
{
    PS_ASSERT_PTR_NON_NULL(chip, false);
    PS_ASSERT_PTR_NON_NULL(fits, false);
    if (!chipWrite(chip, fits, config, blank, recurse, FPA_WRITE_TYPE_VARIANCE)) {
        return false;
    }
    if (!pmChipWriteCovariance(fits, chip)) {
        return false;
    }
    return true;
}

bool pmFPAWriteVariance(pmFPA *fpa, psFits *fits, pmConfig *config, bool blank, bool recurse)
{
    PS_ASSERT_PTR_NON_NULL(fpa, false);
    PS_ASSERT_PTR_NON_NULL(fits, false);
    if (!fpaWrite(fpa, fits, config, blank, recurse, FPA_WRITE_TYPE_VARIANCE)) {
        return false;
    }
    if (!pmFPAWriteCovariance(fits, fpa)) {
        return false;
    }
    return true;
}


int pmCellWriteTable(psFits *fits, const pmCell *cell, const char *name)
{
    PS_ASSERT_PTR_NON_NULL(cell, 0);
    PS_ASSERT_PTR_NON_NULL(fits, 0);
    PS_ASSERT_STRING_NON_EMPTY(name, 0);

    const char *chipName = psMetadataLookupStr(NULL, cell->parent->concepts, "CHIP.NAME"); // Name of chip
    const char *cellName = psMetadataLookupStr(NULL, cell->concepts, "CELL.NAME"); // Name of cell

    psArray *table = psMetadataLookupPtr(NULL, cell->analysis, name); // The FITS table
    if (!table) {
        // We wrote everything we could find
        return 0;
    }

    psString headerName = NULL;         // Name for header in analysis metadata
    psStringAppend(&headerName, "%s.HEADER", name);
    psMetadata *header = psMetadataLookupMetadata(NULL, cell->analysis, headerName); // The FITS header
    psFree(headerName);

    psString extname = NULL;            // Extension name
    psStringAppend(&extname, "%s_%s_%s", name, chipName, cellName);

    // XXX Could do a table lookup from the camera format, in case the input file isn't laid out with
    // NAME_CHIP_CELL extension names --- use these as keys, and the value as the proper extension name.
    // Allow interpolation of concepts, e.g., "{CHIP.NAME}" --> "ccd13".

    if (!psFitsWriteTable(fits, header, table, extname)) {
        psError(PS_ERR_UNKNOWN, false, "Unable to write table from chip %s, cell %s to extension %s\n",
                chipName, cellName, extname);
        psFree(extname);
        return 0;
    }

    psFree(extname);
    return 1;
}


int pmChipWriteTable(psFits *fits, const pmChip *chip, const char *name)
{
    PS_ASSERT_PTR_NON_NULL(chip, 0);
    PS_ASSERT_PTR_NON_NULL(fits, 0);
    PS_ASSERT_STRING_NON_EMPTY(name, 0);

    int numWrite = 0;                    // Number of reads
    psArray *cells = chip->cells;       // Array of cells
    for (int i = 0; i < cells->n; i++) {
        pmCell *cell = cells->data[i];  // Cell of interest
        numWrite += pmCellWriteTable(fits, cell, name);
    }

    return numWrite;
}


int pmFPAWriteTable(psFits *fits, const pmFPA *fpa, const char *name)
{
    PS_ASSERT_PTR_NON_NULL(fpa, 0);
    PS_ASSERT_PTR_NON_NULL(fits, 0);
    PS_ASSERT_STRING_NON_EMPTY(name, 0);

    int numWrite = 0;                    // Number of reads
    psArray *chips = fpa->chips;        // Array of chips
    for (int i = 0; i < chips->n; i++) {
        pmChip *chip = chips->data[i];  // Chip of interest
        numWrite += pmChipWriteTable(fits, chip, name);
    }

    return numWrite;
}

bool pmCellWriteCovariance(psFits *fits, const pmCell *cell)
{
    PS_ASSERT_PTR_NON_NULL(cell, false);
    PS_ASSERT_PTR_NON_NULL(fits, false);

    int numCovar = 0;
    psArray *readouts = cell->readouts; // Array of readouts
    for (int i = 0; i < readouts->n; i++) {
        pmReadout *readout = readouts->data[i]; // The readout of interest
        if (readout && readout->covariance) {
            numCovar++;
        }
    }
    if (numCovar == 0) {
        return true;
    }
    if (numCovar != readouts->n) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                "Number of covariances (%d) doesn't match number of readouts (%ld)",
                numCovar, readouts->n);
        return false;
    }

    // Check size of covariances
    int xMinCovar = INT_MAX, xMaxCovar = INT_MIN, yMinCovar = INT_MAX, yMaxCovar = INT_MIN; // Size
    for (int i = 0; i < readouts->n; i++) {
        pmReadout *readout = readouts->data[i]; // The readout of interest
        psAssert(readout, "Should be defined.");
        psKernel *covar = readout->covariance; // Covariance matrix
        psAssert(covar, "Should be defined.");
        xMinCovar = PS_MIN(xMinCovar, covar->xMin);
        xMaxCovar = PS_MAX(xMaxCovar, covar->xMax);
        yMinCovar = PS_MIN(yMinCovar, covar->yMin);
        yMaxCovar = PS_MAX(yMaxCovar, covar->yMax);
    }

    // Correct covariances to common size
    psArray *images = psArrayAlloc(numCovar); // Array of images
    for (int i = 0; i < readouts->n; i++) {
        pmReadout *readout = readouts->data[i]; // The readout of interest
        psAssert(readout, "Should be defined.");
        psKernel *covar = readout->covariance; // Covariance matrix
        psAssert(covar, "Should be defined.");
        int xMin = covar->xMin, xMax = covar->xMax, yMin = covar->yMin, yMax = covar->yMax;// Size
        if (xMin == xMinCovar && xMax == xMaxCovar && yMin == yMinCovar && yMax == yMaxCovar) {
            images->data[i] = psMemIncrRefCounter(covar->image);
        } else {
            psImage *new = psImageAlloc(xMaxCovar - xMinCovar + 1, yMaxCovar - yMinCovar + 1, PS_TYPE_F32);
            psImageInit(new, 0);
            psImageOverlaySection(new, covar->image, xMinCovar - xMin, yMinCovar - yMin, "=");
            images->data[i] = new;
        }
    }

    // Determine extension name
    const char *chipName = psMetadataLookupStr(NULL, cell->parent->concepts, "CHIP.NAME"); // Name of chip
    const char *cellName = psMetadataLookupStr(NULL, cell->concepts, "CELL.NAME"); // Name of cell
    psString extname = NULL;            // Extension name
    psStringAppend(&extname, "COVAR_%s_%s", chipName, cellName);

    // Generate header
    pmHDU *hdu = pmHDUFromCell(cell);   // HDU for cell
    psMetadata *header = psMetadataCopy(NULL, hdu->header); // Header to write

    psMetadataAddS32(header, PS_LIST_TAIL, "COVARIANCE.CENTRE.X", PS_META_REPLACE,
                     "Centre of covariance matrix in x", -xMinCovar);
    psMetadataAddS32(header, PS_LIST_TAIL, "COVARIANCE.CENTRE.Y", PS_META_REPLACE,
                     "Centre of covariance matrix in y", -yMinCovar);

    // Turn off compression
    int bitpix = fits->options ? fits->options->bitpix : 0; // Desired bits per pixel
    psFitsScaling scaling = fits->options ? fits->options->scaling : 0; // Current scaling method.
    psFitsCompression *compress = psFitsCompressionGet(fits); // Current compression options
    
/*     fprintf(stderr,"Attempting to write chip %s cell %s extension %s with scaling %d\n", */
/* 	    chipName,cellName,extname,fits->options->scaling); */
    if (!psFitsSetCompression(fits, PS_FITS_COMPRESS_NONE, NULL, 0, 0, 0)) {
        psError(PS_ERR_UNKNOWN, false, "Unable to set FITS compression to NONE");
        psFree(extname);
        psFree(header);
        psFree(images);
        psFree(compress);
        return false;
    }
    if (fits->options) {
        fits->options->bitpix = 0;
    }
    if (fits->options) {
        fits->options->scaling = psFitsScalingFromString("STDEV_POSITIVE"); // This is a bit of a hack. We don't really have a default value.
    }

    // Write images
    if (!psFitsWriteImageCube(fits, header, images, extname)) {
        psError(PS_ERR_UNKNOWN, false, "Unable to write covariances from chip %s, cell %s to extension %s",
                chipName, cellName, extname);
        psFree(extname);
        psFree(header);
        psFree(images);
        if (fits->options) {
            fits->options->bitpix = bitpix;
        }
        psFitsCompressionApply(fits, compress);
        psFree(compress);
        return 0;
    }
    psFree(extname);
    psFree(header);
    psFree(images);

    // Restore compression
    if (fits->options) {
        fits->options->bitpix = bitpix;
    }
    if (fits->options) {
        fits->options->scaling = scaling;
    }
    if (!psFitsCompressionApply(fits, compress)) {
        psError(PS_ERR_UNKNOWN, false, "Unable to restore FITS compression");
        psFree(compress);
        return false;
    }
    psFree(compress);

    return true;
}


bool pmChipWriteCovariance(psFits *fits, const pmChip *chip)
{
    PS_ASSERT_PTR_NON_NULL(chip, false);
    PS_ASSERT_PTR_NON_NULL(fits, false);

    psArray *cells = chip->cells;       // Array of cells
    for (int i = 0; i < cells->n; i++) {
        pmCell *cell = cells->data[i];  // Cell of interest
        if (!pmCellWriteCovariance(fits, cell)) {
            return false;
        }
    }

    return true;
}


bool pmFPAWriteCovariance(psFits *fits, const pmFPA *fpa)
{
    PS_ASSERT_PTR_NON_NULL(fpa, false);
    PS_ASSERT_PTR_NON_NULL(fits, false);

    psArray *chips = fpa->chips;        // Array of chips
    for (int i = 0; i < chips->n; i++) {
        pmChip *chip = chips->data[i];  // Chip of interest
        if (!pmChipWriteCovariance(fits, chip)) {
            return false;
        }
    }

    return true;
}

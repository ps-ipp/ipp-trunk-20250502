#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <pslib.h>

#include "pmConfig.h"
#include "pmConfigMask.h"
#include "pmDetrendDB.h"

#include "pmHDU.h"
#include "pmFPA.h"
#include "pmFPALevel.h"
#include "pmFPARead.h"
#include "pmFPAWrite.h"
#include "pmFPAMaskWeight.h"
#include "pmFPAview.h"
#include "pmFPAfile.h"
#include "pmFPAfileFitsIO.h"
#include "pmFPACopy.h"
#include "pmFPAConstruct.h"
#include "pmDark.h"
#include "pmConceptsWrite.h"

// Get a suitable FPA for the file; generate it if necessary
static pmFPA *suitableFPA(const pmFPAfile *file, // File for which to get FPA
                          const pmFPAview *view, // View at which to produce the FPA
                          pmConfig *config, // Configuration (for concepts update)
                          bool pixels   // Worry about copying pixels?
    )
{
    psAssert(file, "It's supposed to be here");
    psAssert(view, "It's supposed to be here");
    psAssert(config, "It's supposed to be here");

    if (!file->format) {                // Working with the same output format as input format
        return psMemIncrRefCounter(file->fpa);
    }

    // May need to change format
    pmFPALevel level = pmFPAviewLevel(view); // Level for the view
    if (level == PM_FPA_LEVEL_NONE || level == PM_FPA_LEVEL_READOUT) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                "This function shouldn't be called at the readout (or unknown) level.");
        return NULL;
    }

    // Does the HDU of interest conform to the desired format?
    pmHDU *hdu = pmFPAviewThisHDU(view, file->fpa); // The HDU of interest
    if (hdu && hdu->format == file->format) {
        // No work required
        return psMemIncrRefCounter(file->fpa);
    }

    // Otherwise, we have to generate a copy with the correct format

    pmFPAview *phuView = pmFPAviewAlloc(0); // View corresponding to the PHU
    *phuView = *view;               // Copy contents
    pmFPALevel phuLevel = pmFPAPHULevel(file->format); // Level for the PHU
    switch (phuLevel) {
      case PM_FPA_LEVEL_FPA:
        phuView->chip = -1;
        // Flow through
      case PM_FPA_LEVEL_CHIP:
        phuView->cell = -1;
        // Flow through
      case PM_FPA_LEVEL_CELL:
        phuView->readout = -1;
        break;
      case PM_FPA_LEVEL_READOUT:
      case PM_FPA_LEVEL_NONE:
      default:
        psAbort("Should never get here: bad phu level.\n");
    }

    pmFPA *copy = pmFPAConstruct(file->camera, file->cameraName);  // FPA to return
    if (!pmFPAAddSourceFromView(copy, phuView, file->format)) {
        psError(PS_ERR_UNKNOWN, false, "Unable to insert HDU into FPA for writing.\n");
        psFree(copy);
        psFree(phuView);
        return NULL;
    }
    psFree(phuView);

    switch (level) {
      case PM_FPA_LEVEL_FPA:
        if ((pixels && !pmFPACopy(copy, file->fpa)) ||
            (!pixels && !pmFPACopyStructure(copy, file->fpa, 1, 1))) {
            psError(PS_ERR_UNKNOWN, false, "Unable to copy FPA for format conversion.\n");
            return NULL;
        }
        return copy;
      case PM_FPA_LEVEL_CHIP: {
          pmChip *chip = pmFPAviewThisChip(view, copy); // Chip of interest
          pmChip *srcChip = pmFPAviewThisChip(view, file->fpa); // Source chip
          if ((pixels && !pmChipCopy(chip, srcChip)) ||
              (!pixels && !pmChipCopyStructure(chip, srcChip, 1, 1))) {
              psError(PS_ERR_UNKNOWN, false, "Unable to copy chip for format conversion.\n");
              return false;
          }
          return copy;
      }
      case PM_FPA_LEVEL_CELL: {
          pmCell *cell = pmFPAviewThisCell(view, copy); // Cell of interest
          pmCell *srcCell = pmFPAviewThisCell(view, file->fpa); // Source cell
          if ((pixels && !pmCellCopy(cell, srcCell)) ||
              (!pixels && !pmCellCopyStructure(cell, srcCell, 1, 1))) {
              psError(PS_ERR_UNKNOWN, false, "Unable to copy cell for format conversion.\n");
              return false;
          }
          return copy;
      }
      case PM_FPA_LEVEL_READOUT:
      case PM_FPA_LEVEL_NONE:
      default:
        psAbort("Should never get here: bad phu level.\n");
    }

    // Unreachable
    return NULL;
}


pmFPA *pmFPAfileSuitableFPA(const pmFPAfile *file, const pmFPAview *view, pmConfig *config, bool pixels)
{
    PS_ASSERT_PTR_NON_NULL(file, NULL);
    PS_ASSERT_PTR_NON_NULL(view, NULL);
    PS_ASSERT_PTR_NON_NULL(config, NULL);

    pmFPA *fpa = suitableFPA(file, view, config, pixels); // A suitable FPA for writing
    if (!fpa) {
        psError(PS_ERR_UNKNOWN, false, "Unable to produce suitable FPA.");
        return NULL;
    }

    // Ensure headers and all are updated
    // This is here so that the individual write functions (e.g., images, PSFs, sources, etc) don't have to
    // take care of all this themselves (because they generally don't).
    switch (file->type) {
      case PM_FPA_FILE_IMAGE:
      case PM_FPA_FILE_MASK:
      case PM_FPA_FILE_VARIANCE:
      case PM_FPA_FILE_HEADER:
      case PM_FPA_FILE_FRINGE:
      case PM_FPA_FILE_DARK:
      case PM_FPA_FILE_LINEARITY:
      case PM_FPA_FILE_CMP:
      case PM_FPA_FILE_CMF:
      case PM_FPA_FILE_PSF:
      case PM_FPA_FILE_ASTROM_MODEL:
      case PM_FPA_FILE_ASTROM_REFSTARS: 
      case PM_FPA_FILE_KH_CORRECT:
      case PM_FPA_FILE_PATTERN_ROW_AMP:
      case PM_FPA_FILE_PATTERN_DEAD_CELLS:
	{
          pmHDU *hdu = pmFPAviewThisHDU(view, fpa);
          if (hdu) {
              if (!hdu->header) {
                  hdu->header = psMetadataAlloc();
              }

              pmConfigConformHeader(hdu->header, file->format);

              // whenever we write out a mask image, we should define the bits which represent mask concepts
              if (file->type == PM_FPA_FILE_MASK) {
                  assert (hdu->header);
                  if (!pmConfigMaskWriteHeader(config, hdu->header)) {
                      psError(PS_ERR_UNKNOWN, false,
                              "failed to set the bitmask names in the PHU header for Image %s (%s)\n",
                              file->filename, file->name);
                      return false;
                  }
              }
          }

          pmChip *chip = pmFPAviewThisChip(view, fpa); // Chip of interest, or NULL
          pmCell *cell = pmFPAviewThisCell(view, fpa); // Cell of interest, or NULL
          if (cell) {
              if (!pmConceptsWriteCell(cell, true, config)) {
                  psError(PS_ERR_IO, false, "Unable to write concepts for cell.\n");
                  return false;
              }
          } else if (chip) {
              if (!pmConceptsWriteChip(chip, true, true, config)) {
                  psError(PS_ERR_IO, false, "Unable to write concepts for chip.\n");
                  return false;
              }
          } else if (!pmConceptsWriteFPA(fpa, true, config)) {
              psError(PS_ERR_IO, false, "Unable to write concepts for FPA.\n");
              return false;
          }

          if (!pmFPAUpdateNames(fpa, chip, cell, file->imageId, file->sourceId)) {
              psError(PS_ERR_UNKNOWN, false, "Unable to update names in header.");
              return false;
          }
          break;
      }
      default:
        // No action
        break;
    }

    return fpa;
}

// given an already-opened fits file, read the table corresponding to the specified view
bool pmFPAviewReadFitsTable(const pmFPAview *view, pmFPAfile *file, const char *name)
{
    PS_ASSERT_PTR_NON_NULL(view, false);
    PS_ASSERT_PTR_NON_NULL(file, false);

    pmFPA *fpa = file->fpa;             // FPA of interest
    psFits *fits = file->fits;          // FITS file

    if (view->chip == -1) {
        return pmFPAReadTable(fpa, fits, name) > 0;
    }

    if (view->cell == -1) {
        pmChip *chip = pmFPAviewThisChip(view, fpa); // Chip of interest
        return pmChipReadTable(chip, fits, name) > 0;
    }

    pmCell *cell = pmFPAviewThisCell(view, fpa); // Cell of interest
    return pmCellReadTable(cell, fits, name) > 0;
}

// given an already-opened fits file, write the table corresponding to the specified view
bool pmFPAviewWriteFitsTable(const pmFPAview *view, pmFPAfile *file, const char *name, pmConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(view, false);
    PS_ASSERT_PTR_NON_NULL(file, false);

    pmFPA *fpa = pmFPAfileSuitableFPA(file, view, config, false); // FPA of interest
    psFits *fits = file->fits;          // FITS file

    if (view->chip == -1) {
        return pmFPAWriteTable(fits, fpa, name) > 0;
    }

    if (view->cell == -1) {
        pmChip *chip = pmFPAviewThisChip(view, fpa); // Chip of interest
        return pmChipWriteTable(fits, chip, name) > 0;
    }

    pmCell *cell = pmFPAviewThisCell(view, fpa); // Cell of interest
    return pmCellWriteTable(fits, cell, name) > 0;
}


// given an already-opened fits file, read the components corresponding to the specified view
static bool fpaViewReadFitsImage(const pmFPAview *view, // FPA view, specifying the level of interest
                                 pmFPAfile *file, // FPA file of interest
                                 pmConfig *config, // Configuration
                                 bool (*fpaReadFunc)(pmFPA*, psFits*, pmConfig*), // Function to read FPA
                                 bool (*chipReadFunc)(pmChip*, psFits*, pmConfig*), // Function to read chip
                                 bool (*cellReadFunc)(pmCell*, psFits*, pmConfig*) // Function to read cell
                                )
{
    assert(view);
    assert(file);

    pmFPA *fpa = file->fpa;             // FPA of interest
    psFits *fits = file->fits;          // FITS file from which to read

    if (view->chip == -1) {
        return fpaReadFunc(fpa, fits, config);
    }

    if (view->chip >= fpa->chips->n) {
        psError(PS_ERR_IO, true, "Requested chip == %d >= fpa->chips->n == %ld", view->chip, fpa->chips->n);
        return false;
    }
    pmChip *chip = fpa->chips->data[view->chip]; // Chip of interest

    if (view->cell == -1) {
        return chipReadFunc(chip, fits, config);
    }

    if (view->cell >= chip->cells->n) {
        psError(PS_ERR_IO, true, "Requested cell == %d >= chip->cells->n == %ld", view->cell, chip->cells->n);
        return false;
    }
    pmCell *cell = chip->cells->data[view->cell]; // Cell of interest

    if (view->readout == -1) {
        return cellReadFunc(cell, fits, config);
    }
    psError(PS_ERR_UNKNOWN, true, "Bad view: %d,%d", view->chip, view->cell);
    return false;

    // XXX pmReadoutRead, pmReadoutReadSegement disabled for now
    #if 0

    if (view->readout >= cell->readouts->n) {
        psError(PS_ERR_IO, true, "Requested readout == %d >= cell->readouts->n == %d",
                view->readout, cell->readouts->n);
        return false;
    }
    pmReadout *readout = cell->readouts->data[view->readout];

    if (view->nRows == 0) {
        pmReadoutRead (readout, fits, config);
    } else {
        pmReadoutReadSegment (readout, fits, view->nRows, view->iRows, NULL, NULL);
    }
    return true;
    #endif
}


bool pmFPAviewReadFitsImage(const pmFPAview *view, pmFPAfile *file, pmConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(view, false);
    PS_ASSERT_PTR_NON_NULL(file, false);
    return fpaViewReadFitsImage(view, file, config, pmFPARead, pmChipRead, pmCellRead);
}

bool pmFPAviewReadFitsMask(const pmFPAview *view, pmFPAfile *file, pmConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(view, false);
    PS_ASSERT_PTR_NON_NULL(file, false);
    return fpaViewReadFitsImage(view, file, config, pmFPAReadMask, pmChipReadMask, pmCellReadMask);
}

bool pmFPAviewReadFitsVariance(const pmFPAview *view, pmFPAfile *file, pmConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(view, false);
    PS_ASSERT_PTR_NON_NULL(file, false);
    return fpaViewReadFitsImage(view, file, config, pmFPAReadVariance, pmChipReadVariance,
                                pmCellReadVariance);
}

bool pmFPAviewReadFitsDark(const pmFPAview *view, pmFPAfile *file, pmConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(view, false);
    PS_ASSERT_PTR_NON_NULL(file, false);
    return fpaViewReadFitsImage(view, file, config, pmFPAReadDark, pmChipReadDark, pmCellReadDark);
}

bool pmFPAviewReadFitsHeaderSet(const pmFPAview *view, pmFPAfile *file, pmConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(view, false);
    PS_ASSERT_PTR_NON_NULL(file, false);
    return fpaViewReadFitsImage(view, file, config, pmFPAReadHeaderSet, pmChipReadHeaderSet,
                                pmCellReadHeaderSet);
}

// given an already-opened fits file, write the components corresponding
// to the specified view. when the file was opened, pmFPA/Chip/CellWrite was
// called on it with blank=true to write the (possible) blank PHU
// do NOT call the functions below with blank=true or they will write
// out data in an inconsistent fashion
// the calls below should recurse down the element to write out all components.
static bool fpaViewWriteFitsImage(const pmFPAview *view, // FPA view, specifying the level of interest
                                  pmFPAfile *file, // FPA file of interest
                                  pmConfig *config, // Configuration
                                  bool (*fpaWriteFunc)(pmFPA*, psFits*, pmConfig*, bool, bool), // Func FPA
                                  bool (*chipWriteFunc)(pmChip*, psFits*, pmConfig*, bool, bool),// Func chip
                                  bool (*cellWriteFunc)(pmCell*, psFits*, pmConfig*, bool) // Func cell
                                 )
{
    assert(view);
    assert(file);

    psFits *fits = file->fits;          // FITS file
    PS_ASSERT_PTR_NON_NULL(fits, false);

    pmFPA *fpa = pmFPAfileSuitableFPA(file, view, config, true); // FPA to write

    switch (pmFPAviewLevel(view)) {
    case PM_FPA_LEVEL_FPA: {
            bool success = fpaWriteFunc(fpa, fits, config, false, true);
            psFree(fpa);
            return success;
        }
    case PM_FPA_LEVEL_CHIP: {
            pmChip *chip = pmFPAviewThisChip(view, fpa); // Chip of interest
            bool success = chipWriteFunc(chip, fits, config, false, true);
            psFree(fpa);
            return success;
        }
    case PM_FPA_LEVEL_CELL: {
            pmCell *cell = pmFPAviewThisCell(view, fpa); // Cell of interest
            bool success = cellWriteFunc(cell, fits, config, false);
            psFree(fpa);
            return success;
        }
    case PM_FPA_LEVEL_READOUT:
        #if 0 // XXX disable readout write for now

        {
            pmReadout *readout = pmFPAviewThisReadout(view, file->fpa); // Readout of interest
            if (changeFormat)
        {
            // No copy function defined for readouts!
            psError(PS_ERR_UNKNOWN, false, "Unable to copy readout for format conversion on write.\n");
                return false;
            }
            if (view->nRows == 0)
        {
            return pmReadoutWrite(readout, fits, NULL, NULL);
            } else
            {
                return pmReadoutWriteSegment(readout, fits, view->nRows, view->iRows, NULL, NULL);
            }
        }
        #endif
    case PM_FPA_LEVEL_NONE:
    default:
        psAbort("Should never reach here: invalid file level.");
    }

    return false;
}

bool pmFPAviewWriteFitsImage(const pmFPAview *view, pmFPAfile *file, pmConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(view, false);
    PS_ASSERT_PTR_NON_NULL(file, false);
    return fpaViewWriteFitsImage(view, file, config, pmFPAWrite, pmChipWrite, pmCellWrite);
}

bool pmFPAviewWriteFitsMask(const pmFPAview *view, pmFPAfile *file, pmConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(view, false);
    PS_ASSERT_PTR_NON_NULL(file, false);
    return fpaViewWriteFitsImage(view, file, config, pmFPAWriteMask, pmChipWriteMask, pmCellWriteMask);
}

bool pmFPAviewWriteFitsVariance(const pmFPAview *view, pmFPAfile *file, pmConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(view, false);
    PS_ASSERT_PTR_NON_NULL(file, false);
    return fpaViewWriteFitsImage(view, file, config, pmFPAWriteVariance, pmChipWriteVariance,
                                 pmCellWriteVariance);
}

bool pmFPAviewWriteFitsDark(const pmFPAview *view, pmFPAfile *file, pmConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(view, false);
    PS_ASSERT_PTR_NON_NULL(file, false);
    return fpaViewWriteFitsImage(view, file, config, pmFPAWriteDark, pmChipWriteDark, pmCellWriteDark);
}

// given an already-opened fits file, read the components corresponding
// to the specified view
bool pmFPAviewFreeData(const pmFPAview *view, pmFPAfile *file)
{
    PS_ASSERT_PTR_NON_NULL(view, false);
    PS_ASSERT_PTR_NON_NULL(file, false);

    pmFPA *fpa = file->fpa;

    if (view->chip == -1) {
        psTrace ("pmFPAfile", 5, "freeing fpa for %s\n", file->filename);
        pmFPAFreeData (fpa);
        // XXX drop me: file->fpa = NULL;
        return true;
    }

    if (view->chip >= fpa->chips->n) {
        psError(PS_ERR_IO, true, "Requested chip == %d >= fpa->chips->n == %ld", view->chip, fpa->chips->n);
        return false;
    }
    pmChip *chip = fpa->chips->data[view->chip];

    if (view->cell == -1) {
        psTrace ("pmFPAfile", 5, "freeing chip %d for %s\n", view->chip, file->filename);
        pmChipFreeData (chip);
        return true;
    }

    if (view->cell >= chip->cells->n) {
        psError(PS_ERR_IO, true, "Requested cell == %d >= chip->cells->n == %ld", view->cell, chip->cells->n);
        return false;
    }
    pmCell *cell = chip->cells->data[view->cell];

    if (view->readout == -1) {
        psTrace ("pmFPAfile", 5, "freeing cell %d for %s\n", view->cell, file->filename);
        pmCellFreeData (cell);
        return true;
    }
    psError(PS_ERR_UNKNOWN, true, "Returning false");
    return false;

    // XXX pmReadoutRead, pmReadoutReadSegement disabled for now
    #if 0

    if (view->readout >= cell->readouts->n) {
        psError(PS_ERR_IO, true, "Requested readout == %d >= cell->readouts->n == %d",
                view->readout, cell->readouts->n);
        return false;
    }
    pmReadout *readout = cell->readouts->data[view->readout];

    if (view->nRows == 0) {
        pmReadoutRead (readout, fits, NULL);
    } else {
        pmReadoutReadSegment (readout, fits, view->nRows, view->iRows, NULL, NULL);
    }
    return true;
    #endif
}

#if 0
// Shouldn't need this --- when we want to free fringe data, we want to free the whole level, not just the
// table.

// Free the table within a cell
static void freeTable(pmCell *cell,     // Cell of interest
                      const char *name  // Name of table to free
                     )
{
    assert(cell);
    assert(name && strlen(name) > 0);

    psString headerName = NULL;         // Name of header
    psStringAppend(&headerName, "%s.HEADER", name);
    if (psMetadataLookup(cell->analysis, headerName)) {
        psMetadataRemoveKey(cell->analysis, headerName);
    }
    psFree(headerName);

    if (psMetadataLookup(cell->analysis, name)) {
        psMetadataRemoveKey(cell->analysis, name);
    }

    return;
}

// given a file, free the components corresponding to the specified view
bool pmFPAviewFreeFitsTable (const pmFPAview *view, pmFPAfile *file, const char *name)
{
    PS_ASSERT_PTR_NON_NULL(view, false);
    PS_ASSERT_PTR_NON_NULL(file, false);

    pmFPA *fpa = file->fpa;

    if (view->chip == -1) {
        psArray *chips = fpa->chips;    // Array of chips
        for (int i = 0; i < chips->n; i++) {
            pmChip *chip = chips->data[i]; // Chip of interest
            psArray *cells = chip->cells; // Array of cells
            for (int j = 0; j < cells->n; j++) {
                pmCell *cell = cells->data[j]; // Cell of interest
                freeTable(cell, name);
            }
        }
        return true;
    }

    if (view->cell == -1) {
        pmChip *chip = pmFPAviewThisChip(view, fpa); // Chip of interest
        psArray *cells = chip->cells;   // Array of cells
        for (int i = 0; i < cells->n; i++) {
            pmCell *cell = cells->data[i]; // Cell of interest
            freeTable(cell, name);
        }
        return true;
    }

    pmCell *cell = pmFPAviewThisCell(view, fpa); // Cell of interest
    freeTable(cell, name);
    return true;
}

#endif

bool pmFPAviewFitsWritePHU (const pmFPAview *view, pmFPAfile *file, pmConfig *config) {

    bool status = false;

    if (file->mode != PM_FPA_MODE_WRITE) return true;
    if (file->wrote_phu) return true;

    // select or generate the desired fpa in the correct output format
    pmFPA *fpa = pmFPAfileSuitableFPA(file, view, config, false);
    pmHDU *phu = pmFPAviewThisHDU(view, fpa);
    if (!phu || !phu->blankPHU) {
        // No PHU to write!
        psFree(fpa);
        return true;
    }

    // whenever we write out a mask image, we should define the bits which represent mask concepts
    if (file->type == PM_FPA_FILE_MASK) {
        assert (phu->header);
        if (!pmConfigMaskWriteHeader (config, phu->header)) {
            psError(PS_ERR_UNKNOWN, false, "failed to set the bitmask names in the PHU header for Image %s (%s)\n", file->filename, file->name);
            return false;
        }
    }

    switch (file->fileLevel) {
      case PM_FPA_LEVEL_FPA:
        status = pmFPAWrite(fpa, file->fits, config, true, false);
        break;
      case PM_FPA_LEVEL_CHIP: {
          pmChip *chip = pmFPAviewThisChip(view, fpa);
          status = pmChipWrite(chip, file->fits, config, true, false);
          break;
      }
      case PM_FPA_LEVEL_CELL: {
          pmCell *cell = pmFPAviewThisCell(view, fpa);
          status = pmCellWrite(cell, file->fits, config, true);
          break;
      }
      default:
        psAbort("fileLevel not correctly set");
        break;
    }

    if (!status) {
        psError(PS_ERR_UNKNOWN, false, "failed to write PHU for Image %s (%s)\n", file->filename, file->name);
        return false;
    }

    psFree(fpa);
    file->wrote_phu = true;
    return true;
}

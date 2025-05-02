#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <pslib.h>

#include "pmHDU.h"
#include "pmFPA.h"
#include "pmFPALevel.h"
#include "pmFPAUtils.h"
#include "pmShifts.h"

#define SHIFTS_BUFFER 100               // Buffer size for shifts
#define TRACE "psModules.detrend"       // Trace facility
#define FFT_SIZE 25                     // Size at which we use FFT instead of direct convolution

// XXX To do:
// * Make the table column names configurable, by having a SHIFTS metadata in the camera format, with entries
//   specifying the column names.


static void shiftsFree(pmShifts *shifts)
{
    psFree(shifts->x);
    psFree(shifts->y);
    psFree(shifts->t);
    return;
}

pmShifts *pmShiftsAlloc(bool tRel, bool xyRel)
{
    pmShifts *shifts = psAlloc(sizeof(pmShifts));
    psMemSetDeallocator(shifts, (psFreeFunc)shiftsFree);

    shifts->x = psVectorAllocEmpty(SHIFTS_BUFFER, PS_TYPE_S32);
    shifts->y = psVectorAllocEmpty(SHIFTS_BUFFER, PS_TYPE_S32);
    shifts->t = psVectorAllocEmpty(SHIFTS_BUFFER, PS_TYPE_F32);
    shifts->num = 0;

    // Suitable defaults
    shifts->tRelative = tRel;
    shifts->xyRelative = xyRel;

    return shifts;
}

// Look up the cell within a hash; supplement the hash with a new value if it doesn't exist
static pmShifts *cellVectors(psHash *shifts, // Hash of shifts
                             const char *cellName, // Key for hash
                             bool tRel, bool xyRel // Are the shifts relative?
    )
{
    assert(shifts);
    assert(cellName);

    // Find the appropriate cell
    pmShifts *vectors = psHashLookup(shifts, cellName);
    if (!vectors) {
        vectors = pmShiftsAlloc(tRel, xyRel);
        psHashAdd(shifts, cellName, vectors);
        psFree(vectors);            // Drop reference
    }
    return vectors;
}


bool pmShiftsRead(const pmCell *cell, psFits *fits)
{
    PS_ASSERT_PTR_NON_NULL(fits, false);
    PS_ASSERT_PTR_NON_NULL(cell, false);

    bool mdok;                          // Status of MD lookup
    pmShifts *check = psMetadataLookupPtr(&mdok, cell->analysis, PM_SHIFTS_TABLE_NAME); // Table, or NULL
    if (check) {
        psTrace(TRACE, 2, "Cell already has OT kernel present.\n");
        return true;                    // No error
    }
    psTrace(TRACE, 2, "Reading FITS file for OT kernels.\n");

    // Determine camera layout
    pmChip *chip = cell->parent;        // The parent chip
    pmFPA *fpa = chip->parent;          // The parent FPA
    pmHDU *phu = NULL;                  // The primary header
    pmFPALevel phuLevel = PM_FPA_LEVEL_NONE; // Level of the PHU
    long numChips = 0;                  // Number of chips below the PHU; for setting efficient hash size
    long numCells = 0;                  // Number of cells below the PHU; for setting efficient hash size
    if (fpa->hdu) {
        phu = fpa->hdu;
        // Count the cells
        psArray *chips = fpa->chips;    // Array of chips
        numChips = chips->n;
        for (int i = 0; i < chips->n; i++) {
            pmChip *chip = chips->data[i]; // Chip of interest
            numCells += chip->cells->n;
        }
        numCells = (float)numCells / (float)numChips + 0.5; // Average number of cells per chip
        phuLevel = PM_FPA_LEVEL_FPA;
    }
    if (!phu && chip->hdu) {
        phu = chip->hdu;
        numChips = 1;
        numCells = chip->cells->n;
        phuLevel = PM_FPA_LEVEL_CHIP;
    }
    if (!phu && cell->hdu) {
        phu = cell->hdu;
        numChips = 0;
        numCells = 1;
        phuLevel = PM_FPA_LEVEL_CELL;
    }
    if (!phu || phuLevel == PM_FPA_LEVEL_NONE) {
        psError(PS_ERR_UNEXPECTED_NULL, true, "Can't find the PHU.\n");
        return false;
    }


    // Find out what to read
    psMetadata *shiftsInfo = psMetadataLookupMetadata(&mdok, phu->format, "SHIFTS"); // Shifts information
    if (!mdok || !shiftsInfo) {
        // We have read all the shifts that we have been told about --- which is none.
        return true;
    }
    const char *shiftsExt = psMetadataLookupStr(&mdok, shiftsInfo, "EXTENSION"); // Extension name for shifts
    if (!mdok || !shiftsExt) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                "Can't find EXTENSION name in SHIFTS information from camera format.");
        return false;
    }
    const char *chipCol = NULL;         // Column name for chip
    if (phuLevel == PM_FPA_LEVEL_FPA) {
        chipCol = psMetadataLookupStr(&mdok, shiftsInfo, "CHIP");
        if (!mdok || !chipCol) {
            psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                    "Can't find CHIP column name in SHIFTS information from camera format.");
            return false;
        }
    }
    const char *cellCol = NULL;         // Column name for cell
    if (phuLevel <= PM_FPA_LEVEL_CHIP) {
        cellCol = psMetadataLookupStr(&mdok, shiftsInfo, "CELL");
        if (!mdok || !chipCol) {
            psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                    "Can't find CELL column name in SHIFTS information from camera format.");
            return false;
        }
    }
    const char *tCol = psMetadataLookupStr(&mdok, shiftsInfo, "T");
    if (!mdok || !tCol) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                "Can't find T column name in SHIFTS information from camera format.");
        return false;
    }
    const char *xCol = psMetadataLookupStr(&mdok, shiftsInfo, "X");
    if (!mdok || !xCol) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                "Can't find X column name in SHIFTS information from camera format.");
        return false;
    }
    const char *yCol = psMetadataLookupStr(&mdok, shiftsInfo, "Y");
    if (!mdok || !yCol) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                "Can't find Y column name in SHIFTS information from camera format.");
        return false;
    }

    bool tRel = psMetadataLookupBool(&mdok, shiftsInfo, "TRELATIVE");
    if (!mdok) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                "Can't find TRELATIVE in SHIFTS information from camera format.");
        return false;
    };
    bool xyRel = psMetadataLookupStr(&mdok, shiftsInfo, "XYRELATIVE");
    if (!mdok) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                "Can't find XYRELATIVE in SHIFTS information from camera format.");
        return false;
    };

    // Read the FITS file
    int origExt = psFitsGetExtNum(fits); // Original extension number; to preserve position
    if (!psFitsMoveExtName(fits, shiftsExt)) {
        psError(PS_ERR_IO, false, "Unable to move to shifts extension %s", shiftsExt);
        return false;
    }
    psArray *table = psFitsReadTable(fits); // The table of shifts
    if (!table) {
        psError(PS_ERR_IO, false, "Unable to read shifts table.\n");
        return false;
    }

    // More sensible storage
    pmShifts *singleShifts = NULL;      // Shifts for a single cell
    psHash *multipleShifts = NULL;      // Shifts for multiple cells, stored by cell name
    switch (phuLevel) {
      case PM_FPA_LEVEL_FPA:
        multipleShifts = psHashAlloc(2 * numChips);
        break;
      case PM_FPA_LEVEL_CHIP:
        multipleShifts = psHashAlloc(2 * numCells);
        break;
      case PM_FPA_LEVEL_CELL:
        singleShifts = pmShiftsAlloc(tRel, xyRel);
        break;
      default:
        psAbort("Should never get here.\n");
    }

    // Pull values out of the table into something a bit more sensible
    for (int i = 0; i < table->n; i++) {
        psMetadata *row = table->data[i]; // The row of interest

        const char *chipName = NULL;    // Name of chip
        if (phuLevel == PM_FPA_LEVEL_FPA) {
            // Only care about the chip name if there's more than one chip
            chipName = psMetadataLookupStr(&mdok, row, chipCol);
            if (!mdok || !chipName) {
                psError(PS_ERR_UNEXPECTED_NULL, true, "Unable to find column %s in row %d of shifts table\n",
                        chipCol, i);
                psFree(multipleShifts);
                psFree(table);
                return false;
            }
        }
        const char *cellName = NULL;    // Name of cell
        if (phuLevel <= PM_FPA_LEVEL_CHIP) {
            // Only care about the cell name if there's a chip
            cellName = psMetadataLookupStr(&mdok, row, cellCol);
            if (!mdok || !cellName) {
                psError(PS_ERR_UNEXPECTED_NULL, true, "Unable to find column %s in row %d of shifts table\n",
                        cellCol, i);
                psFree(multipleShifts);
                psFree(table);
                return false;
            }
        }
        float x = psMetadataLookupS32(&mdok, row, xCol); // Shift in x
        if (!mdok) {
            psError(PS_ERR_UNEXPECTED_NULL, true, "Unable to find column %s in row %d of shifts table\n",
                    xCol, i);
            psFree(multipleShifts);
            psFree(table);
            return false;
        }
        float y = psMetadataLookupF32(&mdok, row, yCol); // Shift in y
        if (!mdok) {
            psError(PS_ERR_UNEXPECTED_NULL, true, "Unable to find column %s in row %d of shifts table\n",
                    yCol, i);
            psFree(multipleShifts);
            psFree(table);
            return false;
        }
        float t = psMetadataLookupF32(&mdok, row, tCol); // Time of shift
        if (!mdok) {
            psError(PS_ERR_UNEXPECTED_NULL, true, "Unable to find column %s in row %d of shifts table\n",
                    tCol, i);
            psFree(multipleShifts);
            psFree(table);
            return false;
        }

        pmShifts *shifts = NULL;        // Shifts for the cell of interest
        switch (phuLevel) {
          case PM_FPA_LEVEL_FPA: {
              psHash *cells = psHashLookup(multipleShifts, chipName); // Hash of cells
              if (!cells) {
                  cells = psHashAlloc(numCells);
                  psHashAdd(multipleShifts, chipName, cells);
                  psFree(cells);          // Drop reference
              }
              shifts = cellVectors(cells, cellName, tRel, xyRel);
              break;
          }
          case PM_FPA_LEVEL_CHIP:
            shifts = cellVectors(multipleShifts, cellName, tRel, xyRel);
            break;
          case PM_FPA_LEVEL_CELL:
            shifts = singleShifts;
            break;
          default:
            psAbort("Should never get here.\n");
        }

        // Add the shift
        psVectorExtend(shifts->x, 1, SHIFTS_BUFFER);
        psVectorExtend(shifts->y, 1, SHIFTS_BUFFER);
        psVectorExtend(shifts->t, 1, SHIFTS_BUFFER);
        shifts->x->data.S32[shifts->num] = (int)x;
        shifts->y->data.S32[shifts->num] = (int)y;
        shifts->t->data.F32[shifts->num] = t;
        shifts->num++;
    }
    psFree(table);

    // Put the kernels into their own cells
    if (phuLevel == PM_FPA_LEVEL_CELL) {
        // Only a single cell
        psMetadataAddPtr(cell->analysis, PS_LIST_TAIL, PM_SHIFTS_TABLE_NAME, PS_DATA_KERNEL,
                         "Orthogonal transfer shifts", singleShifts);
        psFree(singleShifts);
        return true;
    } else {
        psList *names = psHashKeyList(multipleShifts); // List of hash keys (chip/cell names)
        psListIterator *namesIter = psListIteratorAlloc(names, PS_LIST_HEAD, false); // Iterator for names
        const char *name;               // Name, from iteration
        while ((name = psListGetAndIncrement(namesIter))) {
            switch (phuLevel) {
              case PM_FPA_LEVEL_FPA: {
                  int chipNum = pmFPAFindChip(fpa, name); // Number of chip of interest
                  if (chipNum < 0) {
                      psError(PS_ERR_UNEXPECTED_NULL, false, "Unable to find chip %s", name);
                      psFree(namesIter);
                      psFree(names);
                      psFree(multipleShifts);
                      return false;
                  }
                  pmChip *chip = fpa->chips->data[chipNum]; // Chip of interest

                  // Loop over component cells
                  psHash *cells = psHashLookup(multipleShifts, name); // Hash of cells
                  psList *cellNames = psHashKeyList(multipleShifts); // List of hash keys (cell names)
                  psListIterator *cellNamesIter = psListIteratorAlloc(cellNames, PS_LIST_HEAD, false);
                  const char *cellName;       // Cell name, from iteration
                  while ((cellName = psListGetAndIncrement(cellNamesIter))) {
                      int cellNum = pmChipFindCell(chip, cellName); // Number of cell of interest
                      if (!cell) {
                          psError(PS_ERR_UNEXPECTED_NULL, false, "Unable to find cell %s", cellName);
                          psFree(cellNamesIter);
                          psFree(cellNames);
                          psFree(namesIter);
                          psFree(names);
                          psFree(multipleShifts);
                          return false;
                      }
                      pmCell *cell = chip->cells->data[cellNum]; // Cell of interest
                      if (psMetadataLookup(cell->analysis, PM_SHIFTS_TABLE_NAME)) {
                          // Already has a shifts table, for some reason
                          psWarning("Chip %s, cell %s already has a shifts table --- overwriting\n",
                                    name, cellName);
                      }

                      pmShifts *vectors = psHashLookup(cells, cellName); // Shifts for the cell of interest
                      psMetadataAddPtr(cell->analysis, PS_LIST_TAIL, PM_SHIFTS_TABLE_NAME,
                                       PS_DATA_KERNEL | PS_META_REPLACE,
                                       "Orthogonal transfer shifts", vectors);
                  }
                  psFree(cellNamesIter);
                  psFree(cellNames);
                  break;
              }
              case PM_FPA_LEVEL_CHIP: {
                  int cellNum = pmChipFindCell(chip, name); // Number of cell of interest
                  if (!cell) {
                      psError(PS_ERR_UNEXPECTED_NULL, false, "Unable to find cell %s", name);
                      psFree(namesIter);
                      psFree(names);
                      psFree(multipleShifts);
                      return false;
                  }
                  pmCell *cell = chip->cells->data[cellNum]; // Cell of interest
                  if (psMetadataLookup(cell->analysis, PM_SHIFTS_TABLE_NAME)) {
                      // Already has a shifts table, for some reason
                      psWarning("Cell %s already has a shifts table --- overwriting\n", name);
                  }

                  pmShifts *vectors = psHashLookup(multipleShifts, name); // Shifts for this cell
                  psMetadataAddPtr(cell->analysis, PS_LIST_TAIL, PM_SHIFTS_TABLE_NAME,
                                   PS_DATA_KERNEL | PS_META_REPLACE,
                                   "Orthogonal transfer shifts", vectors);
                  break;
              }
              default:
                psAbort("Should never get here.\n");
            }
        }
        psFree(namesIter);
        psFree(names);
        psFree(multipleShifts);
    }

    // Go back to the original position in the FITS file
    return psFitsMoveExtNum(fits, origExt, false);
}


// Generate a kernel and stuff it on the cell metadata
bool pmShiftsKernel(const pmCell *cell     // Cell to which the shifts belong
    )
{
    PS_ASSERT_PTR(cell, false);

    bool mdok;                          // Status of MD lookup
    pmShifts *shifts = psMetadataLookupPtr(&mdok, cell->analysis, PM_SHIFTS_TABLE_NAME);
    if (!shifts) {
        psError(PS_ERR_UNEXPECTED_NULL, true, "Unable to find shifts table for cell.\n");
        return false;
    }

    psKernel *kernel = psKernelGenerate(shifts->t, shifts->x, shifts->y,
                                        shifts->tRelative, shifts->xyRelative); // Shift kernel
    if (!kernel) {
        psError(PS_ERR_UNEXPECTED_NULL, false, "Unable to generate kernel from OT shifts");
        return false;
    }
    psMetadataAddPtr(cell->analysis, PS_LIST_TAIL, PM_SHIFTS_KERNEL_NAME, PS_DATA_KERNEL,
                     "Orthogonal transfer kernel, calculated from shifts", kernel);
    psFree(kernel);

    return true;
}

bool pmShiftsConvolve(pmReadout *detrend, const pmCell *source, psImageMaskType maskVal)
{
    PS_ASSERT_PTR(detrend, false);
    PS_ASSERT_PTR(source, false);

    bool mdok;                          // Status of MD lookup
    psKernel *kernel = psMetadataLookupPtr(&mdok, source->analysis, PM_SHIFTS_KERNEL_NAME);
    if (!kernel) {
        // Maybe they just forgot to generate the kernel with pmShiftsKernel
        if (psMetadataLookup(source->analysis, PM_SHIFTS_TABLE_NAME)) {
            if (!pmShiftsKernel(source)) {
                psError(PS_ERR_UNKNOWN, false, "Unable to generate shifts kernel.");
                return false;
            }
            // Hopefully it's there now
            kernel = psMetadataLookupPtr(&mdok, source->analysis, PM_SHIFTS_KERNEL_NAME);
            if (!kernel) {
                psError(PS_ERR_UNKNOWN, false, "Unable to find shifts kernel.");
                return false;
            }
        } else {
            psError(PS_ERR_UNKNOWN, false, "Unable to find shifts kernel or shifts table.");
            return false;
        }
    }

    if (detrend->image) {
#if 1
        // Always do direct convolution (no fuss with edge effects)
        psImage *convolved = psImageConvolveDirect(NULL, detrend->image, kernel);
#else
        // Kernel size-dependent convolution --- if it's big, use the FFT
        int xSize = kernel->xMax - kernel->xMin; // Kernel size in x
        int ySize = kernel->yMax - kernel->yMin; // Kernel size in y
        psImage *convolved;
        if (xSize * ySize < FFT_SIZE * FFT_SIZE) {
            convolved = psImageConvolveDirect(NULL, detrend->image, kernel);
        } else {
            // This is a little dodgy --- making choices about parameters without the user's input
            psStats *stats = psImageStats(PS_STAT_ROBUST_MEDIAN);
            stats->nSubsample = 10000;
            psImageBackground(stats, detrend->image, detrend->mask, maskVal, NULL);
            convolved = psImageConvolveFFT(detrend->image, kernel, stats->robustMedian);
        }
#endif
        if (!convolved) {
            psError(PS_ERR_UNKNOWN, false, "Unable to convolve detrend image.");
            return false;
        }
        psFree(detrend->image);
        detrend->image = convolved;
    }

    // Purposely ignoring the weight map --- don't care about the weight map for a detrend image

    if (maskVal && detrend->mask && !psImageConvolveMaskDirect(detrend->mask, detrend->mask, maskVal, 0,
                                                               kernel->xMin, kernel->xMax,
                                                               kernel->yMin, kernel->yMax)) {
        psError(PS_ERR_UNKNOWN, false, "Unable to convolve detrend mask.");
        return false;
    }

    return true;
}

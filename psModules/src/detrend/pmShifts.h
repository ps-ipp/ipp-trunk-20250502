#ifndef PM_SHIFTS_H
#define PM_SHIFTS_H

#define PM_SHIFTS_TABLE_NAME "SHIFTS.TABLE" ///< Name for table on the analysis metadata
#define PM_SHIFTS_KERNEL_NAME "SHIFTS.KERNEL" ///< Name for kernel on the analysis metadata

/// Shifts due to orthogonal transfer
typedef struct {
    psVector *x;                        ///< Shifts in x
    psVector *y;                        ///< Shifts in y
    psVector *t;                        ///< Times of shifts
    long num;                           ///< Number of values
    bool tRelative;                     ///< Are the time values relative (durations)?
    bool xyRelative;                    ///< Are the shift (x,y) values relative to the previous position?
} pmShifts;

/// Allocator for pmShifts
pmShifts *pmShiftsAlloc(bool tRel,      ///< Are the time values relative (durations)?
                        bool xyRel      ///< Are the shift (x,y) values relative to the previous position?
                        );

/// Read orthogonal transfer shifts table for a cell
///
/// Given a cell, this function searches for the orthogonal transfer shifts for this cell in the supplied FITS
/// file.  The FITS extension containing the shifts table is specified by the SHIFTS keyword within the FILE
/// information in the camera format.  If the extension is found, the shifts table is read and translated into
/// kernels which are placed in the analysis metadata of each cell.  Note that this operation is performed on
/// all cells within the FITS file (or at least, those contained within the shifts table), not just the cell
/// provided --- if we have to read the whole table, we may as well translate the whole lot.  If a kernel is
/// already present in the cell analysis metadata, the function returns true without doing any work.
bool pmShiftsRead(const pmCell *cell,         ///< Cell for which to search for shifts
                  psFits *fits          ///< FITS file in which to search for OT shifts extension
                  );


/// Generate a kernel for the cell from the orthogonal transfer shifts
///
/// The kernel is saved in the analysis metadata
bool pmShiftsKernel(const pmCell *cell   ///< Cell for which to generate kernel
                    );

/// Convolve a detrend with the appropriate orthogonal transfer convolution kernel from a science exposure.
///
/// The kernel is generated (with pmShiftsKernel) if required.  The image and mask are convolved with the
/// kernel (specified maskVal is smeared).  The weight map is not convolved.
bool pmShiftsConvolve(pmReadout *detrend, ///< Detrend readout to convolve
                      const pmCell *source, ///< Science exposure, containing a shifts kernel
                      psImageMaskType maskVal ///< Mask value to smear
                      );

#endif

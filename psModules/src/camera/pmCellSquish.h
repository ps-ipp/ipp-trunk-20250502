#ifndef PM_CELL_SQUISH_H
#define PM_CELL_SQUISH_H

/// Squish (combine all component readouts of) a cell
///
/// The component readouts are combined, optionally taking into account orthogonal transfer shifts (assumed to
/// already have been read) and masks.
bool pmCellSquish(pmCell *cell,         ///< Cell to have readouts combined
                  psImageMaskType maskVal,   ///< Value to be masked
                  bool useShifts        ///< Use the shifts when squishing?
    );

#endif

/* @file pmConceptsAverage.h
 * @brief Average the values of multiple concepts
 *
 * @author Paul Price, IfA
 *
 * @version $Revision: 1.11 $ $Name: not supported by cvs2svn $
 * @date $Date: 2008-04-10 06:31:42 $
 * Copyright 2005-2006 Institute for Astronomy, University of Hawaii
 */

#ifndef PM_CONCEPTS_AVERAGE_H
#define PM_CONCEPTS_AVERAGE_H

/// @addtogroup Concepts Data Abstraction Concepts
/// @{

/// Set a variety of concepts in an FPA by averaging over several
///
/// This function averages the values of the following concepts:
/// FPA.TIME
/// And ensure the following concepts are consistent:
/// FPA.FILTER
/// FPA.TIMESYS
/// The following concepts could be of interest to the user, but are not treated:
/// FPA.EXPOSURE
bool pmConceptsAverageFPAs(pmFPA *target,///< Target FPA
                           psList *sources ///< List of source FPAs
    );

/// Set a variety of concepts in a cell by averaging over several
///
/// In some instances, we want to combine the values of a concept for several cells into a single concept for
/// a single cell (e.g., when mosaicking multiple cells into a chip with one "cell").  This function averages
/// the values of various concepts:
/// - CELL.GAIN
/// - CELL.READNOISE
/// - CELL.EXPOSURE
/// - CELL.DARKTIME
/// - CELL.TIME
/// For other concepts, it ensures the values are consistent:
/// - CELL.READDIR
/// - CELL.TIMESYS
/// - CELL.X0, CELL.Y0
/// - CELL.XPARITY, CELL.YPARITY
/// And for others, it takes the "worst" possible value:
/// - CELL.SATURATION
/// - CELL.BAD
/// These concepts are only handled if the cells are all the same cell (mosaicking vs stacking):
/// - CELL.X0, CELL.Y0
/// - CELL.XPARITY, CELL.YPARITY
bool pmConceptsAverageCells(pmCell *target,///< Target cell
                            psList *sources, ///< List of source cells
                            psRegion *trimsec, ///< The new trim section
                            psRegion *biassec, ///< The new bias section
                            bool same   ///< Are the cells the same cell from different chips?
                           );

/// Set a variety of concepts in a chip by averaging over several 
bool pmConceptsAverageChips(pmChip *target,///< Target chip
                            psList *sources, ///< List of source chips
                            bool same   ///< Are the chips the same chip from different exposures?
                           );

/// @}
#endif

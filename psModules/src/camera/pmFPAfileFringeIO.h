/* @file  pmFPAfileFringeIO.h
 * @brief Read & Write Fringe tables
 *
 * @author EAM, IfA
 * @author PAP, IfA
 *
 * @version $Revision: 1.16 $
 * @date $Date: 2009-02-06 02:31:24 $
 * Copyright 2004-2005 Institute for Astronomy, University of Hawaii
 */

#ifndef PM_FPA_FILE_FRINGE_IO_H
#define PM_FPA_FILE_FRINGE_IO_H

/// @addtogroup Camera Camera Layout
/// @{

/// Read an fringes into the current view
bool pmFPAviewReadFringes(const pmFPAview *view, pmFPAfile *file);
int pmFPAReadFringes(pmFPA *fpa, psFits *fits);
int pmChipReadFringes(pmChip *chip, psFits *fits);
int pmCellReadFringes(pmCell *cell, psFits *fits);
bool pmFPAviewWriteFringes(const pmFPAview *view, pmFPAfile *file, pmConfig *config);
int pmFPAWriteFringes(psFits *fits, const pmFPA *fpa);
int pmChipWriteFringes(psFits *fits, const pmChip *chip);
int pmCellWriteFringes(psFits *fits, const pmCell *cell);

/// @}

# endif

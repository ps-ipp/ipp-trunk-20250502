/* @file  pmSpan.h
 *
 * @author EAM, IfA
 *
 * @version $Revision: 1.3 $ $Name: not supported by cvs2svn $
 * @date $Date: 2008-12-09 21:16:09 $
 * Copyright 2006 Institute for Astronomy, University of Hawaii
 */

# ifndef PM_SPAN_H
# define PM_SPAN_H

/// @addtogroup Objects Object Detection / Analysis Functions
/// @{

// Describe a segment of an image
typedef struct {
    int y;                              //!< Row that span's in
    int x0;                             //!< Starting column (inclusive)
    int x1;                             //!< Ending column (inclusive)
} pmSpan;

pmSpan *pmSpanAlloc(int y, int x1, int x2);
bool pmSpanTest(const psPtr ptr);
int pmSpanSortByYX (const void **a, const void **b);

/// @}
# endif /* PM_SPAN_H */

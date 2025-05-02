/* @file psVectorSmooth.h
 * @brief smooth the input vector
 *
 * $Revision: 1.3 $ $Name: not supported by cvs2svn $
 * $Date: 2007-06-20 20:21:30 $
 * Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
 */

#ifndef PS_VECTOR_SMOOTH_H
#define PS_VECTOR_SMOOTH_H

/// @addtogroup MathOps Mathematical Operations
/// @{

/// Smooth a vector with a Gaussian
psVector *psVectorSmooth(psVector *output, ///< Output vector, or NULL
                         const psVector *input, ///< Input vector (F32 or F64 only)
                         double sigma,  ///< Gausian width (standard deviations)
                         double Nsigma  ///< Number of standard deviations for Gaussian to extend
                        );

/// Smooth a vector with a boxcar
psVector *psVectorBoxcar(psVector *output, ///< Output vector, or NULL
                         const psVector *input, ///< Input vector (F32 or F64 only)
                         int size       ///< Boxcar size (one-sided size)
    );

/// @}
#endif

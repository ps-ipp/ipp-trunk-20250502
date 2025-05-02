/// @file  psVectorFFT.h
/// @brief Contains FFT transform related functions for psVector
///
/// @author Paul Price, IfA
/// @author Robert DeSonia, MHPCC
///
/// @version $Revision: 1.21 $ $Name: not supported by cvs2svn $
/// @date $Date: 2007-02-08 04:23:57 $
/// Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
///

#ifndef PS_VECTOR_FFT_H
#define PS_VECTOR_FFT_H

#include "psVector.h"

/// @addtogroup MathOps Mathematical Operations
/// @{

/// Forward FFT of a vector
///
/// Applies a forward FFT (exponent -1), with the result returned in both real and imaginary parts.  The FFT
/// is not normalised (a forward followed by a reverse is the original scaled by the number of elements).  The
/// FFT takes advantage of the fact that the input is purely real; hence the output size is N/2 + 1 (with
/// division rounding down).  Only implemented for F32 input.
bool psVectorForwardFFT(psVector **real,///< Real part of FFT
                        psVector **imag,///< Imaginary part of FFT
                        const psVector *in ///< Input vector (F32)
    );

/// Backward FFT of a vector
///
/// Applies a backward FFT (exponent +1) from the real and imaginary parts with the (purely) real result
/// returned.  The FFT is not normalised (a forward followed by a reverse is the original scaled by the number
/// of elements).  The FFT takes advantage of the fact that the output will be purely real; hence the input
/// size is N/2 + 1 (with division rounding down); to manage the redundancy (is the original size even or
/// odd?), we need the original size (the size of the array that was input to psVectorForwardFFT) to be
/// provided.  Only implemented for F32 input.
bool psVectorBackwardFFT(psVector **out,///< Output vector
                         const psVector *real, ///< Real input (F32)
                         const psVector *imag, ///< Imaginary input (F32)
                         long origNum    ///< Original number of elements
    );

/// Power spectrum of a vector
///
/// Generates the power spectrum of a vector.  Only implemented for F32 input.
psVector *psVectorPowerSpectrum(psVector *out, ///< Output power spectrum, or NULL
                                const psVector* in ///< Input vector (F32)
    );

/// Multiply complex vectors
///
/// The input vectors are the real and imaginary parts of each of two vectors.  The real and imaginary parts
/// of the output vector are returned.  Only implemented for F32 input.
bool psVectorComplexMultiply(psVector **outReal, ///< Real part of output
                             psVector **outImag, ///< Imaginary part of output
                             const psVector *in1Real, ///< Real part of input 1
                             const psVector *in1Imag, ///< Imaginary part of input 1
                             const psVector *in2Real, ///< Real part of input 2
                             const psVector *in2Imag ///< Imaginary part of input 2
    );

/// @}
#endif // #ifndef PS_VECTOR_FFT_H

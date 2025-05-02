/* @file  psBinaryOp.h
 *
 * @brief Provides binary functions for simple matrix and vector element operations. 
 * 
 * Functions include:
 *     Addition (+)
 *     Subtraction (-)
 *     Multiplication (*)
 *     Division (/)
 *     Power (^)
 *     Minimum (min)
 *     Maximum (max)
 *     Absolute value (abs)
 *     Exponent (exp)
 *     Natural Log (ln)
 *     Power of 10 (ten)
 *     Log (log)
 *     Sine (sin or dsin)
 *     Cosine (cos or dcos)
 *     Tangent (tan or dtan)
 *     Arcsine (asin or dasin)
 *     Arccosine (acos or dacos)
 *     Arctan (atan or datan)
 *
 * Currently only vector-vector and image-image binary operations are supported.
 *
 * @author Ross Harman, MHPCC
 * @author Robert DeSonia, MHPCC
 *
 * @version $Revision: 1.5 $ $Name: not supported by cvs2svn $
 * @date $Date: 2007-01-23 22:47:23 $
 *
 * Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
 */

#ifndef PSBINARY_OP_H
#define PSBINARY_OP_H

/// @addtogroup MathOps Mathematical Operations
/// @{

/** Perform simple binary arithmetic with images or vectors
 *
 *  Performs addition, subtraction, multiplication, division, power, minumum, and maximum arithmetic
 *  operations with images and vectors. Uses the form:
 *
 *      out = in1 op in2,
 *
 *      Where op is: "=", "+", "-", "*", "/", "^", "min", or "max"
 *
 *  This function only supports vector-vector or image-image operations.
 *
 *  @return  psType* : Pointer to either psImage or psVector.
 */
psMathType* psBinaryOp(
    psPtr out,                         ///< Output type, either psImage or psVector.
    psPtr in1,                   ///< First input, either psImage or psVector.
    const char *op,                    ///< Operator.
    psPtr in2                    ///< Second input, either psImage or psVector.
);

/// @}
#endif // #ifndef PSBINARY_OP_H

/* @file  psUnaryOp.h
 *
 * @brief Provides unary functions for simple matrix and vector element operations
 *
 * Defined unary functions include:
 *
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
 * @author EAM, IfA
 * @author Ross Harman, MHPCC
 * @author Robert DeSonia, MHPCC
 *
 * @version $Revision: 1.5 $ $Name: not supported by cvs2svn $
 * @date $Date: 2007-01-23 22:47:23 $
 * Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
 */

#ifndef PSUNARY_OP_H
#define PSUNARY_OP_H

/// @addtogroup MathOps Mathematical Operations
/// @{

/** Perform simple unary arithmetic with images or vectors
 *
 *  Performs absolute value, exponent, natural log, power of 10, log, sine, cosine, tangent, arcsine,
 *  arccosine, or arctan. operations with images and vectors. Uses the form:
 *
 *     out = op(in),
 *
 *     Where op is: "abs", "exp", "ln", "ten", "log", "sin", "cos", "tan" "asin", "acos", "atan", "dsin",
 *                  "dcos", dtan", "dasin", "dacos", or "datan".
 *
 *  Trigometric Operations with "d" prefix use units of degrees. Those without are in radians.
 *
 *  This function only supports vector-vector or image-image opertions.
 *
 *  @return  psType* : Pointer to either psImage or psVector.
 */
psMathType* psUnaryOp(
    psPtr out,                         ///< Output type, either psImage or psVector.
    psPtr in,			       ///< Input, either psImage or psVector.
    const char *op                     ///< Operator.
);

/// @}
#endif // #ifndef PSUNARY_OP_H

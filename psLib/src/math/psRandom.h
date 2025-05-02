/* @file psRandom.h
 * @brief Random Number Generators
 *
 * This file will hold the prototypes for procedures which allocate, free,
 * and evaluate random number Generators.
 *
 * @author GLG, MHPCC
 *
 * @version $Revision: 1.11 $ $Name: not supported by cvs2svn $
 * @date $Date: 2007-11-16 00:41:07 $
 *
 * Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
 */

#ifndef PS_RANDOM_H
#define PS_RANDOM_H

/// @addtogroup MathOps Mathematical Operations
/// @{

#include <stdio.h>
#include <stdbool.h>
#include <float.h>
#include <math.h>

#include "psVector.h"
#include "psScalar.h"
#include <gsl/gsl_rng.h>
#include <gsl/gsl_randist.h>

/// Random number generator types
typedef enum {
    PS_RANDOM_TAUS                     ///< A maximally equidistributed combined Tausworthe generator.
} psRandomType;

/// Random number generator
typedef struct {
    psRandomType type;                 ///< The type of RNG
    gsl_rng *gsl;                      ///< The RNG itself
} psRandom;

/// Get a seed from the system.
///
/// Tries /dev/random first, and then the system clock
psU64 p_psRandomGetSystemSeed(bool log  ///< Print a log message about the choice of seed?
    );

/// Set the seed to use for random number generators.
///
/// A seed value of zero indicates that the seed is to be generated from the system.
/// The new seed value is returned
psU64 psRandomSeed(psU64 seed           ///< Seed for RNG
                   );

/// Allocate a random number generator
///
/// The currently defined seed (via psRandomSeed) is used
psRandom *psRandomAlloc(
    psRandomType type                   ///< The type of RNG
) PS_ATTR_MALLOC;

/// Allocate a random number generator with a specific seed
///
/// A seed value of zero indicates that the seed is to be generated from the system.
psRandom *psRandomAllocSpecific(psRandomType type, ///< The type of RNG
                                psU64 specificSeed ///< The specific seed to use
    );

/// Resets an existing random number generator
bool psRandomReset(
    psRandom *rand                      ///< Random number generator to reset
);

/** Random number generator based on a uniform distribution on [0,1).
 *  Uses gsl_rng_uniform.
 *
 *  @return double:     Random number.
 */
double psRandomUniform(
    const psRandom *r                  ///< Random number generator
);

/** Random number generator based on a Gaussian deviate, N(0,1).
 *  Uses gsl_ran_gaussian.
 *
 *  @return double:     Random number.
 */
double psRandomGaussian(
    const psRandom *r                  ///< Random number generator
);

/** Random number generator based on a Gaussian deviate with specified standard deviation.
 *  Uses gsl_ran_gaussian.
 *
 *  @return double:     Random number.
 */
double p_psRandomGaussian(
    const psRandom *r,                  ///< Random number generator
    double sigma
);

/** Random number generator based on a Poisson distribution with the given mean.
 *  Uses gsl_ran_poisson.
 *
 *  @return double:     Random number.
 */
double psRandomPoisson(
    const psRandom *r,                  ///< Random number generator
    double mean                         ///< Mean value
);

#define PS_ASSERT_RANDOM_NON_NULL(NAME, RVAL) \
if (!(NAME) || !(NAME)->gsl) { \
    psError(PS_ERR_UNEXPECTED_NULL, true, "Error: Random number generator %s is NULL", #NAME); \
    return RVAL; \
}

/// @}
#endif // #ifndef PS_RANDOM_H

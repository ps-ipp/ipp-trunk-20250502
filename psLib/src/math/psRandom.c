/** @file psRandom.c
*  \brief Random Number Generators
*  \ingroup Math
*
*  This file will hold the functions which allocate, free,
*  and evaluate random number Generators.
*
*  @ingroup Math
*
*  @author GLG, MHPCC
*
*  @version $Revision: 1.18 $ $Name: not supported by cvs2svn $
*  @date $Date: 2008-11-05 11:12:39 $
*
*  Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
*/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdio.h>
#include <stdbool.h>
#include <float.h>
#include <math.h>
#include <time.h>
#include <gsl/gsl_rng.h>
#include <gsl/gsl_randist.h>
#include <inttypes.h>

#include "psAbort.h"
#include "psMemory.h"
#include "psRandom.h"
#include "psScalar.h"
#include "psError.h"
#include "psTrace.h"
#include "psLogMsg.h"
#include "psAssert.h"

// XXX set to non-zero for a test
# define HARDWIRED_SEED 0
unsigned long seed = HARDWIRED_SEED;                 // Seed for RNG

psU64 p_psRandomGetSystemSeed(bool log)
{
    psU64 seedVal = 0;                  // Seed value to return

    // Since zero is a special value in our context, don't allow the final value chosen to be zero
    while (seedVal == 0) {
        FILE *fd = fopen("/dev/urandom", "r");
        if (fd) {
            // Read urandom to get seed
            if (fread(&seedVal, sizeof(psU64), 1, fd)) {;} // ignore return value
            // Close file
            fclose(fd);
        } else {
            // Read system clock to get seed
            time_t timeVal;                 // Time value
            seedVal = (psU64)time(&timeVal);
        }
    }

    // Send log message of the system seed value used
    if (log) {
        psLogMsg(__func__,PS_LOG_INFO,"System random seed value used = %" PRIx64, seedVal);
    }

    return seedVal;
}

psU64 psRandomSeed(psU64 value)
{
    if (HARDWIRED_SEED) {
	seed = HARDWIRED_SEED;
	return seed;
    }

    while (value == 0) {
        value = p_psRandomGetSystemSeed(false);
    }
    seed = value;
    return seed;
}

// Destructor for psRandom
static void randomFree(psRandom *rng)
{
    if (rng->gsl) {
        gsl_rng_free(rng->gsl);
    }
    return;
}

// Constructor for psRandom
static psRandom *randomAlloc(psRandomType type)
{
    psRandom *rng = psAlloc(sizeof(psRandom)); // Random number generator to return
    psMemSetDeallocator(rng, (psFreeFunc)randomFree);

    rng->type = type;

    const gsl_rng_type *gslType;        // Type of RNG according to GSL
    switch (type) {
      case PS_RANDOM_TAUS:
        gslType = gsl_rng_taus;
        break;
      default:
        psAbort("Unknown Random Number Generator Type: %x", type);
        break;
    }

    rng->gsl = gsl_rng_alloc(gslType);
    return rng;
}

psRandom *psRandomAlloc(psRandomType type)
{
    psRandom *rng = randomAlloc(type);
    psRandomReset(rng);

    return rng;
}

psRandom *psRandomAllocSpecific(psRandomType type, psU64 specificSeed)
{
    psRandom *rng = randomAlloc(type);
    if (specificSeed == 0) {
        specificSeed = p_psRandomGetSystemSeed(true);
    }
    gsl_rng_set(rng->gsl, specificSeed);
    return rng;
}

bool psRandomReset(psRandom *rand)
{
    PS_ASSERT_RANDOM_NON_NULL(rand, false);
    if (seed == 0) {
        seed = p_psRandomGetSystemSeed(true);
    }
    gsl_rng_set(rand->gsl, seed);
    return true;
}

double psRandomUniform(const psRandom *r)
{
    PS_ASSERT_RANDOM_NON_NULL(r, NAN);
    return gsl_rng_uniform(r->gsl);
}

double psRandomGaussian(const psRandom *r)
{
    PS_ASSERT_RANDOM_NON_NULL(r, NAN);
    return gsl_ran_gaussian(r->gsl, 1.0);
}

double p_psRandomGaussian(const psRandom *r, double sigma)
{
    PS_ASSERT_RANDOM_NON_NULL(r, NAN);
    return gsl_ran_gaussian(r->gsl, sigma);
}

double psRandomPoisson(const psRandom *r, double mean)
{
    PS_ASSERT_RANDOM_NON_NULL(r, NAN);
    return gsl_ran_poisson(r->gsl, mean);
}


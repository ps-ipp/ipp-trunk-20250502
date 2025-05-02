#ifndef PS_ASSERT_H
#define PS_ASSERT_H

/// @addtogroup SysUtils System Utilities
/// @{

#include <assert.h>
#include <inttypes.h>
#include <math.h>

#include "psError.h"
#include "psLogMsg.h"

// these two asserts can be used in the middle of a function to test for programming errors
#define PS_ASSERT(VAR, RVAL) \
if (!(VAR)) { \
    psError(PS_ERR_PROGRAMMING, false, "Error: %s is not true.", #VAR); \
    return(RVAL); \
}

#define PS_ASSERT_INT_UNEQUAL(NAME1, NAME2, RVAL) \
if ((NAME1) == (NAME2)) { \
    psError(PS_ERR_BAD_PARAMETER_VALUE, true, \
            "Error: %s and %s are equal.", \
            #NAME1, #NAME2); \
    return(RVAL); \
}

#define PS_ASSERT_INT_UNEQUAL(NAME1, NAME2, RVAL) \
if ((NAME1) == (NAME2)) { \
    psError(PS_ERR_BAD_PARAMETER_VALUE, true, \
            "Error: %s and %s are equal.", \
            #NAME1, #NAME2); \
    return(RVAL); \
}

#define PS_ASSERT_INT_EQUAL(NAME1, NAME2, RVAL) \
if ((NAME1) != (NAME2)) { \
    psError(PS_ERR_BAD_PARAMETER_VALUE, true, \
            "Error: %s and %s are not equal.", \
            #NAME1, #NAME2); \
    return(RVAL); \
}

#define PS_ASSERT_INT_NONNEGATIVE(NAME, RVAL) \
if ((NAME) < 0) { \
    psError(PS_ERR_BAD_PARAMETER_VALUE, true, "Error: %s is less than 0.", #NAME); \
    return(RVAL); \
}

#define PS_ASSERT_INT_POSITIVE(NAME, RVAL) \
if ((NAME) < 1) { \
    psError(PS_ERR_BAD_PARAMETER_VALUE, true, \
            "Error: %s is 0 or less.", #NAME); \
    return(RVAL); \
}

#define PS_ASSERT_INT_ZERO(NAME, RVAL) \
if ((NAME) != 0) { \
    psError(PS_ERR_BAD_PARAMETER_VALUE, true, \
            "Error: %s is 0.", #NAME); \
    return(RVAL); \
}

#define PS_ASSERT_INT_NONZERO(NAME, RVAL) \
if ((NAME) == 0) { \
    psError(PS_ERR_BAD_PARAMETER_VALUE, true, \
            "Error: %s is 0.", #NAME); \
    return(RVAL); \
}

// XXX: Where did these int casts come from?
#define PS_ASSERT_INT_WITHIN_RANGE(NAME, LOWER, UPPER, RVAL) \
if ((int)(NAME) < LOWER || (int)(NAME) > UPPER) { \
    psError(PS_ERR_BAD_PARAMETER_VALUE, true, \
            "Error: %s, %ld, is out of range.  Must be between %ld and %ld.", \
            #NAME,(long)(NAME),(long)(LOWER),(long)(UPPER)); \
    return RVAL; \
}

#define PS_ASSERT_INT_LESS_THAN(VAR1, VAR2, RVAL) \
if (!(VAR1 < VAR2)) { \
    psError(PS_ERR_UNKNOWN, true, \
            "Error: %s is not less than %s (%ld, %ld)", #VAR1, #VAR2, (long)(VAR1), (long)(VAR2)); \
    return(RVAL); \
}

#define PS_ASSERT_INT_LESS_THAN_OR_EQUAL(VAR1, VAR2, RVAL) \
if (!(VAR1 <= VAR2)) { \
    psError(PS_ERR_UNKNOWN, true, \
            "Error: %s is not less than %s (%ld, %ld)", #VAR1, #VAR2, (long)(VAR1), (long)(VAR2)); \
    return(RVAL); \
}

#define PS_ASSERT_INT_LARGER_THAN(NAME1, NAME2, RVAL) \
if (!((NAME1) > (NAME2))) { \
    psError(PS_ERR_BAD_PARAMETER_VALUE, true, \
            "Error: !(%s > %s) (%ld %ld).", \
            #NAME1, #NAME2,(long)(NAME1), (long)(NAME2)); \
    return(RVAL); \
}

#define PS_ASSERT_INT_LARGER_THAN_OR_EQUAL(NAME1, NAME2, RVAL) \
if (!((NAME1) >= (NAME2))) { \
    psError(PS_ERR_BAD_PARAMETER_VALUE, true, \
            "Error: !(%s >= %s) (%ld %ld).", \
            #NAME1, #NAME2, (long)(NAME1), (long)(NAME2)); \
    return(RVAL); \
}
#define PS_ASSERT_FLOAT_LARGER_THAN(NAME1, NAME2, RVAL) \
if (!((NAME1) > (NAME2))) { \
    psError(PS_ERR_BAD_PARAMETER_VALUE, true, \
            "Error: !(%s > %s) (%lf %lf).", \
            #NAME1, #NAME2, (double)(NAME1), (double)(NAME2)); \
    return(RVAL); \
}

#define PS_ASSERT_FLOAT_LARGER_THAN_OR_EQUAL(NAME1, NAME2, RVAL) \
if (!((NAME1) >= (NAME2))) { \
    psError(PS_ERR_BAD_PARAMETER_VALUE, true, \
            "Error: !(%s >= %s) (%lf %lf).", \
            #NAME1, #NAME2, (double)(NAME1), (double)(NAME2)); \
    return(RVAL); \
}

#define PS_ASSERT_FLOAT_LESS_THAN(NAME1, NAME2, RVAL) \
if (!((NAME1) < (NAME2))) { \
    psError(PS_ERR_BAD_PARAMETER_VALUE, true, \
            "Error: !(%s < %s) (%lf %lf).", \
            #NAME1, #NAME2, (double)(NAME1), (double)(NAME2)); \
    return(RVAL); \
}

#define PS_ASSERT_FLOAT_LESS_THAN_OR_EQUAL(NAME1, NAME2, RVAL) \
if (!((NAME1) <= (NAME2))) { \
    psError(PS_ERR_BAD_PARAMETER_VALUE, true, \
            "Error: !(%s <= %s) (%lf %lf).", \
            #NAME1, #NAME2, (double)(NAME1), (double)(NAME2)); \
    return(RVAL); \
}

#define PS_ASSERT_FLOAT_NON_EQUAL(NAME1, NAME2, RVAL) \
if (fabs((NAME2) - (NAME1)) < FLT_EPSILON) { \
    psError(PS_ERR_BAD_PARAMETER_VALUE, true, \
            "Error: %s and %s are equal.", \
            #NAME1, #NAME2); \
    return(RVAL); \
}

#define PS_ASSERT_FLOAT_EQUAL(NAME1, NAME2, RVAL) \
if (fabs((NAME2) - (NAME1)) > FLT_EPSILON) { \
    psError(PS_ERR_BAD_PARAMETER_VALUE, true, \
            "Error: %s and %s are not equal.", \
            #NAME1, #NAME2); \
    return(RVAL); \
}

// Return an error if the arg lies outside the supplied range.
#define PS_ASSERT_FLOAT_WITHIN_RANGE(NAME, LOWER, UPPER, RVAL) \
if ((NAME) < (LOWER) || (NAME) > (UPPER)) { \
    psError(PS_ERR_BAD_PARAMETER_VALUE, true, \
            "Error: %s, %f, is out of range.  Must be between %lf and %lf.", \
            #NAME, NAME, (double)(LOWER), (double)(UPPER)); \
    return RVAL; \
}

#define PS_ASSERT_FLOAT_REAL(NAME, RVAL) \
if (!isfinite(NAME)) { \
    psError(PS_ERR_BAD_PARAMETER_VALUE, true, \
            "Error: %s=%lf is not a real value.\n", \
            #NAME, (double)(NAME)); \
    return RVAL; \
}

#define PS_ASSERT_DOUBLE_WITHIN_RANGE(NAME, LOWER, UPPER, RVAL) \
if ((NAME) < (LOWER) || (NAME) > (UPPER)) { \
    psError(PS_ERR_BAD_PARAMETER_VALUE, true, \
            "Error: %s, %lf, is out of range.  Must be between %lf and %lf.", \
            #NAME, NAME, LOWER, UPPER); \
    return RVAL; \
}

#define PS_ASSERT_LONG_WITHIN_RANGE(NAME, LOWER, UPPER, RVAL) \
if ((NAME) < (LOWER) || (NAME) > (UPPER)) { \
    psError(PS_ERR_BAD_PARAMETER_VALUE, true, \
            "Error: %s, %ld, is out of range.  Must be between %ld and %ld.", \
            #NAME, NAME, LOWER, UPPER); \
    return RVAL; \
}

#define PS_ASSERT_LONG_LARGER_THAN_OR_EQUAL(NAME1, NAME2, RVAL) \
if (!((NAME1) >= (NAME2))) { \
    psError(PS_ERR_BAD_PARAMETER_VALUE, true, "Error: !(%s >= %s) (%ld %ld).",\
            #NAME1, #NAME2, NAME1, NAME2); \
    return(RVAL); \
}

#define PS_ASSERT_S64_WITHIN_RANGE(NAME, LOWER, UPPER, RVAL) \
if ((NAME) < (LOWER) || (NAME) > (UPPER)) { \
    psError(PS_ERR_BAD_PARAMETER_VALUE, true, \
            "Error: %s, %" PRId64 ", is out of range.  Must be between %" PRId64 " and %" PRId64 ".", \
            #NAME, NAME, LOWER, UPPER); \
    return RVAL; \
}

/*****************************************************************************
Macros which take a generic psLib type and determine if it is NULL, or has
the wrong type.
*****************************************************************************/
#define PS_WARN_PTR_NON_NULL(NAME) \
if ((NAME) == NULL) { \
    psLogMsg(__func__, PS_LOG_WARN, "WARNING: %s is NULL.", #NAME); \
} \

#define PS_ASSERT_PTR(NAME, RVAL)

#define PS_ASSERT_PTR_NON_NULL(NAME, RVAL) PS_ASSERT_GENERAL_PTR_NON_NULL(NAME, return RVAL)
#define PS_ASSERT_GENERAL_PTR_NON_NULL(NAME, CLEANUP) \
if ((NAME) == NULL) { \
    psError(PS_ERR_BAD_PARAMETER_NULL, true, \
            "Unallowable operation: %s is NULL.", \
            #NAME); \
    CLEANUP; \
}

#define PS_ASSERT_PTR_NULL(NAME, RVAL) \
if ((NAME) != NULL) { \
    psError(PS_ERR_BAD_PARAMETER_NULL, true, \
            "Unallowable operation: %s is not NULL.", \
            #NAME); \
    return RVAL; \
}

#define PS_ASSERT_PTR_TYPE(NAME, TYPE, RVAL) \
if ((NAME)->type.type != TYPE) { \
    psError(PS_ERR_BAD_PARAMETER_TYPE, true, \
            "Unallowable operation: %s has incorrect type.", \
            #NAME); \
    return(RVAL); \
}

#define PS_ASSERT_PTR_DIMEN(NAME, DIMEN, RVAL) PS_ASSERT_GENERAL_PTR_DIMEN(NAME, DIMEN, return RVAL)
#define PS_ASSERT_GENERAL_PTR_DIMEN(NAME, DIMEN, CLEANUP) \
if ((NAME)->type.dimen != DIMEN) { \
    psError(PS_ERR_BAD_PARAMETER_TYPE, true, \
            "Unallowable operation: %s has incorrect dimensionality.", \
            #NAME); \
    CLEANUP; \
}

#define PS_ASSERT_PTR_DIMEN_GENERAL_NOT(NAME, DIMEN, CLEANUP) \
if ((NAME)->type.dimen == DIMEN) { \
    psError(PS_ERR_BAD_PARAMETER_TYPE, true, \
            "Unallowable operation: %s has incorrect dimensionality.", \
            #NAME); \
    CLEANUP; \
}


#define PS_ASSERT_PTRS_SIZE_EQUAL(PTR1, PTR2, RVAL) \
if (PTR1->n != PTR2->n) { \
    psError(PS_ERR_BAD_PARAMETER_SIZE, true, \
            "ptr %s has size %d, ptr %s has size %d.", \
            #PTR1, PTR1->n, #PTR2, PTR2->n); \
    return(RVAL); \
}

#define PS_ASSERT_PTR_TYPE_EQUAL(PTR1, PTR2, RVAL) PS_ASSERT_GENERAL_PTR_TYPE_EQUAL(PTR1, PTR2, return RVAL)
#define PS_ASSERT_GENERAL_PTR_TYPE_EQUAL(PTR1, PTR2, CLEANUP) \
if (PTR1->type.type != PTR2->type.type) { \
    psError(PS_ERR_BAD_PARAMETER_TYPE, true, \
            "ptr %s has type %d, ptr %s has type %d.", \
            #PTR1, PTR1->type.type, #PTR2, PTR2->type.type); \
    CLEANUP; \
}


/// @}
#endif

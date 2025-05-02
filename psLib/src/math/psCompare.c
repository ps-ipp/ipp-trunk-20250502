
/** @file psCompare.c
 *  @brief Comparison functions for sorting routines
 *  @ingroup Compare
 *
 *  @author Robert Lupton, Princeton University
 *  @author Robert Daniel DeSonia, MHPCC
 *
 *  @version $Revision: 1.9 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2007-01-09 22:38:52 $
 *
 *  Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "psCompare.h"

#define COMPARE_NUMERIC_PTR(TYPE) \
int psCompare##TYPE##Ptr(const void** a, const void** b) { \
    return **((ps##TYPE**)a) - **((ps##TYPE**)b); \
}

#define COMPARE_NUMERIC_PTR_DESCENDING(TYPE) \
int psCompareDescending##TYPE##Ptr(const void** a, const void** b) { \
    return **((ps##TYPE**)b) - **((ps##TYPE**)a); \
}

#define COMPARE_NUMERIC(TYPE) \
int psCompare##TYPE(const void* a, const void* b) { \
    return *((ps##TYPE*)a) - *((ps##TYPE*)b); \
}

#define COMPARE_NUMERIC_DESCENDING(TYPE) \
int psCompareDescending##TYPE(const void* a, const void* b) { \
    return *((ps##TYPE*)b) - *((ps##TYPE*)a); \
}

COMPARE_NUMERIC_PTR(S8)
COMPARE_NUMERIC_PTR(S16)
COMPARE_NUMERIC_PTR(S32)
COMPARE_NUMERIC_PTR(S64)
COMPARE_NUMERIC_PTR(U8)
COMPARE_NUMERIC_PTR(U16)
COMPARE_NUMERIC_PTR(U32)
COMPARE_NUMERIC_PTR(U64)

int psCompareF32Ptr(const void** a, const void** b)
{
    psF32 diff = **((psF32**)a) - **((psF32**)b);
    return (diff>FLT_EPSILON) ? 1 : ((diff<FLT_EPSILON) ? -1 :0);
}

int psCompareF64Ptr(const void** a, const void** b)
{
    psF64 diff = **((psF64**)a) - **((psF64**)b);
    return (diff>DBL_EPSILON) ? 1 : ((diff<DBL_EPSILON) ? -1 :0);
}

COMPARE_NUMERIC_PTR_DESCENDING(S8)
COMPARE_NUMERIC_PTR_DESCENDING(S16)
COMPARE_NUMERIC_PTR_DESCENDING(S32)
COMPARE_NUMERIC_PTR_DESCENDING(S64)
COMPARE_NUMERIC_PTR_DESCENDING(U8)
COMPARE_NUMERIC_PTR_DESCENDING(U16)
COMPARE_NUMERIC_PTR_DESCENDING(U32)
COMPARE_NUMERIC_PTR_DESCENDING(U64)

int psCompareDescendingF32Ptr(const void** a, const void** b)
{
    psF32 diff = **((psF32**)b) - **((psF32**)a);
    return (diff>FLT_EPSILON) ? 1 : ((diff<FLT_EPSILON) ? -1 :0);
}

int psCompareDescendingF64Ptr(const void** a, const void** b)
{
    psF64 diff = **((psF64**)b) - **((psF64**)a);
    return (diff>DBL_EPSILON) ? 1 : ((diff<DBL_EPSILON) ? -1 :0);
}

COMPARE_NUMERIC(S8)
COMPARE_NUMERIC(S16)
COMPARE_NUMERIC(S32)
COMPARE_NUMERIC(S64)
COMPARE_NUMERIC(U8)
COMPARE_NUMERIC(U16)
COMPARE_NUMERIC(U32)
COMPARE_NUMERIC(U64)

int psCompareF32(const void* a, const void* b)
{
    psF32 diff = *((psF32*)a) - *((psF32*)b);
    return (diff>FLT_EPSILON) ? 1 : ((diff<FLT_EPSILON) ? -1 :0);
}

int psCompareF64(const void* a, const void* b)
{
    psF64 diff = *((psF64*)a) - *((psF64*)b);
    return (diff>DBL_EPSILON) ? 1 : ((diff<DBL_EPSILON) ? -1 :0);
}

COMPARE_NUMERIC_DESCENDING(S8)
COMPARE_NUMERIC_DESCENDING(S16)
COMPARE_NUMERIC_DESCENDING(S32)
COMPARE_NUMERIC_DESCENDING(S64)
COMPARE_NUMERIC_DESCENDING(U8)
COMPARE_NUMERIC_DESCENDING(U16)
COMPARE_NUMERIC_DESCENDING(U32)
COMPARE_NUMERIC_DESCENDING(U64)

int psCompareDescendingF32(const void* a, const void* b)
{
    psF32 diff = *((psF32*)b) - *((psF32*)a);
    return (diff>FLT_EPSILON) ? 1 : ((diff<FLT_EPSILON) ? -1 :0);
}

int psCompareDescendingF64(const void* a, const void* b)
{
    psF64 diff = *((psF64*)b) - *((psF64*)a);
    return (diff>DBL_EPSILON) ? 1 : ((diff<DBL_EPSILON) ? -1 :0);
}

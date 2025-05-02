/** @file  psScalar.c
 *
 *  @brief Contains basic scalar definitions and operations
 *
 *  This file defines the basic type for a scalar struct and functions useful
 *  in manupulating scalars.
 * *
 *  @author Ross Harman, MHPCC
 *
 *  @version $Revision: 1.29 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2008-07-31 23:40:12 $
 *
 *  Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "psMemory.h"
#include "psError.h"
#include "psScalar.h"
#include "psLogMsg.h"
#include "psAbort.h"
#include "psAssert.h"


static void scalarFree(psScalar *scalar)
{
    // There are non dynamic allocated items
}

// XXX this function is badly designed for int types.  we should be using a var-arg syntax so
// we can actually pass the value correctly casted.
psScalar* p_psScalarAlloc(const char *file,
                          unsigned int lineno,
                          const char *func,
                          double value,
                          psElemType type)
{
    psScalar* scalar = NULL;

    // Create scalar
    scalar = (psScalar* ) p_psAlloc(file, lineno, func, sizeof(psScalar));
    psMemSetDeallocator(scalar, (psFreeFunc)scalarFree);
    scalar->type.dimen = PS_DIMEN_SCALAR;
    scalar->type.type = type;

    switch (type) {
    case PS_TYPE_S8:
        scalar->data.S8 = (psS8) value;
        break;
    case PS_TYPE_U8:
        scalar->data.U8 = (psU8) value;
        break;
    case PS_TYPE_S16:
        scalar->data.S16 = (psS16) value;
        break;
    case PS_TYPE_U16:
        scalar->data.U16 = (psU16) value;
        break;
    case PS_TYPE_S32:
        scalar->data.S32 = (psS32) value;
        break;
    case PS_TYPE_U32:
        scalar->data.U32 = (psU32) value;
        break;
    case PS_TYPE_S64:
        scalar->data.S64 = (psS64) value;
        break;
    case PS_TYPE_U64:
        scalar->data.U64 = (psU64) value;
        break;
    case PS_TYPE_F32:
        scalar->data.F32 = (psF32) value;
        break;
    case PS_TYPE_F64:
        scalar->data.F64 = (psF64) value;
        break;
    default:
        psError(PS_ERR_BAD_PARAMETER_TYPE, true,
                _("Specified datatype (%d) is unsupported by psScalar."),
                type);
        psFree(scalar);
        return NULL;
    }

    return scalar;
}



bool psMemCheckScalar(psPtr ptr)
{
    PS_ASSERT_PTR(ptr, NULL);
    return ( psMemGetDeallocator(ptr) == (psFreeFunc)scalarFree );
}


psScalar* p_psScalarCopy(const char *file,
                       unsigned int lineno,
                       const char *func,
                       const psScalar *value)
{
    psElemType dataType;
    psScalar *newScalar = NULL;

    if (value == NULL) {
        psError(PS_ERR_BAD_PARAMETER_NULL, true,
                _("Can not copy a NULL psScalar."));
        return NULL;
    }

    dataType = value->type.type;
    switch (dataType) {
    case PS_TYPE_S8:
        newScalar =  p_psScalarAlloc(file, lineno, func, value->data.S8, dataType);
        break;
    case PS_TYPE_U8:
        newScalar =  p_psScalarAlloc(file, lineno, func, value->data.U8, dataType);
        break;
    case PS_TYPE_S16:
        newScalar =  p_psScalarAlloc(file, lineno, func, value->data.S16, dataType);
        break;
    case PS_TYPE_U16:
        newScalar =  p_psScalarAlloc(file, lineno, func, value->data.U16, dataType);
        break;
    case PS_TYPE_S32:
        newScalar =  p_psScalarAlloc(file, lineno, func, value->data.S32, dataType);
        break;
    case PS_TYPE_U32:
        newScalar =  p_psScalarAlloc(file, lineno, func, value->data.U32, dataType);
        break;
    case PS_TYPE_S64:
        newScalar =  p_psScalarAlloc(file, lineno, func, value->data.S64, dataType);
        break;
    case PS_TYPE_U64:
        newScalar =  p_psScalarAlloc(file, lineno, func, value->data.U64, dataType);
        break;
    case PS_TYPE_F32:
        newScalar =  p_psScalarAlloc(file, lineno, func, value->data.F32, dataType);
        break;
    case PS_TYPE_F64:
        newScalar =  p_psScalarAlloc(file, lineno, func, value->data.F64, dataType);
        break;
    default:
        psError(PS_ERR_BAD_PARAMETER_TYPE, true,
                _("Specified datatype (%d) is unsupported by psScalar."),
                dataType);
        return NULL;
    }

    return newScalar;
}

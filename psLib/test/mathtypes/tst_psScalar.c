/** @file  tst_psScalar.c
 *
 *  @brief Contains the tests for psScalar.[ch]
 *
 *  @author Eric Van Alst, MHPCC
 *
 *  @version $Revision: 1.1 $
 *           $Name: not supported by cvs2svn $
 *  @date $Date: 2005-07-13 02:47:00 $
 *
 *  Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
 */

#include "psTest.h"
#include "pslib_strict.h"

static psS32 testScalarAlloc(void);
static psS32 testScalarCopy(void);

#define tstScalarAllocByType(datatype,value)                                                                    \
scalar = psScalarAlloc(value,PS_TYPE_##datatype);                                                               \
if(scalar == NULL) {                                                                                            \
    psError(PS_ERR_UNKNOWN,true,"psScalarAlloc returned NULL.");                                                \
    return 1+value;                                                                                             \
} else {                                                                                                        \
    if( (scalar->type.type != PS_TYPE_##datatype) && (scalar->data.datatype != value) ) {                       \
        psError(PS_ERR_UNKNOWN,true,"psScalarAlloc created object with unexpected type and/or value");          \
        return 2+value;                                                                                         \
    }                                                                                                           \
}                                                                                                               \
psFree(scalar);

#define tstScalarCopyByType(datatype,value)                                                                               \
scalarOrig = psScalarAlloc(value,PS_TYPE_##datatype);                                                                     \
scalarCopy = psScalarCopy(scalarOrig);                                                                                    \
if(scalarCopy == NULL) {                                                                                                  \
    psError(PS_ERR_UNKNOWN,true,"psScalarCopy returned NULL.");                                                           \
    return 3+value;                                                                                                       \
} else {                                                                                                                  \
    if( (scalarCopy->type.type != scalarOrig->type.type) || (scalarCopy->data.datatype != scalarOrig->data.datatype) ) {  \
        psError(PS_ERR_UNKNOWN,true,"psScalarCopy did not copy the original scalar by type and/or value");                \
        return 4+value;                                                                                                   \
    }                                                                                                                     \
}                                                                                                                         \
psFree(scalarCopy);                                                                                                       \
psFree(scalarOrig);

testDescription tests[] = {
                              {testScalarAlloc,783,"psScalarAlloc",0,false},
                              {testScalarCopy,784,"psScalarCopy",0,false},
                              {NULL}
                          };

psS32 main(psS32 argc, char* argv[])
{
    psLogSetLevel(PS_LOG_INFO);

    if ( ! runTestSuite(stderr,"psScalar",tests,argc,argv) ) {
        psError(PS_ERR_UNKNOWN,true,"One or more tests failed");
        return 1;
    }
    return 0;
}

psS32 testScalarAlloc(void)
{
    psScalar* scalar;

    psLogMsg(__func__,PS_LOG_INFO,"psScalarAlloc shall create scalar data objects");

    // Verify the proper allocation/deallocation of scalar objects of valid types
    tstScalarAllocByType(S8,10);
    tstScalarAllocByType(U8,12);
    tstScalarAllocByType(S16,14);
    tstScalarAllocByType(U16,16);
    tstScalarAllocByType(S32,18);
    tstScalarAllocByType(U32,20);
    tstScalarAllocByType(S64,22);
    tstScalarAllocByType(U64,24);
    tstScalarAllocByType(F32,26);
    tstScalarAllocByType(F64,28);
    tstScalarAllocByType(C32,30);
    tstScalarAllocByType(C64,32);

    // Verify return is null for invalid scalar type
    psLogMsg(__func__,PS_LOG_INFO,"Following should generate an error message");
    scalar = psScalarAlloc(true,PS_TYPE_BOOL);
    if(scalar != NULL) {
        psError(PS_ERR_UNKNOWN,true,"psScalarAlloc did not return null for invalid type");
        return 5;
    }

    return 0;
}

psS32 testScalarCopy(void)
{
    psScalar*  scalarOrig;
    psScalar*  scalarCopy;

    psLogMsg(__func__,PS_LOG_INFO,"psScalarCopy shall copy scalar objects");

    // Verify the proper copying of scalar objects for all valid types
    tstScalarCopyByType(S8,100);
    tstScalarCopyByType(U8,110);
    tstScalarCopyByType(S16,120);
    tstScalarCopyByType(U16,130);
    tstScalarCopyByType(S32,140);
    tstScalarCopyByType(U32,150);
    tstScalarCopyByType(S64,160);
    tstScalarCopyByType(U64,170);
    tstScalarCopyByType(F32,180);
    tstScalarCopyByType(F64,190);
    tstScalarCopyByType(C32,200);
    tstScalarCopyByType(C64,210);

    // Verify the return is null for invalid scalar type in the original
    scalarOrig = psScalarAlloc(0,PS_TYPE_S8);
    scalarOrig->type.type = PS_TYPE_BOOL;
    psLogMsg(__func__,PS_LOG_INFO,"Following should generate an error message");
    scalarCopy = psScalarCopy(scalarOrig);
    if(scalarCopy != NULL ) {
        psError(PS_ERR_UNKNOWN,true,"psScalarCopy did not return NULL for invalid type");
        return 6;
    }
    psFree(scalarOrig);

    // Verify the return is null for null original scalar value
    psLogMsg(__func__,PS_LOG_INFO,"Following should generate an error message");
    scalarCopy = psScalarCopy(NULL);
    if(scalarCopy != NULL) {
        psError(PS_ERR_UNKNOWN,true,"psScalarCopy did not return NULL for NULL argument");
        return 7;
    }

    return 0;
}


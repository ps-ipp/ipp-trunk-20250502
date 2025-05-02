/** @file  tst_psScalar.c
 *
 *  @brief Contains the tests for psScalar.[ch]
 *
 *  @author Eric Van Alst, MHPCC
 *
 *  @version $Revision: 1.5 $
 *           $Name: not supported by cvs2svn $
 *  @date $Date: 2007-04-10 21:09:30 $
 *
 *  Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
 */
#include <pslib.h>
#include "tap.h"
#include "pstap.h"


#define tstScalarAllocByType(datatype,value)            \
{                                                       \
    psScalar* scalar;                                   \
    scalar = psScalarAlloc(value,PS_TYPE_##datatype);   \
    ok(scalar != NULL, "psScalarAlloc successful");     \
    skip_start(scalar==NULL,1,"Skipping 1 test because psScalarAlloc failed");\
    ok(scalar->type.type==PS_TYPE_##datatype && scalar->data.datatype==value, \
       "scalar datatype and value are legit" );        \
    skip_end();                                         \
    psFree(scalar);                                     \
}


#define tstScalarCopyByType(datatype,value)                 \
{                                                           \
    psScalar*  scalarOrig;                                  \
    psScalar*  scalarCopy;                                  \
    scalarOrig = psScalarAlloc(value,PS_TYPE_##datatype);   \
    scalarCopy = psScalarCopy(scalarOrig);                  \
    ok(scalarCopy != NULL, "psScalarCopy != NULL");         \
    skip_start(scalarCopy==NULL,1,"Skipping 1 test since psScalarCopy failed");\
    ok(scalarCopy->type.type == scalarOrig->type.type && scalarCopy->data.datatype == scalarOrig->data.datatype, "Scalar datatype and value legit"); \
    skip_end();                                             \
    psFree(scalarCopy);                                     \
    psFree(scalarOrig);                                     \
}



int main(void)
{
    psLogSetFormat("HLNM");
    psLogSetLevel(PS_LOG_INFO);
    plan_tests(47);


    // Verify return is null for invalid scalar type
    {
        psMemId id = psMemGetId();
        psScalar* scalar = psScalarAlloc(true,PS_TYPE_BOOL);
        ok(scalar == NULL, "psScalarAlloc returns null for invalid type");
        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }


    //Verify the proper allocation/deallocation of scalar objects of valid types
    {
        psMemId id = psMemGetId();
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
        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }


    // Verify the return is null for invalid scalar type in the original
    {
        psMemId id = psMemGetId();
        psScalar* scalarOrig = psScalarAlloc(0,PS_TYPE_S8);
        scalarOrig->type.type = PS_TYPE_BOOL;
        psScalar* scalarCopy = psScalarCopy(scalarOrig);
        ok(scalarCopy == NULL, "psScalarCopy returns NULL for invalid type");
        scalarCopy = psScalarCopy(NULL);
        ok(scalarCopy == NULL, "psScalarCopy returns NULL for NULL argument");
        psFree(scalarOrig);
        psFree(scalarCopy);
        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }


    // Verify the proper copying of scalar objects for all valid types
    {
        psMemId id = psMemGetId();
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
        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }
}

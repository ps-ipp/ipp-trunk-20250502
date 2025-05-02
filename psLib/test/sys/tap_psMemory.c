/** @file  tst_psMemory.c
*
*  @brief Contains the tests for psMemory.[ch]
*
*  @author Robert DeSonia, MHPCC
*
*  @version $Revision: 1.8 $ $Name: not supported by cvs2svn $
*  @date $Date: 2007-05-01 00:08:52 $
*
*  XXXX: Several tests fail with an Abort and are commented out.
*
*  Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
*/

#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <limits.h>
#include <stdlib.h>


#include "pslib.h" // need to allow malloc for callback use
#include "tap.h"
#include "pstap.h"

static psS32 problemCallbackCalled = 0;
static psS32 allocCallbackCalled = 0;
static psS32 freeCallbackCalled = 0;
static psS32 exhaustedCallbackCalled = 0;

psMemId memAllocCallback( const psMemBlock *ptr );
psMemId memFreeCallback( const psMemBlock *ptr );
psS32 memCheckTypes( void );
void memProblemCallback( psMemBlock *ptr, const char *filename, unsigned int lineno );
psPtr TPOutOfMemoryExhaustedCallback( size_t size );


psS32 main(psS32 argc, char* argv[])
{
    psLogSetFormat("HLNM");
    psLogSetLevel(PS_LOG_INFO);
    plan_tests(54);

    // TPFreeReferencedMemory()
    {
        psMemId id = psMemGetId();
        psS32 *mem  = ( psS32* ) psAlloc( 100 * sizeof( psS32 ) );
        psS32 ref = psMemGetRefCounter( mem );
        ok(ref == 1, "buffer reference count %d.", ref );
        skip_start ( ref != 1, 3, "buffer reference count %d.", ref );
        psMemIncrRefCounter(mem);
        psMemIncrRefCounter(mem);
        psMemIncrRefCounter(mem);

        ref = psMemGetRefCounter( mem );
        ok(ref == 4, "buffer reference count was %d.", ref );
        skip_start ( ref != 4, 2, "buffer reference count was %d.", ref );

        psMemDecrRefCounter( mem );
        psMemDecrRefCounter( mem );

        ref = psMemGetRefCounter( mem );
        ok(ref == 2, "Found buffer reference count to be %d.", ref );
        skip_start ( ref != 2, 1, "Found buffer reference count to be %d.", ref );

        psMemDecrRefCounter( mem );
        ref = psMemGetRefCounter( mem );
        ok(ref == 1, "Found buffer reference count to be %d.", ref );
        skip_end();
        skip_end();
        skip_end();
        psFree(mem);
        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }


    // Upon requesting more memory than is available, psalloc shall call
    // the psMemExhaustedCallback.
    // XXXX: Skipping TPOutOfMemory() because of test abort failure
    skip_start (1, 2, "Skipping TPOutOfMemory() because of test abort failure");
    {
        psMemId id = psMemGetId();
        psS32 *mem[ 100 ];
        psMemExhaustedCallback cb;
        for ( psS32 lcv = 0; lcv < 100; lcv++ ) {
            mem[ lcv ] = NULL;
        }
        exhaustedCallbackCalled = 0;
        cb = psMemExhaustedCallbackSet( TPOutOfMemoryExhaustedCallback );
        // #ifdef COMMENTED_OUT
        // Don't include since intentionally aborts
        for ( psS32 lcv = 0; lcv < 100; lcv++ ) {
            mem[ lcv ] = ( psS32* ) psAlloc( SIZE_MAX/2 - 1000 );
        }
        psMemExhaustedCallbackSet( cb );
        ok(exhaustedCallbackCalled != 0,
             "Called psAlloc with HUGE memory requirement and survived!");
        for ( psS32 lcv = 0; lcv < 100; lcv++ ) {
            psFree( mem[ lcv ] );
        }
	// #endif
        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }
    skip_end();


    // Bug/Task #562 regression test.  Upon requesting more memory than is available,
    // psRealloc shall call the psMemExhaustedCallback.
    // XXXX: Skipping TPReallocOutOfMemory() because of test abort failure
    skip_start (1, 2, "Skipping TPReallocOutOfMemory() because of test abort failure");
    {
        psMemId id = psMemGetId();
        psS32 *mem[ 100 ];
        psMemExhaustedCallback cb;
        for ( psS32 lcv = 0; lcv < 100; lcv++ ) {
            mem[ lcv ] = NULL;
        }
        exhaustedCallbackCalled = 0;
        cb = psMemExhaustedCallbackSet( TPOutOfMemoryExhaustedCallback );
        for ( psS32 lcv = 0; lcv < 100; lcv++ ) {
            mem[ lcv ] = ( psS32* ) psAlloc( 10 );
        }
        for ( psS32 lcv = 0; lcv < 100; lcv++ ) {
            mem[ lcv ] = ( psS32* ) psRealloc( mem[ lcv ], SIZE_MAX/2 - 1000 );
        }
        psMemExhaustedCallbackSet( cb );
        ok(exhaustedCallbackCalled != 0,
             "Called psRealloc with HUGE memory requirement and survived in %s!", __func__ );
        for ( psS32 lcv = 0; lcv < 100; lcv++ ) {
            psFree( mem[ lcv ] );
        }
        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }
    skip_end();

    // psAlloc shall allocate memory blocks writeable by caller.
    {
        psMemId id = psMemGetId();
        const psS32 size = 100;
        psS32 *mem = ( psS32* ) psAlloc( size * sizeof( psS32 ) );
        ok(mem != NULL, "psAlloc returned non-NULL value" );
        for ( psS32 index = 0;index < size;index++ ) {
            mem[ index ] = index;
        }
        psS32 failed = 0;
        for ( psS32 index = 0;index < size;index++ ) {
            if ( mem[ index ] != index ) {
                failed++;
            }
        }
        ok(failed == 0, "mem legit" );
        psFree( mem );
        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }


    // psRealloc shall increase/decrease memory buffer while preserving contents
    {
        psMemId id = psMemGetId();
        const psS32 initialSize = 100;
        // allocate buffer with known values.
        psS32 *mem1 = ( psS32* ) psAlloc( initialSize * sizeof( psS32 ) );
        psS32 *mem2 = ( psS32* ) psAlloc( initialSize * sizeof( psS32 ) );
        psS32 *mem3 = ( psS32* ) psAlloc( initialSize * sizeof( psS32 ) );
        for ( psS32 lcv = 0;lcv < initialSize;lcv++ ) {
            mem1[lcv] = mem2[lcv] = mem3[lcv] = lcv;
        }
        psMemCheckCorruption(stderr, false);
        // realloc to 2x
        mem1 = ( psS32* ) psRealloc( mem1, 2 * initialSize * sizeof( psS32 ) );
        mem2 = ( psS32* ) psRealloc( mem2, 2 * initialSize * sizeof( psS32 ) );
        mem3 = ( psS32* ) psRealloc( mem3, 2 * initialSize * sizeof( psS32 ) );
        // check values of initial block
        int error = 0;
        for ( psS32 i = 0;i < initialSize;i++ ) {
            if ( mem1[ i ] != i || mem2[ i ] != i || mem3[ i ] != i ) {
                error = 1;
                break;
            }
        }
        ok(error==0, "Realloc preserve the contents with expanding buffer");
        psMemCheckCorruption(stderr, false);
        // realloc to 1/2 initial value.
        mem1 = ( psS32* ) psRealloc( mem1, ( initialSize / 2 ) * sizeof( psS32 ) );
        mem2 = ( psS32* ) psRealloc( mem2, ( initialSize / 2 ) * sizeof( psS32 ) );
        mem3 = ( psS32* ) psRealloc( mem3, ( initialSize / 2 ) * sizeof( psS32 ) );
        // check values of initial block
        error = 0;
        for ( psS32 i = 0;i < initialSize / 2;i++ ) {
            if ( mem1[ i ] != i || mem2[ i ] != i || mem3[ i ] != i ) {
                error = 1;
                break;
            }
        }
        ok(error==0, "Realloc preserved the contents with shrinking buffer");
        psFree( mem1 );
        psFree( mem2 );
        psFree( mem3 );
        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }


    // TPallocCallback()
    {
        psMemId id = psMemGetId();
        psS32 currentId = psMemGetId();
        const psS32 initialSize = 100;
        psS32 mark;
        allocCallbackCalled = 0;
        freeCallbackCalled = 0;
        psMemAllocCallbackSet( memAllocCallback );
        psMemFreeCallbackSet( memFreeCallback );
        psMemAllocCallbackSetID( currentId + 1 );
        psMemFreeCallbackSetID( currentId + 1 );
        // allocate buffer with known values.
        psS32 *mem1 = ( psS32* ) psAlloc( initialSize * sizeof( psS32 ) );
        psS32 *mem2 = ( psS32* ) psAlloc( initialSize * sizeof( psS32 ) );
        psS32 *mem3 = ( psS32* ) psAlloc( initialSize * sizeof( psS32 ) );
        psFree(mem1);
        psFree(mem2);
        psFree(mem3);
        ok(allocCallbackCalled == 2 && freeCallbackCalled == 2,
            "alloc/free callbacks called the proper number of times" );
        allocCallbackCalled = 0;
        freeCallbackCalled = 0;
        mark = psMemGetId();
        mem1 = ( psS32* ) psAlloc( initialSize * sizeof( psS32 ) );
        psMemAllocCallbackSetID( mark );
        mem1 = ( psS32* ) psRealloc( mem1, initialSize * 2 * sizeof( psS32 ) );
        psFree( mem1 );
        ok(allocCallbackCalled == 2,
             "realloc callbacks were called the proper number of times" );
        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }


    // TPcheckLeaks()
    // XXXX: Skipping TPcheckLeaks() because of test abort failure
    skip_start (1, 6, "Skipping TPcheckLeaks() because of test abort failure");
    {
        const psS32 numBuffers = 5;
        psS32* buffers[ 5 ];
        psS32 lcv;
        psS32 currentId = psMemGetId();
        psMemBlock** blks;
        psS32 nLeaks = 0;
        psS32 lineMark = 0;

        for ( lcv = 0;lcv < numBuffers;lcv++ ) {
            lineMark = __LINE__ + 1;
            buffers[ lcv ] = psAlloc( sizeof( psS32 ) );
        }
        for ( lcv = 1;lcv < numBuffers;lcv++ ) {
            psFree( buffers[ lcv ] );
        }
        nLeaks = psMemCheckLeaks( currentId, &blks, stderr, false );
        ok(nLeaks == 1, "psMemCheckLeaks found %d leaks", nLeaks );
        ok(blks[ 0 ] ->lineno == lineMark,
             "psMemCheckLeaks found a leak other than the expected one (line %d vs %d)", lineMark, blks[ 0 ] ->lineno );
        psFree( buffers[ 0 ] );
        psFree( blks );
        psMemCheckLeaks(currentId,NULL,stderr, false);
        for ( lcv = 0;lcv < numBuffers;lcv++ ) {
            lineMark = __LINE__ + 1;
            buffers[ lcv ] = psAlloc( sizeof( psS32 ) );
        }
        for ( lcv = 0;lcv < numBuffers - 1;lcv++ ) {
            psFree( buffers[ lcv ] );
        }
        nLeaks = psMemCheckLeaks( currentId, &blks, stderr, false );
        ok(nLeaks == 1, "psMemCheckLeaks found %d leaks.", nLeaks );
        ok(blks[ 0 ] ->lineno == lineMark, "psMemCheckLeaks found leaks");
        psFree( buffers[ 4 ] );
        psFree( blks );
        for ( lcv = 0;lcv < numBuffers;lcv++ ) {
            lineMark = __LINE__ + 1;
            buffers[ lcv ] = psAlloc( sizeof( psS32 ) );
        }
        for ( lcv = 0;lcv < numBuffers;lcv++ ) {
            if ( lcv % 2 == 0 ) {
                psFree( buffers[ lcv ] );
            }
        }
        nLeaks = psMemCheckLeaks( currentId, &blks, stderr, false );
        ok(nLeaks == 2, "psMemCheckLeaks found %d leaks.", nLeaks);
        ok(blks[ 0 ] ->lineno == lineMark,
             "psMemCheckLeaks found a leak other than the expected." );
        psFree(blks);
        psFree(buffers[1]);
        psFree(buffers[3]);
    }
    skip_end();


    void TPmultipleFree( void );
    // XXXX: Skipping TPmultipleFree() because of test abort failure
    if (0) {
        TPmultipleFree();
    }

    // memCheckTypes()
    if (1) {
        psMemId id = psMemGetId();
        psArray *negative = psArrayAlloc(2);
        psMetadata *neg = psMetadataAlloc();

        psArray *array = psArrayAlloc(100);
        int okay = psMemCheckType(PS_DATA_ARRAY,array);
        if (!okay) psFree(array);
        ok(okay, "psMemCheckType PS_DATA_ARRAY in memCheckType");

        ok(!psMemCheckType(PS_DATA_ARRAY, neg), "psMemCheckType PS_DATA_ARRAY with metadata input");
        psFree(array);

	// XXX EAM 2019.11.08 : this data type no longer exists
        // psBitSet *bits;
        // bits = psBitSetAlloc(100);
        // okay = psMemCheckType(PS_DATA_BITSET, bits);
        // if (!okay ) psFree(bits);
	// 
        // ok(okay, "psMemCheckBitSet in memCheckType");
        // ok(!psMemCheckType(PS_DATA_BITSET, negative), "psMemCheckType on psArray");
        // psFree(bits);

        psCube *cube;
        cube = psCubeAlloc();
        okay = psMemCheckType(PS_DATA_CUBE, cube);
        if (!okay ) psFree(cube);
        ok(okay, "psMemCheckCube in memCheckType");
        psFree(cube);

        psFits *fits;
        fits = psFitsOpen("test.fits","w");
        psImage* img = psImageAlloc(16,16,PS_TYPE_F32);
        psFitsWriteImage(fits,NULL,img,1,NULL);
        psFree(img);
        okay = psMemCheckType(PS_DATA_FITS, fits);
        if (!okay ) psFree(fits);
        ok(okay, "psMemCheckFits in memCheckType");
        psFitsClose(fits);

        psHash *hash;
        hash = psHashAlloc(100);
        okay = psMemCheckType(PS_DATA_HASH, hash);
        if (!okay ) psFree(hash);
        ok(okay, "psMemCheckHash in memCheckType");
        psFree(hash);

        psHistogram *histogram;
        histogram = psHistogramAlloc(1.1, 2.2, 2);
        okay = psMemCheckType(PS_DATA_HISTOGRAM, histogram);
        if (!okay ) psFree(histogram);
        ok(okay, "psMemCheckHistogram in memCheckType");
        psFree(histogram);

        psImage *image;
        image = psImageAlloc(5, 5, PS_TYPE_F32);
        okay = psMemCheckType(PS_DATA_IMAGE, image);
        if (!okay ) psFree(image);
        ok(okay, "psMemCheckImage in memCheckType");
        psFree(image);

        psKernel *kernel;
        kernel = psKernelAlloc(0, 1, 0, 1);
        okay = psMemCheckType(PS_DATA_KERNEL, kernel);
        if (!okay ) psFree(kernel);
        ok(okay, "psMemCheckKernel in memCheckType");
        psFree(kernel);

        psList *list;
        list = psListAlloc(NULL);
        okay = psMemCheckType(PS_DATA_LIST, list);
        if (!okay ) psFree(list);
        ok(okay, "psMemCheckList in memCheckType");
        psFree(list);

        psLookupTable *lookup;
        char *file = "tableF32.dat";
        char *format = "\%f \%lf \%d \%ld";
        lookup = psLookupTableAlloc(file, format, 10);
        okay = psMemCheckType(PS_DATA_LOOKUPTABLE, lookup);
        if (!okay ) psFree(lookup);
        ok(okay, "psMemCheckLookupTable in memCheckType");
        psFree(lookup);

        psMetadata *metadata;
        metadata = psMetadataAlloc();
        okay = psMemCheckType(PS_DATA_METADATA, metadata);
        if (!okay ) psFree(metadata);
        ok(okay, "psMemCheckMetadata in memCheckType");
        psFree(metadata);

        psMetadataItem *metaItem;
        metaItem = psMetadataItemAlloc("name", PS_DATA_S32, "COMMENT", 1);
        okay = psMemCheckType(PS_DATA_METADATAITEM, metaItem);
        if (!okay ) psFree(metaItem);
        ok(okay, "psMemCheckMetadataItem in memCheckType");
        psFree(metaItem);

        psMinimization *min;
        min = psMinimizationAlloc(3, 0.1, 1.0);
        okay = psMemCheckType(PS_DATA_MINIMIZATION, min);
        if (!okay ) psFree(min);
        ok(okay, "psMemCheckMinimization in memCheckType");
        psFree(min);

        psPixels *pixels;
        pixels = psPixelsAlloc(100);
        okay = psMemCheckType(PS_DATA_PIXELS, pixels);
        if (!okay ) psFree(pixels);
        ok(okay, "psMemCheckPixels in memCheckType");
        psFree(pixels);

        psPlane *plane;
        plane = psPlaneAlloc();
        okay = psMemCheckType(PS_DATA_PLANE, plane);
        if (!okay ) psFree(plane);
        ok(okay, "psMemCheckPlane in memCheckType.");
        psFree(plane);

        psPlaneDistort *planeDistort;
        planeDistort = psPlaneDistortAlloc(1, 1, 1, 1);
        okay =  psMemCheckType(PS_DATA_PLANEDISTORT, planeDistort);
        if (!okay ) psFree(planeDistort);
        ok(okay, "psMemCheckPlaneDistort in memCheckType.");
        psFree(planeDistort);

        psPlaneTransform *planeTransform;
        planeTransform = psPlaneTransformAlloc(1, 1);
        okay = psMemCheckType(PS_DATA_PLANETRANSFORM, planeTransform);
        if (!okay ) psFree(planeTransform);
        ok(okay, "psMemCheckPlaneTransform in memCheckType");
        psFree(planeTransform);
    
        psPolynomial1D *poly1;
        poly1 = psPolynomial1DAlloc(PS_POLYNOMIAL_ORD, 2);
        okay = psMemCheckType(PS_DATA_POLYNOMIAL1D, poly1);
        if (!okay ) psFree(poly1);
        ok(okay, "psMemCheckPolynomial1D in memCheckType");
        psFree(poly1);
    
        psPolynomial2D *poly2;
        poly2 = psPolynomial2DAlloc(PS_POLYNOMIAL_ORD, 2, 1);
        okay = psMemCheckType(PS_DATA_POLYNOMIAL2D, poly2);
        if (!okay ) psFree(poly2);
        ok(okay, "psMemCheckPolynomial2D in memCheckType");
        psFree(poly2);
    
        psPolynomial3D *poly3;
        poly3 = psPolynomial3DAlloc(PS_POLYNOMIAL_ORD, 2, 1, 1);
        okay = psMemCheckType(PS_DATA_POLYNOMIAL3D, poly3);
        if (!okay ) psFree(poly3);
        ok(okay, "psMemCheckPolynomial3D in memCheckType");
        psFree(poly3);
    
        psPolynomial4D *poly4;
        poly4 = psPolynomial4DAlloc(PS_POLYNOMIAL_ORD, 2, 1, 2, 1);
        okay = psMemCheckType(PS_DATA_POLYNOMIAL4D, poly4);
        if (!okay ) psFree(poly4);
        ok(okay, "psMemCheckPolynomial4D in memCheckType");
        psFree(poly4);
    
        psProjection *proj;
        proj = psProjectionAlloc(1, 1, 2.1, 2.1, PS_PROJ_TAN);
        okay = psMemCheckType(PS_DATA_PROJECTION, proj);
        if (!okay ) psFree(proj);
        ok(okay, "psMemCheckProjection in memCheckType.");
        psFree(proj);
    
        psScalar *scalar;
        psF64 f64 = 1.1;
        scalar = psScalarAlloc(f64, PS_TYPE_F64);
        okay = psMemCheckType(PS_DATA_SCALAR, scalar);
        if (!okay ) psFree(scalar);
        ok(okay, "psMemCheckScalar in memCheckType");
        psFree(scalar);
    
        psSphere *sphere;
        sphere = psSphereAlloc();
        okay = psMemCheckType(PS_DATA_SPHERE, sphere);
        if (!okay ) psFree(sphere);
        ok(okay, "psMemCheckSphere in memCheckType");
        psFree(sphere);
    
        psSphereRot *sphereRot;
        sphereRot = psSphereRotAlloc(0, 0, 20);
        okay = psMemCheckType(PS_DATA_SPHEREROT, sphereRot);
        if (!okay ) psFree(sphereRot);
        ok(okay, "psMemCheckSphereRot in memCheckType");
        psFree(sphereRot);
    
        psSpline1D *spline;
	// XXX API changed : 
        // spline = psSpline1DAlloc(2, 1, 0, 2);
        spline = psSpline1DAlloc();
        okay = psMemCheckType(PS_DATA_SPLINE1D, spline);
        if (!okay ) psFree(spline);
        ok(okay, "psMemCheckSpline1D in memCheckType");
        psFree(spline);
    
        psStats *stats;
        stats = psStatsAlloc(PS_STAT_MAX);
        okay = psMemCheckType(PS_DATA_STATS, stats);
        if (!okay ) psFree(stats);
        ok(okay, "psMemCheckStats in memCheckType");
        psFree(stats);
    
        psTime *time;
        time = psTimeAlloc(PS_TIME_UT1);
        okay = psMemCheckType(PS_DATA_TIME, time);
        if (!okay ) psFree(time);
        ok(okay, "psMemCheckTime in memCheckType");
        psFree(time);
    
        psVector *vector;
        vector = psVectorAlloc(100, PS_TYPE_F32);
        okay = psMemCheckType(PS_DATA_VECTOR, vector);
        ok(okay, "psMemCheckVector in memCheckType");
        psFree(vector);
        psFree(negative);
        psFree(neg);
        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }
}

#if 0
void TPmemCorruption( void )
{
    diag("TPmemCorruption");

    psS32 * buffer = NULL;
    psS32 oldValue = 0;
    psS32 corruptions = 0;
    psMemProblemCallback cb;

    buffer = psAlloc( sizeof( psS32 ) );

    // cause memory corruption via buffer underflow
    *buffer = 1;
    buffer--;
    oldValue = *buffer;
    *buffer = 2;

    problemCallbackCalled = 0;
    cb = psMemProblemCallbackSet( memProblemCallback );

    corruptions = psMemCheckCorruption( 0 );

    // restore the memory problem callback
    psMemProblemCallbackSet( cb );

    // restore the value, 'uncorrupting' the buffer
    *buffer = oldValue;
    buffer++;

    psFree( buffer );

    ok(corruptions == 1,
         "Expected one memory corruption but found %d", corruptions );
    ok(problemCallbackCalled == 1, "The memProblemCallback was invoked" );
}
#endif


void memProblemCallback( psMemBlock *ptr, const char *file, unsigned int lineno )
{
    problemCallbackCalled++;
}


psMemId memAllocCallback( const psMemBlock *ptr )
{
    allocCallbackCalled++;
    return 1;
}

psMemId memFreeCallback( const psMemBlock *ptr )
{
    freeCallbackCalled++;
    return 1;
}

psPtr TPOutOfMemoryExhaustedCallback( size_t size )
{
    exhaustedCallbackCalled++;
    return NULL;
}

void TPmultipleFree( void )
{
    psPtr buffer = psAlloc( 1024 );
    psPtr buffer2 = buffer;

    psFree( buffer );
    psFree( buffer2 );
}



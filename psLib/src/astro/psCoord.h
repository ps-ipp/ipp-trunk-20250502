/** @file  psCoord.h
 *
 *  @brief Basic coordinate transformations
 *
 *  This file defines the basic types for astronomical coordinate
 *  transformation
 *
 *  @author GLG, MHPCC
 *
 *  @version $Revision: 1.61 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2007-08-09 01:40:07 $
 *
 *  Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
 */

#ifndef PS_COORD_H
#define PS_COORD_H

/// @addtogroup Astro Astronomy
/// @{

#include "psType.h"
#include "psImage.h"
#include "psArray.h"
#include "psList.h"
#include "psPolynomial.h"
#include "psPixels.h"
#include "psRegion.h"
//#include "psTime.h"

/** Euclidiean Coordinate System.
 *
 *  Both detector and sky positions will be used extensively in the IPP. One
 *  coordinate system to be used is linear coordinates which conform to
 *  Euclidean geometry.
 *
 */
typedef struct
{
    double x;                          ///< x position
    double y;                          ///< y position
    double xErr;                       ///< Error in x position
    double yErr;                       ///< Error in y position
}
psPlane;

/** Angular Coordinate System
 *
 *  Both detector and sky positions will be used extensively in the IPP. One
 *  coordinate system to be used is angular coordinates for which additional
 *  care must often be taken in comparison to a euclidiean coordinate system.
 *
 */
typedef struct
{
    double r;                          ///< RA
    double d;                          ///< Dec
    double rErr;                       ///< Error in RA
    double dErr;                       ///< Error in Dec
}
psSphere;

/** 3-Dimensional Euclidean Coordinate System
 *
 *  Both detector and sky positions will be used extensively in the IPP. One
 *  coordinate system to be used is cubic coordinates for which additional
 *  care must often be taken in comparison to an angular coordinate system.
 *
 */
typedef struct
{
    double x;                          ///< cos (DEC) cos (RA)
    double y;                          ///< cos (DEC) sin (RA)
    double z;                          ///< sin (DEC)
    double xErr;                       ///< Error in x
    double yErr;                       ///< Error in y
    double zErr;                       ///< Error in z
}
psCube;

/** 2D Polynomial Transform
 *
 *  A transform between coordinate systems that consists simply of two 2D
 *  polynomials to transform both components - the output coordinates depend
 *  only on the input coordinates and no other quantities of objects at those
 *  coordinates.
 *
 */
typedef struct
{
    psPolynomial2D* x;                 ///< 2D polynomial transform of X coordinates
    psPolynomial2D* y;                 ///< 2D polynomial transform of Y coordinates
}
psPlaneTransform;

/** 4D Polynomial Transform
 *
 *  A transform between coordinate systems that consists of two 4D polynomials
 *  in which the output coordinates are also specified to be a function of the
 *  magnitude and color of the object with the given coordinates. This type of
 *  coordinate transformation is necessary to represent the (color-dependent)
 *  optical distortions caused by the atmosphere and camera optics, and the
 *  possibly effects of charge transfer inefficiency.
 *
 *  The lowest two terms are the x and y axis of the target system.  The higher
 *  two terms may represent magnitude and color terms.
 */
typedef struct
{
    psPolynomial4D* x;                 ///< 4D polynomial transform of X coordinates
    psPolynomial4D* y;                 ///< 4D polynomial transform of Y coordinates
}
psPlaneDistort;

/** Projection type for projection/deprojection
 *
 *  @see psProject, psDeproject
 *
 */
typedef enum {
    PS_PROJ_NONE,                       ///< No projection
    PS_PROJ_LIN,                        ///< Linear projection
    PS_PROJ_PLY,                        ///< Linear polynomial projection
    PS_PROJ_WRP,                        ///< Linear polynomial projection
    PS_PROJ_TAN,                        ///< Tangent projection
    PS_PROJ_DIS,                        ///< Sine projection
    PS_PROJ_SIN,                        ///< Sine projection
    PS_PROJ_STG,                        ///< Sine projection
    PS_PROJ_TNX,                        ///< Sine projection
    PS_PROJ_ZEA,                        ///< Sine projection
    PS_PROJ_ZPL,                        ///< Sine projection
    PS_PROJ_ZPN,                        ///< Sine projection
    PS_PROJ_AIT,                        ///< Aitoff projection
    PS_PROJ_PAR,                        ///< Par projection
    PS_PROJ_GLS,                        ///< GLS projection
    PS_PROJ_CAR,                        ///< CAR projection
    PS_PROJ_MER,                        ///< MER projection
    PS_PROJ_NTYPE                       ///< Number of types; must be last.
} psProjectionType;

/** Parameter set for projection/deprojection
 *
 *  @see psProject, psDeproject
 *
 */
typedef struct
{
    double R;                          ///< Coordinates of projection center
    double D;                          ///< Coordinates of projection center
    double Xs;                         ///< plate-scale in X direction
    double Ys;                         ///< plate-scale in Y direction
    psProjectionType type;             ///< Projection type
    psVector *radial;	       ///< radial distortion terms 
} psProjection;

/** Allocates a psPlane
 *
 *  @return psPlane*     resulting plane structure.
 */
psPlane* psPlaneAlloc(void) PS_ATTR_MALLOC;

/** Allocates a psSphere
 *
 *  @return psSphere*     resulting sphere structure.
 */
psSphere* psSphereAlloc(void) PS_ATTR_MALLOC;

/** Allocates a psCube
 *
 *  @return psCube*     resulting cubic structure.
 */
psCube* psCubeAlloc(void) PS_ATTR_MALLOC;

/** Allocates a psPlaneTransform transform.
 *
 *  @return psPlaneTransform*     resulting plane transform
 */

psPlaneTransform* psPlaneTransformAlloc(
    int order1,			      ///< The order of the x term in the transform.
    int order2,			      ///< The order of the y term in the transform.
    psPolynomialType type	      ///< The polynomial type (ORD or CHEB) for this transform
) PS_ATTR_MALLOC;

/** Applies the psPlaneTransform transform to a specified coordinate
 *
 *  @return psPlane*     resulting coordinate based on transform
 */
psPlane* psPlaneTransformApply(
    psPlane* out,                      ///< a psPlane to recycle.  If NULL, a new one is generated.
    const psPlaneTransform* transform, ///< the transform to apply
    const psPlane* coords              ///< the coordinate to apply the transform above.
);

/** Allocates a psPlaneDistort transform.
 *
 *  @return psPlaneDistort*     resulting plane distort transform
 */

psPlaneDistort* psPlaneDistortAlloc(
    int order1,                        ///< The order of the w term in the transform.
    int order2,                        ///< The order of the x term in the transform.
    int order3,                        ///< The order of the y term in the transform.
    int order4                         ///< The order of the z term in the transform.
) PS_ATTR_MALLOC;


/** Applies the psPlaneDistort transform to a specified coordinate
 *
 *  @return psPlane*     resulting coordinate based on transform
 */
psPlane* psPlaneDistortApply(
    psPlane* out,                      ///< a psPlane to recycle.  If NULL, a new one is generated.
    const psPlaneDistort* distort,     ///< the transform to apply
    const psPlane* coords,             ///< the coordinate to apply the transform above.
    float mag,                         ///< third term -- maybe magnitude
    float color                        ///< forth term -- maybe color
);

/** Allocates memory for a psProjection structure
 *
 *  @return psProjection*    psProjection structure
 */
psProjection* psProjectionAlloc(
    double R,                          ///< Right-ascension of projection center.
    double D,                          ///< Declination of projection center.
    double Xs,                         ///< Scale in x-dimension
    double Ys,                         ///< Scale in y-dimension
    psProjectionType type
) PS_ATTR_MALLOC;

/** Checks the type of a particular pointer.
 *
 *  Uses the appropriate deallocation function in psMemBlock to check the ptr datatype.
 *
 *  @return bool:       True if the pointer matches a psPlane structure, false otherwise.
 */
bool psMemCheckPlane(
    psPtr ptr                          ///< the pointer whose type to check
);

/** Checks the type of a particular pointer.
 *
 *  Uses the appropriate deallocation function in psMemBlock to check the ptr datatype.
 *
 *  @return bool:       True if the pointer matches a psSphere structure, false otherwise.
 */
bool psMemCheckSphere(
    psPtr ptr                          ///< the pointer whose type to check
);

/** Checks the type of a particular pointer.
 *
 *  Uses the appropriate deallocation function in psMemBlock to check the ptr datatype.
 *
 *  @return bool:       True if the pointer matches a psCube structure, false otherwise.
 */
bool psMemCheckCube(
    psPtr ptr                          ///< the pointer whose type to check
);

/** Checks the type of a particular pointer.
 *
 *  Uses the appropriate deallocation function in psMemBlock to check the ptr datatype.
 *
 *  @return bool:       True if the pointer matches a psPlaneTransform structure, false otherwise.
 */
bool psMemCheckPlaneTransform(
    psPtr ptr                          ///< the pointer whose type to check
);

/** Checks the type of a particular pointer.
 *
 *  Uses the appropriate deallocation function in psMemBlock to check the ptr datatype.
 *
 *  @return bool:       True if the pointer matches a psPlaneDistort structure, false otherwise.
 */
bool psMemCheckPlaneDistort(
    psPtr ptr                          ///< the pointer whose type to check
);

/** Checks the type of a particular pointer.
 *
 *  Uses the appropriate deallocation function in psMemBlock to check the ptr datatype.
 *
 *  @return bool:       True if the pointer matches a psProjection structure, false otherwise.
 */
bool psMemCheckProjection(
    psPtr ptr                          ///< the pointer whose type to check
);


/** Projects a spherical coordinate to a linear coordinate system
 *
 *  @return psPlane*    projected coordinate
 */
psPlane* psProject(
    psPlane *out,
    const psSphere* coord,             ///< coordinate to project
    const psProjection* projection     ///< parameters of the projection
);

/** Reverse projection of a linear coordinate to a spherical coordinate system
 *
 *  @return psPlane*    projected coordinate
 */
psSphere* psDeproject(
    psSphere *outSphere,
    const psPlane* coord,              ///< coordinate to project
    const psProjection* projection     ///< parameters of the projection
);

/** Takes a given transform and inverts it linearly if possible.
 *
 *  @return psPlaneTransform
 *  the linearly inverted transform
*/
psPlaneTransform *p_psPlaneTransformLinearInvert(
    psPlaneTransform *transform        ///< transform to invert
);

/** Takes a transform and tests whether or not it is a linear projection.
 *
 *  @return psS32
 *  the order of the projection
*/
bool p_psIsProjectionLinear(
    psPlaneTransform *transform        ///< transform to test for linearity
);


/** inverts a given transformation.
 *
 *  It may assume that the input transformation is one-to-one, and that the
 *  inverse transformation may be specified through using polynomials of the
 *  same type and order as the forward transformation. In the event that the
 *  input transformation is linear, an exact solution may be calculated;
 *  otherwise nSamples samples in each axis, covering the region specified by
 *  region shall be used as a grid to fit the best inverse transformation. The
 *  function shall return NULL if it was unable to generate the inverse
 *  transformation; otherwise it shall return the inverse transformation. In
 *  the event that out is NULL, a new psPlaneTransform shall be allocated and
 *  returned.
 *
 *  @return psPlaneTransform*  the resulting inverted transform
 */
psPlaneTransform* psPlaneTransformInvert(
    psPlaneTransform *out,             ///< a transform to recycle, or NULL if one is to be created.
    const psPlaneTransform *in,        ///< transform to invert
    psRegion region,                   ///< region to fit for non-linear transform inversion
    int nSamples,                       ///< number of samples in each axis for fit
    int extraOrders			///< increase the order of the output by the amount vs input, if input is non-linear
);

/** Creates a single transformation that has the effect of performing trans1
 *  followed by trans2.
 *
 *  psPlaneTransformCombine takes two transformations (trans1 and trans2) and
 *  returns a single transformation that has the effect of performing trans1
 *  followed by trans2. In the event that the input transformation is linear,
 *  an exact solution may be calculated; otherwise nSamples samples in each
 *  axis, covering the region specified by region shall be used as a grid to
 *  fit the best inverse transformation. The function shall return NULL if it
 *  was unable to generate the transformation; otherwise it shall return the
 *  transformation.
 *
 *  @return psPlaneTransform*    resulting transformation
 */
psPlaneTransform* psPlaneTransformCombine(
    psPlaneTransform *out,             ///< a transform to recycle, or NULL if one is to be created.
    const psPlaneTransform *trans1,    ///< first transform to combine
    const psPlaneTransform *trans2,    ///< first transform to combine
    psRegion region,                   ///< region to cover (for non-linear transforms)
    int nSamples                       ///< number of samples on each axis (for non-linear transforms)
);


/** takes two arrays containing matched coordinates and returns the
 *  best-fitting transformation.
 *
 *  psPlaneTransformFit takes two arrays containing matched coordinates (i.e.,
 *  coordinates in the source array correspond to the coordinates in the dest
 *  array) and returns the best-fitting transformation. The source and dest
 *  will contain psCoords. In the event that the number of coordinates in each
 *  is not identical, the function shall generate a warning, and extra
 *  coordinates in the longer of the two shall be ignored. The trans transform
 *  may not be NULL, since it specifies the desired order, polynomial type and
 *  any polynomial terms to mask. nRejIter rejection iterations shall be
 *  performed, wherein coordinates lying more than sigmaClip standard
 *  deviations from the fit shall be rejected.
 *
 *  @return bool        TRUE if successful, otherwise FALSE.
 */
bool psPlaneTransformFit(
    psPlaneTransform *trans,           ///< desired order, polynomial type, & polynomial mask terms
    const psArray *source,             ///< coordinates matching those in dest
    const psArray *dest,               ///< coordinates matching those in source
    int nRejIter,                      ///< number of rejection iterations to be performed
    float sigmaClip                    ///< coordinates above this number of standard deviations will be rejected
);
//XXX: need to add doxygen comments on the parameters above. -rdd

/** Converts from a 3-dimensional coordinate system to an angular coordinate system.
 *
 *  @return psSphere*       The former psCube in terms of a psSphere.
 */
psSphere *psCubeToSphere(
    const psCube *cube                 ///< psCube to convert
);

/** Converts from an angular coordinate system to a 3-dimensional coordinate system.
 *
 *  @return psCube*         The former psSphere in terms of a psCube.
 */
psCube *psSphereToCube(
    const psSphere *sphere             ///< psSphere to convert
);


/** Calculates the derivative of the specified psPlaneTransform with respect to x and y.
 *
 *  @return psPlane*         The derivative.
 */
psPlane *psPlaneTransformDeriv(
    psPlane *out,
    const psPlaneTransform *transformation,
    const psPlane *coord
);

/** Generates a list of pixels in the output coordinate frame that overlap the input
 *  pixels in the input coordinate frame through the specified transformation, inToOut.
 *
 *  psPixelsTransform is more complicated than simply transforming the input pixels,
 *  but requires the evaluation of the derivatives of the transformation in order to
 *  obtain the list of all pixels in the output coordinate frame that possibly overlap
 *  the pixel in the input coordinate frame.  In the event that input or inToOut are
 *  NULL, the function shall generate an error and return NULL.  If out is non-NULL it
 *  shall be modified and returned;  otherwise a new psPixels shall be allocated and
 *  returned.
 *
 *  @return psPixels*:      the list of overlapping pixels.
 */
psPixels *psPixelsTransform(
    psPixels *out,                     ///< output list of overlapping pixels
    const psPixels *input,             ///< input list of pixels
    const psPlaneTransform *inToOut    ///< specified transformation
);

psString psProjectTypeToString(psProjectionType type, const char *prefix);
psProjectionType psProjectTypeFromString(const char *name);


#define PS_PRINT_PLANE_TRANSFORM(NAME) \
{ \
    printf("---------------------- Plane Transform ----------------------\n"); \
    printf("x:\n"); \
    PS_POLY_PRINT_2D(NAME->x); \
    printf("y:\n"); \
    PS_POLY_PRINT_2D(NAME->y); \
} \

/// @}
#endif // #ifndef PS_COORD_H

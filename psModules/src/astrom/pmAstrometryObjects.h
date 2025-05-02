/* @file  pmAstrometryObjects.h
 * @brief basic matching of objects based on their astrometry.
 *
 * @author EAM, IfA
 *
 * @version $Revision: 1.18 $ $Name: not supported by cvs2svn $
 * @date $Date: 2008-11-20 01:26:07 $
 * Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
 */

#ifndef PM_ASTROMETRY_OBJECTS_H
#define PM_ASTROMETRY_OBJECTS_H

/// @addtogroup Astrometry
/// @{

/*
 *
 * This structure specifies the coordinate of the detection in each of the
 * four necessary coordinate frames: pix defines the position in the psReadout
 * frame, FP defines the position in the Focal Plane frame, TP defines the
 * position in the Tangent Plane frame, sky defines the position on the Celestial
 * Sphere. In addition, a measurement of the brightness is given by the element
 * Mag. Such a data structure should be used for both the raw and the reference
 * stars. In astrometric processing, the raw detections will be projected using
 * the best available information to each of these coordinate frames from the pix
 * coordinates, while the reference detections will be projected to the other
 * frames from the sky coordinates.
 *
 * XXX: There are more members here than in the SDRS.
 *
 */
typedef struct
{
    psPlane *pix;			///< the position in the pmReadout frame
    psPlane *cell;			///< the position in the pmCell frame
    psPlane *chip;			///< the position in the pmChip frame
    psPlane *FP;			///< the position in the pmFPA frame
    psPlane *TP;			///< the position in the tangent plane
    psSphere *sky;			///< the position on the Celestial Sphere.
    float Mag;				///< object magnitude in extracted filter
    float Color;			///< object color 
    float dMag;				///< error on object magnitude
    float SBinst;			///< surface brightness, used for Koppenhoefer correction
    float magCal;		        ///< object calibrated magnitude in extracted filter
}
pmAstromObj;

/*
 *
 * The pmAstromMatch structure defines the cross-correlation between two
 * arrays. A single such data item specifies that item number pmAstromMatch.idx1
 * in the first list corresponds to pmAstromMatch.idx2 in the second list.
 *
 */
typedef struct
{
    int raw;                             ///< reference to the rawstar entry
    int ref;                             ///< reference to the refstar entry
}
pmAstromMatch;


/*
 * The pmAstromMatchInfo structure is used to generate a unique set of matches
 */
typedef struct
{
    pmAstromMatch *match;		///< reference to the match
    float radius;			///< distance between the object
}
pmAstromMatchInfo;


/*
 *
 * XXX: Not in SDRS.
 *
 */
typedef struct
{
    psPlane center;                     ///<
    psPlane offset;                     ///<
    double  scale;                      ///<
    double  angle;                      ///<
    double  minMetric;                  ///<
    double  minVar;                     ///<
    int     nMatch;                     ///<
    int     nTest;                      ///<
    double  nSigma;                     ///<
}
pmAstromStats;

typedef struct
{
    psStats *xStats;
    psStats *yStats;
    int     nMatch;                     ///<
    double  nSigma;                     ///<
    double  dXsys;			///< systematic error in X
    double  dYsys;			///< systematic error in Y
    double  dXrange;			///< 10% - 90% range X residuals (unmasked, high S/N)
    double  dYrange;			///< 10% - 90% range Y residuals (unmasked, high S/N)
    double  dXstdev;			///< stdev of median residual in NxN bins
    double  dYstdev;			///< stdev of median residual in NxN bins
}
pmAstromFitResults;

/*
 *
 * If the two sets of coordinates are expected to agree very well (ie, the current best-guess
 * astrometric solution is quite close to reality), perform a match based on a simple radius
 * test. The following functions accept two sets of pmAstromObj sources and determines the
 * matched objects between the two lists using coordinates of the desired depth (depending on
 * the function). The input and reference sources must have been projected to the desired depth
 * (eg, for Focal Plane coordinates, to pmAstromObj.FP).  The specified radius must be in the
 * units for the matching depth (chip: pixels, focal plane: microns, tangent plane:
 * degrees. The output consists an array of pmAstromMatch values, defined above.
 *
 */
psArray *pmAstromRadiusMatch(
    const psArray *st1,
    const psArray *st2,
    double RADIUS
);
psArray *pmAstromRadiusMatchFP(
    const psArray *st1,
    const psArray *st2,
    double RADIUS
);
psArray *pmAstromRadiusMatchTP(
    const psArray *st1,
    const psArray *st2,
    double RADIUS
);
psArray *pmAstromRadiusMatchChip(
    const psArray *st1,
    const psArray *st2,
    double RADIUS
);

psArray *pmAstromRadiusMatchUniq (psArray *refstars, psArray *rawstars, psArray *matches);

pmAstromStats *pmAstromStatsAlloc(void);

/*
 *
 * This function accepts an array of pmAstromObj objects and rotates them by
 * the given angle about the given center coordinate pCenter,qCenter in the Focal
 * Plane Array coordinates.
 *
 * XXX: This differs from the SDRS
 *
 */
/* SDRS
psArray *pmAstromRotateObj(
    psArray *old,
    double angle,
    double pCenter,
    double qCenter
);
*/
psArray *pmAstromRotateObj(
    const psArray *old,
    psPlane center,
    double angle,
    double scale
);


/*
 *
 * If the two sets of coordinates are not known to agree well, but the
 * relative scale and approximate relative rotation is known, then a much faster
 * match can be found using pair-pair displacements. In such a case, the two
 * lists can be considered as having the same coordinate system, with an unknown
 * relative displacement. In this algorithm, all possible pair-wise differences
 * between the source positions in the two lists are constructed and accumulated
 * in a grid of possible offset values. The resulting grid is searched for a
 * cluster representing the offset between the two input lists. This algorithm
 * can only tolerate a small error in the relative scale or the relative rotation
 * of the two coordinate lists. However, this process is naturally O(N2), and is
 * thus advantageous over triangle matching in some circumstances. This process
 * can be extended to allow a larger uncertainty in the relative rotation by
 * allowing the procedure to scan over a range of rotations. We define the
 * following function to apply this matching algorithm:
 *
 * XXX: In the SDRS, this function is a pointer.
 *
 */
pmAstromStats *pmAstromGridMatch(
    const psArray *st1,
    const psArray *st2,
    const psMetadata *config
);

/******************************************************************************
pmAstromGridTweak(*raw, *ref, *recipe, stats): improve match for two star lists.
 ******************************************************************************/
pmAstromStats *pmAstromGridTweak(
    psArray *raw,
    psArray *ref,
    psMetadata *recipe,
    pmAstromStats *stats);

/*
 *
 * The result of a pmAstromGridMatch may be used to modify the astrometry
 * transformation information for a pmFPA image hierarchy structure. The result
 * of pmAstromGridMatch defines the adjustments which should be made to the
 * reference coordinate of the projection (pmFPA.projection.R,D) and the
 * effective rotation of the Focal Plane.  The rotation implies modification of
 * the linear terms of the pmFPA.toTangentPlane transformation. These two
 * adjustments are made using the function:
 *
 * XXX: This function name is different in the SDRS.
 *
 */
psPlaneTransform *pmAstromGridApply(
    psPlaneTransform *map,
    pmAstromStats *stat
);


/*
 *
 * This function is identical to pmAstromGridMatch, but is valid for only a
 * single relative rotation. The input config information need not contain any of
 * the GRID.*.ANGLE entries (they will be ignored).
 *
 * XXX: This function name is different in the SDRS.
 *
 */
/* in pmAstromGrid.c */
pmAstromStats *pmAstromGridAngle(
    const psArray *st1,
    const psArray *st2,
    const psMetadata *config);



/*
 *
 * This function accepts the raw and reference source lists and the list of
 * matched entries. It uses the matched list to determine a polynomial
 * transformation between the two coordinate systems. The fitting uses clipping
 * to exclude outliers, likely representing poor matches. The config element must
 * contain the information ASTROM.NSIGMA (specifying the number of sigma used in
 * the clipping) and ASTROM.NCLIP (specifying the number of clipping iterations
 * must be performed). The config element must also specify the order of the
 * polynomial fit (keyword: ASTROM.ORDER). The result of this fit is a set of
 * modifications of the components of the pmFPA.toTangentPlane transformation,
 * and the modifications of the reference coordinate of the projection
 * (pmFPA.projection.R,D) and the projection scale (pmFPA.projection.Xs,Ys). The
 * modifications to pmFPA.toTangentPlane incorporate the rotation component of
 * the linear terms and the higher-order terms of the polynomial fits.
 *
 * XXX: No prototype code.
 *
 */
bool pmAstromFitFPA(pmFPA *fpa,
                    psArray *st1,
                    psArray *st2,
                    psArray *match,
                    psMetadata *config);



/*
 *
 * This function accepts the raw and reference source lists for a single chip
 * and the list of matched entries. It uses the matched list to determine a
 * polynomial transformation between the two coordinate systems. The fitting
 * uses clipping to exclude outliers, likely representing poor matches. The
 * config element must contain the information ASTROM.NSIGMA
 *(specifying the number of sigma used in the clipping) and ASTROM.NCLIP
 *(specifying the number of clipping iterations must be performed). The config
 *element must also specify the order of the polynomial fit (keyword:
 *ASTROM.ORDER).  The result of this fit is a set of modifications of the
 *components of the pmChip.toFPA transformation.
 *
 * XXX: No prototype code.
 *
 */
bool pmAstromFitChip(
    pmFPA *fpa,
    psArray *st1,
    psArray *st2,
    psArray *match,
    psMetadata *config);


/*******************************************************************************
 The following functions and structs were in the prototype code, but not the
 SDRS.
 ******************************************************************************/
/*
 *
 *
 *
 *
 */

pmAstromFitResults *pmAstromFitResultsAlloc(void);

/*
 *
 * Allocates a pmAstromObj struct.
 *
 */
pmAstromObj *pmAstromObjAlloc (void);
/*
 * Is a given pointer a pmAstromObj?
 */
bool pmAstromObjTest(const psPtr ptr);


/*
 *
 * Copies a pmAstromObj struct.
 *
 */
pmAstromObj *pmAstromObjCopy(
    const pmAstromObj *old
);



/*
 *
 *
 *
 */
pmAstromMatch *pmAstromMatchAlloc(
    int i1,
    int i2
);




/*
 *
 *
 *
 */
pmAstromFitResults *pmAstromMatchFit(
    psPlaneTransform *map,
    psArray *raw,
    psArray *ref,
    psArray *match,
    psStats *stats,
    const psMetadata *config
);

/*
 *
 *
 *
 */
int pmAstromObjSortByMag(
    const void **a,
    const void **b
);

float pmAstromVectorRange (psVector *myVector, float minFrac, float maxFrac, float stdevGuess);

/// @}
#endif // PM_ASTROMETRY_OBJECTS_H

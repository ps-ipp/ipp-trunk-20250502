/* @file  pmDetrendDB.h
 * @brief Tools to query the detrend database system
 *
 * the functions in here do not perform the detrend database queries directly.  all interfaces
 * to the detrend database go through the external dettools functions.  this allows the modules
 * and directly dependent program to be sufficiently independent of the database schema that it
 * can be used with any properly defined detrend database tables.
 *
 * @author EAM, IfA
 *
 * @version $Revision: 1.16 $ $Name: not supported by cvs2svn $
 * @date $Date: 2008-10-27 00:23:15 $
 * Copyright 2004-2005 Institute for Astronomy, University of Hawaii
 */

#ifndef PM_DETREND_DB_H
#define PM_DETREND_DB_H

/// @addtogroup detrend Detrend Creation and Application
/// @{

#include <pslib.h>

#include "pmConfig.h"

typedef enum {
    PM_DETREND_TYPE_NONE,
    PM_DETREND_TYPE_MASK,
    PM_DETREND_TYPE_BIAS,
    PM_DETREND_TYPE_DARK,
    PM_DETREND_TYPE_FLAT,
    PM_DETREND_TYPE_FLATCORR,
    PM_DETREND_TYPE_SHUTTER,
    PM_DETREND_TYPE_FRINGE,
    PM_DETREND_TYPE_BACKGROUND,
    PM_DETREND_TYPE_ASTROM,
    PM_DETREND_TYPE_NOISEMAP,
    PM_DETREND_TYPE_VIDEOMASK,
    PM_DETREND_TYPE_VIDEODARK,
    PM_DETREND_TYPE_LINEARITY,
    PM_DETREND_TYPE_NEWNONLIN,
    PM_DETREND_TYPE_AUXMASK,
    PM_DETREND_TYPE_KH_CORRECT,
    PM_DETREND_TYPE_PATTERN_ROW_AMP,
    PM_DETREND_TYPE_PATTERN_DEAD_CELLS,
} pmDetrendType;

typedef struct {
    char *camera;                       // name of camera
    char *version;                      // optional version string
    char *filter;                       // name of filter
    char *dettype;                      // actual detrend type name
    float exptime;                      // exposure time (for dark, maybe flat & fringe)
    float airmass;                      // for fringe
    float dettemp;                      // for fringe
    float twilight;                     // hours (or seconds?) since/before nearest twilight
    psTime time;                        // time of input data
    pmDetrendType type;                 // type of detrend data

    bool  exptimeSet;
    bool  airmassSet;
    bool  dettempSet;
    bool  twilightSet;
} pmDetrendSelectOptions;

typedef struct {
    char *detID;                        // identifier of detrend run
    psString level;                     // level in FPA hierarchy of individual file
} pmDetrendSelectResults;

psString pmDetrendTypeToString(pmDetrendType type);

pmDetrendSelectOptions *pmDetrendSelectOptionsAlloc(const char *camera, psTime time, pmDetrendType type);
pmDetrendSelectResults *pmDetrendSelectResultsAlloc(void);
pmDetrendSelectResults *pmDetrendSelect (const pmDetrendSelectOptions *options, const pmConfig *config);
char *pmDetrendFile (const char *detID, const char *classID, const pmConfig *config);

/// @}
# endif

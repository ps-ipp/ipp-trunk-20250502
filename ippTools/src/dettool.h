/*
 * dettool.h
 *
 * Copyright (C) 2006  Joshua Hoblitt
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * program; see the file COPYING. If not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifndef DETTOOL_H
#define DETTOOL_H 1

#include "pxtools.h"

typedef enum {
    DETTOOL_MODE_NONE           = PXTOOL_MODE_NONE,
    DETTOOL_MODE_PENDING,
    DETTOOL_MODE_DEFINEBYTAG,
    DETTOOL_MODE_DEFINEBYQUERY,
    DETTOOL_MODE_DEFINEBYDETRUN,
    DETTOOL_MODE_MAKECORRECTION,
    DETTOOL_MODE_TOCORRECTEXP,
    DETTOOL_MODE_TOCORRECTIMFILE,
    DETTOOL_MODE_ADDCORRECTIMFILE,
    DETTOOL_MODE_RUNS,
    DETTOOL_MODE_CHILDLESSRUN,
    DETTOOL_MODE_INPUT,
    DETTOOL_MODE_RAW,

    DETTOOL_MODE_TOPROCESSEDIMFILE,
    DETTOOL_MODE_ADDPROCESSEDIMFILE,
    DETTOOL_MODE_PROCESSEDIMFILE,
    DETTOOL_MODE_REVERTPROCESSEDIMFILE,
    DETTOOL_MODE_UPDATEPROCESSEDIMFILE,
    DETTOOL_MODE_PENDINGCLEANUP_PROCESSEDIMFILE,
    DETTOOL_MODE_DONECLEANUP_PROCESSEDIMFILE,

    DETTOOL_MODE_TOPROCESSEDEXP,
    DETTOOL_MODE_ADDPROCESSEDEXP,
    DETTOOL_MODE_PROCESSEDEXP,
    DETTOOL_MODE_REVERTPROCESSEDEXP,
    DETTOOL_MODE_UPDATEPROCESSEDEXP,
    DETTOOL_MODE_PENDINGCLEANUP_PROCESSEDEXP,
    DETTOOL_MODE_DONECLEANUP_PROCESSEDEXP,
    DETTOOL_MODE_UPDATESTATE_PROCESSED,

    DETTOOL_MODE_TOSTACKED,
    DETTOOL_MODE_ADDSTACKED,
    DETTOOL_MODE_STACKED,
    DETTOOL_MODE_REVERTSTACKED,
    DETTOOL_MODE_UPDATESTACKED,
    DETTOOL_MODE_PENDINGCLEANUP_STACKED,
    DETTOOL_MODE_DONECLEANUP_STACKED,

    DETTOOL_MODE_TONORMALIZEDSTAT,
    DETTOOL_MODE_ADDNORMALIZEDSTAT,
    DETTOOL_MODE_NORMALIZEDSTAT,
    DETTOOL_MODE_REVERTNORMALIZEDSTAT,
    DETTOOL_MODE_UPDATENORMALIZEDSTAT,
    DETTOOL_MODE_PENDINGCLEANUP_NORMALIZEDSTAT,
    DETTOOL_MODE_DONECLEANUP_NORMALIZEDSTAT,

    DETTOOL_MODE_TONORMALIZE,
    DETTOOL_MODE_ADDNORMALIZEDIMFILE,
    DETTOOL_MODE_NORMALIZEDIMFILE,
    DETTOOL_MODE_REVERTNORMALIZEDIMFILE,
    DETTOOL_MODE_UPDATENORMALIZEDIMFILE,
    DETTOOL_MODE_PENDINGCLEANUP_NORMALIZEDIMFILE,
    DETTOOL_MODE_DONECLEANUP_NORMALIZEDIMFILE,

    DETTOOL_MODE_TONORMALIZEDEXP,
    DETTOOL_MODE_ADDNORMALIZEDEXP,
    DETTOOL_MODE_NORMALIZEDEXP,
    DETTOOL_MODE_REVERTNORMALIZEDEXP,
    DETTOOL_MODE_UPDATENORMALIZEDEXP,
    DETTOOL_MODE_PENDINGCLEANUP_NORMALIZEDEXP,
    DETTOOL_MODE_DONECLEANUP_NORMALIZEDEXP,

    DETTOOL_MODE_TORESIDIMFILE,
    DETTOOL_MODE_ADDRESIDIMFILE,
    DETTOOL_MODE_RESIDIMFILE,
    DETTOOL_MODE_REVERTRESIDIMFILE,
    DETTOOL_MODE_UPDATERESIDIMFILE,
    DETTOOL_MODE_PENDINGCLEANUP_RESIDIMFILE,
    DETTOOL_MODE_DONECLEANUP_RESIDIMFILE,

    DETTOOL_MODE_TORESIDEXP,
    DETTOOL_MODE_ADDRESIDEXP,
    DETTOOL_MODE_RESIDEXP,
    DETTOOL_MODE_REVERTRESIDEXP,
    DETTOOL_MODE_UPDATERESIDEXP,
    DETTOOL_MODE_PENDINGCLEANUP_RESIDEXP,
    DETTOOL_MODE_DONECLEANUP_RESIDEXP,
    DETTOOL_MODE_UPDATESTATE_RESID,

    DETTOOL_MODE_TODETRUNSUMMARY,
    DETTOOL_MODE_ADDDETRUNSUMMARY,
    DETTOOL_MODE_DETRUNSUMMARY,
    DETTOOL_MODE_REVERTDETRUNSUMMARY,
    DETTOOL_MODE_UPDATEDETRUNSUMMARY,
    DETTOOL_MODE_PENDINGCLEANUP_DETRUNSUMMARY,
    DETTOOL_MODE_UPDATEDETRUN,
    DETTOOL_MODE_RERUN,
    DETTOOL_MODE_REGISTER_DETREND,
    DETTOOL_MODE_REGISTER_DETREND_IMFILE,

    DETTOOL_MODE_EXPORTRUN,
    DETTOOL_MODE_IMPORTRUN
} dettoolMode;

pxConfig *dettoolConfig(pxConfig *config, int argc, char **argv);

// correct
bool makecorrectionMode(pxConfig *config);
bool tocorrectexpMode(pxConfig *config);
bool tocorrectimfileMode(pxConfig *config);
bool addcorrectimfileMode(pxConfig *config);

// register
bool register_detrend_imfileMode(pxConfig *config);

// processedimfile
bool toprocessedimfileMode(pxConfig *config);
bool addprocessedimfileMode(pxConfig *config);
bool processedimfileMode(pxConfig *config);
bool revertprocessedimfileMode(pxConfig *config);
bool updateprocessedimfileMode(pxConfig *config);
bool pendingcleanup_processedimfileMode(pxConfig *config);
bool donecleanup_processedimfileMode(pxConfig *config);

// processedexp
bool toprocessedexpMode(pxConfig *config);
bool addprocessedexpMode(pxConfig *config);
bool processedexpMode(pxConfig *config);
bool revertprocessedexpMode(pxConfig *config);
bool updateprocessedexpMode(pxConfig *config);
bool pendingcleanup_processedexpMode(pxConfig *config);
bool donecleanup_processedexpMode(pxConfig *config);
bool updatestateprocessedMode(pxConfig *config);

// stackedimfile
bool tostackedMode(pxConfig *config);
bool addstackedMode(pxConfig *config);
bool stackedMode(pxConfig *config);
bool revertstackedMode(pxConfig *config);
bool updatestackedMode(pxConfig *config);
bool pendingcleanup_stackedMode(pxConfig *config);
bool donecleanup_stackedMode(pxConfig *config);

// normalizedstat
bool tonormalizedstatMode(pxConfig *config);
bool addnormalizedstatMode(pxConfig *config);
bool normalizedstatMode(pxConfig *config);
bool revertnormalizedstatMode(pxConfig *config);
bool updatenormalizedstatMode(pxConfig *config);
bool pendingcleanup_normalizedstatMode(pxConfig *config);
bool donecleanup_normalizedstatMode(pxConfig *config);

// normalizedimfile
bool tonormalizeMode(pxConfig *config);
bool addnormalizedimfileMode(pxConfig *config);
bool normalizedimfileMode(pxConfig *config);
bool revertnormalizedimfileMode(pxConfig *config);
bool updatenormalizedimfileMode(pxConfig *config);
bool pendingcleanup_normalizedimfileMode(pxConfig *config);
bool donecleanup_normalizedimfileMode(pxConfig *config);

// normalizedexp
bool tonormalizedexpMode(pxConfig *config);
bool addnormalizedexpMode(pxConfig *config);
bool normalizedexpMode(pxConfig *config);
bool revertnormalizedexpMode(pxConfig *config);
bool updatenormalizedexpMode(pxConfig *config);
bool pendingcleanup_normalizedexpMode(pxConfig *config);
bool donecleanup_normalizedexpMode(pxConfig *config);

// residimfile
bool toresidimfileMode(pxConfig *config);
bool addresidimfileMode(pxConfig *config);
bool residimfileMode(pxConfig *config);
bool revertresidimfileMode(pxConfig *config);
bool updateresidimfileMode(pxConfig *config);
bool pendingcleanup_residimfileMode(pxConfig *config);
bool donecleanup_residimfileMode(pxConfig *config);

// residexp
bool toresidexpMode(pxConfig *config);
bool addresidexpMode(pxConfig *config);
bool residexpMode(pxConfig *config);
bool revertresidexpMode(pxConfig *config);
bool updateresidexpMode(pxConfig *config);
bool pendingcleanup_residexpMode(pxConfig *config);
bool donecleanup_residexpMode(pxConfig *config);
bool updatestateresidMode(pxConfig *config);

// detrunsummary
bool todetrunsummaryMode(pxConfig *config);
bool adddetrunsummaryMode(pxConfig *config);
bool detrunsummaryMode(pxConfig *config);
bool revertdetrunsummaryMode(pxConfig *config);
bool updatedetrunsummaryMode(pxConfig *config);
bool pendingcleanup_detrunsummaryMode(pxConfig *config);

// other utilities
bool startNewIteration(pxConfig *config, psS64 det_id);
bool setDetRunState(pxConfig *config, psS64 det_id, const char *state);
bool isValidDetRunState (const char *state);
bool isValidDataState (const char *data_state);
bool isValidMode(pxConfig *config, const char *mode);

// functions to set the 'data_state' for stages
bool setProcessedImfileDataState(pxConfig *config, psMetadata *where, const char *data_state);
bool setProcessedExpDataState(pxConfig *config, psMetadata *where, const char *data_state);
bool setResidImfileDataState(pxConfig *config, psMetadata *where, const char *data_state);
bool setResidExpDataState(pxConfig *config, psMetadata *where, const char *data_state);

bool setStackedImfileDataState(pxConfig *config, psS64 det_id, psS32 iteration, const char *class_id, const char *data_state);
bool setNormStatImfileDataState(pxConfig *config, psS64 det_id, psS32 iteration, const char *class_id, const char *data_state);
bool setNormImfileDataState(pxConfig *config, psS64 det_id, psS32 iteration, const char *class_id, const char *data_state);
bool setNormExpDataState(pxConfig *config, psS64 det_id, psS32 iteration, const char *data_state);

#endif // DETTOOL_H

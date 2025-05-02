/*
 * stacktool.h
 *
 * Copyright (C) 2007  Joshua Hoblitt
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

#ifndef STACKTOOL_H
#define STACKTOOL_H 1

#include "pxtools.h"

typedef enum {
    STACKTOOL_MODE_NONE           = 0x0,
    STACKTOOL_MODE_DEFINEBYQUERY,
    STACKTOOL_MODE_DEFINERUN,
    STACKTOOL_MODE_UPDATERUN,
    STACKTOOL_MODE_ADDINPUTSKYFILE,
    STACKTOOL_MODE_INPUTSKYFILE,
    STACKTOOL_MODE_TOSUM,
    STACKTOOL_MODE_TOBKG,
    STACKTOOL_MODE_ADDSUMSKYFILE,
    STACKTOOL_MODE_SUMSKYFILE,
    STACKTOOL_MODE_SASSSKYFILE,
    STACKTOOL_MODE_UPDATESASS,
    STACKTOOL_MODE_REVERTSUMSKYFILE,
    STACKTOOL_MODE_TOSUMMARY,
    STACKTOOL_MODE_ADDSUMMARY,
    STACKTOOL_MODE_REVERTSUMMARY,
    STACKTOOL_MODE_SUMMARY,
    STACKTOOL_MODE_PENDINGCLEANUPRUN,
    STACKTOOL_MODE_PENDINGCLEANUPSKYFILE,
    STACKTOOL_MODE_DONECLEANUP,
    STACKTOOL_MODE_UPDATESUMSKYFILE,
    STACKTOOL_MODE_EXPORTRUN,
    STACKTOOL_MODE_IMPORTRUN,
    STACKTOOL_MODE_IMPORTEXTERNAL,
    STACKTOOL_MODE_ADDEXTERNAL
} stacktoolMode;

pxConfig *stacktoolConfig(pxConfig *config, int argc, char **argv);

#endif // STACKTOOL_H

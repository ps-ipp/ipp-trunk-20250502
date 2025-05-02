/*
 * bgtool.h
 *
 * Copyright (C) 2006-2010  Joshua Hoblitt, Paul Price
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

#ifndef BGTOOL_H
#define BGTOOL_H 1

#include "pxtools.h"

typedef enum {
    BGTOOL_MODE_NONE           = 0x0,
    // Chip stage
    BGTOOL_MODE_DEFINECHIP,
    BGTOOL_MODE_UPDATECHIP,
    BGTOOL_MODE_TOCHIP,
    BGTOOL_MODE_CHIPINPUTS,
    BGTOOL_MODE_ADDCHIP,
    BGTOOL_MODE_CHIP,
    BGTOOL_MODE_ADVANCECHIP,
    BGTOOL_MODE_REVERTCHIP,
    BGTOOL_MODE_LISTCHIP,
    BGTOOL_MODE_UPDATECHIPIMFILE,
    // Warp stage
    BGTOOL_MODE_DEFINEWARP,
    BGTOOL_MODE_UPDATEWARP,
    BGTOOL_MODE_TOWARP,
    BGTOOL_MODE_WARPINPUTS,
    BGTOOL_MODE_ADDWARP,
    BGTOOL_MODE_WARP,
    BGTOOL_MODE_ADVANCEWARP,
    BGTOOL_MODE_REVERTWARP,
    BGTOOL_MODE_LISTWARP,
    BGTOOL_MODE_UPDATEWARPSKYFILE,
    // Cleanups
    BGTOOL_MODE_PENDINGCLEANUPCHIPRUN,
    BGTOOL_MODE_PENDINGCLEANUPCHIPIMFILE,
    BGTOOL_MODE_TOCLEANEDCHIPIMFILE,
    BGTOOL_MODE_PENDINGCLEANUPWARPRUN,
    BGTOOL_MODE_PENDINGCLEANUPWARPSKYFILE,
    BGTOOL_MODE_TOCLEANEDWARPSKYFILE,
    // Exporting
    BGTOOL_MODE_EXPORTCHIP,
    BGTOOL_MODE_IMPORTCHIP,
    BGTOOL_MODE_EXPORTWARP,
    BGTOOL_MODE_IMPORTWARP,
} bgtoolMode;

pxConfig *bgtoolConfig(pxConfig *config, int argc, char **argv);

#endif // BGTOOL_H

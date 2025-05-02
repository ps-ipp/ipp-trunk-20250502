/*
 * regtool.h
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

#ifndef REGTOOL_H
#define REGTOOL_H 1

#include "pxtools.h"

typedef enum {
    REGTOOL_MODE_NONE      = 0x0,
    REGTOOL_MODE_PENDINGIMFILE,
    REGTOOL_MODE_CHECKBURNTOOLIMFILE,
    REGTOOL_MODE_PENDINGBURNTOOLIMFILE,
    REGTOOL_MODE_ADDPROCESSEDIMFILE,
    REGTOOL_MODE_PROCESSEDIMFILE,
    REGTOOL_MODE_REVERTPROCESSEDIMFILE,
    REGTOOL_MODE_UPDATEPROCESSEDIMFILE,
    REGTOOL_MODE_PENDINGEXP,
    REGTOOL_MODE_ADDPROCESSEDEXP,
    REGTOOL_MODE_PROCESSEDEXP,
    REGTOOL_MODE_REVERTPROCESSEDEXP,
    REGTOOL_MODE_UPDATEPROCESSEDEXP,
    REGTOOL_MODE_CLEARDUPEXP,
    REGTOOL_MODE_UPDATEBYQUERY,
    REGTOOL_MODE_PENDINGCOMPRESSIMFILE,
    REGTOOL_MODE_FINISHCOMPRESSEXP,
    REGTOOL_MODE_CHECKSTATUS,
    REGTOOL_MODE_EXPORTRUN,
    REGTOOL_MODE_IMPORTRUN
} regtoolMode;

pxConfig *regtoolConfig(pxConfig *config, int argc, char **argv);

#endif // REGTOOL_H

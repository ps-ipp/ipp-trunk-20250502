/*
 * faketool.h
 *
 * Copyright (C) 2006-2008  Joshua Hoblitt
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

#ifndef FAKETOOL_H
#define FAKETOOL_H 1

#include "pxtools.h"

typedef enum {
    FAKETOOL_MODE_NONE      = 0x0,
    FAKETOOL_MODE_DEFINEBYQUERY,
    FAKETOOL_MODE_UPDATERUN,
    FAKETOOL_MODE_PENDINGIMFILE,
    FAKETOOL_MODE_PENDINGEXP,
    FAKETOOL_MODE_ADDPROCESSEDIMFILE,
    FAKETOOL_MODE_PROCESSEDIMFILE,
    FAKETOOL_MODE_REVERTPROCESSEDIMFILE,
    FAKETOOL_MODE_UPDATEPROCESSEDIMFILE,
    FAKETOOL_MODE_ADVANCEEXP,
    FAKETOOL_MODE_BLOCK,
    FAKETOOL_MODE_MASKED,
    FAKETOOL_MODE_UNMASKED,
    FAKETOOL_MODE_UNBLOCK,
    FAKETOOL_MODE_RETRYPROCESSEDIMFILE,
    FAKETOOL_MODE_PENDINGCLEANUPRUN,
    FAKETOOL_MODE_PENDINGCLEANUPIMFILE,
    FAKETOOL_MODE_DONECLEANUP,
    FAKETOOL_MODE_TOCLEANEDIMFILE,
    FAKETOOL_MODE_TOFULLIMFILE,
    FAKETOOL_MODE_TOPURGEDIMFILE,
    FAKETOOL_MODE_TOSCRUBBEDIMFILE,
    FAKETOOL_MODE_EXPORTRUN,
    FAKETOOL_MODE_IMPORTRUN
} FAKETOOLMode;

pxConfig *faketoolConfig(pxConfig *config, int argc, char **argv);

#endif // FAKETOOL_H

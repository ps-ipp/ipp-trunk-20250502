/*
 * chiptool.h
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

#ifndef CHIPTOOL_H
#define CHIPTOOL_H 1

#include "pxtools.h"

typedef enum {
    CHIPTOOL_MODE_NONE      = 0x0,
    CHIPTOOL_MODE_DEFINEBYQUERY,
    CHIPTOOL_MODE_DEFINECOPY,
    CHIPTOOL_MODE_UPDATERUN,
    CHIPTOOL_MODE_PENDINGIMFILE,
    CHIPTOOL_MODE_ADDPROCESSEDIMFILE,
    CHIPTOOL_MODE_PROCESSEDIMFILE,
    CHIPTOOL_MODE_REVERTPROCESSEDIMFILE,
    CHIPTOOL_MODE_UPDATEPROCESSEDIMFILE,
    CHIPTOOL_MODE_DROPPROCESSEDIMFILE,
    CHIPTOOL_MODE_SETIMFILETOUPDATE,
    CHIPTOOL_MODE_ADVANCEEXP,
    CHIPTOOL_MODE_BLOCK,
    CHIPTOOL_MODE_MASKED,
    CHIPTOOL_MODE_UNMASKED,
    CHIPTOOL_MODE_UNBLOCK,
    CHIPTOOL_MODE_RETRYPROCESSEDIMFILE,
    CHIPTOOL_MODE_PENDINGCLEANUPRUN,
    CHIPTOOL_MODE_PENDINGCLEANUPIMFILE,
    CHIPTOOL_MODE_REVERTCLEANUP,
    CHIPTOOL_MODE_DONECLEANUP,
    CHIPTOOL_MODE_RUN,
    CHIPTOOL_MODE_TOCLEANEDIMFILE,
    CHIPTOOL_MODE_TOFULLIMFILE,
    CHIPTOOL_MODE_TOPURGEDIMFILE,
    CHIPTOOL_MODE_TOSCRUBBEDIMFILE,
    CHIPTOOL_MODE_EXPORTRUN,
    CHIPTOOL_MODE_IMPORTRUN,
    CHIPTOOL_MODE_RUNSTATE,
    CHIPTOOL_MODE_LISTRUN
} chiptoolMode;

pxConfig *chiptoolConfig(pxConfig *config, int argc, char **argv);

#endif // CHIPTOOL_H

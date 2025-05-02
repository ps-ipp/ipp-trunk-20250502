/*
 * disttool.h
 *
 * Copyright (C) 2008 IfA
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

#ifndef DISTTOOL_H
#define DISTTOOL_H 1

#include "pxtools.h"

typedef enum {
    DISTTOOL_MODE_NONE      = 0x0,
    DISTTOOL_MODE_DEFINEBYQUERY,
    DISTTOOL_MODE_UPDATERUN,
    DISTTOOL_MODE_REVERTRUN,
    DISTTOOL_MODE_STARTOVER,
    DISTTOOL_MODE_PENDINGCOMPONENT,
    DISTTOOL_MODE_ADDPROCESSEDCOMPONENT,
    DISTTOOL_MODE_REVERTCOMPONENT,
    DISTTOOL_MODE_PROCESSEDCOMPONENT,
    DISTTOOL_MODE_UPDATEPROCESSEDCOMPONENT,
    DISTTOOL_MODE_TOADVANCE,
    DISTTOOL_MODE_PENDINGCLEANUP,
    DISTTOOL_MODE_PENDINGFILESET,
    DISTTOOL_MODE_ADDFILESET,
    DISTTOOL_MODE_REVERTFILESET,
    DISTTOOL_MODE_LISTFILESETS,
    DISTTOOL_MODE_UPDATEFILESET,
    DISTTOOL_MODE_QUEUERCRUN,
    DISTTOOL_MODE_UPDATERCRUN,
    DISTTOOL_MODE_REVERTRCRUN,
    DISTTOOL_MODE_PENDINGDEST,
    DISTTOOL_MODE_DEFINEDESTINATION,
    DISTTOOL_MODE_UPDATEDESTINATION,
    DISTTOOL_MODE_DEFINETARGET,
    DISTTOOL_MODE_UPDATETARGET,
    DISTTOOL_MODE_LISTTARGETS,
    DISTTOOL_MODE_DEFINEINTEREST,
    DISTTOOL_MODE_UPDATEINTEREST,
    DISTTOOL_MODE_LISTINTERESTS,
} disttoolMode;

pxConfig *disttoolConfig(pxConfig *config, int argc, char **argv);

#endif // DISTTOOL_H

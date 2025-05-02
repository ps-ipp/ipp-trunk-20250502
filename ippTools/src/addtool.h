/*
 * addtool.h
 *
 * Copyright (C) 2006  Joshua Hoblitt, Christopher Waters
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

#ifndef ADDTOOL_H
#define ADDTOOL_H 1

#include "pxtools.h"

typedef enum {
    ADDTOOL_MODE_NONE      = 0x0,
    ADDTOOL_MODE_DEFINEBYQUERY,
    ADDTOOL_MODE_UPDATERUN,
    ADDTOOL_MODE_PENDINGEXP,
    ADDTOOL_MODE_ADDPROCESSEDEXP,
    ADDTOOL_MODE_PROCESSEDEXP,
    ADDTOOL_MODE_REVERTPROCESSEDEXP,
    ADDTOOL_MODE_UPDATEPROCESSEDEXP,
    ADDTOOL_MODE_BLOCK,
    ADDTOOL_MODE_MASKED,
    ADDTOOL_MODE_UNBLOCK,
    ADDTOOL_MODE_PENDINGCLEANUPRUN,
    ADDTOOL_MODE_PENDINGCLEANUPEXP,
    ADDTOOL_MODE_DONECLEANUP,
    ADDTOOL_MODE_EXPORTRUN,
    ADDTOOL_MODE_IMPORTRUN,
    ADDTOOL_MODE_ADDMINIDVODBRUN,
    ADDTOOL_MODE_UPDATEMINIDVODBRUN,
    ADDTOOL_MODE_LISTMINIDVODBRUN,
    ADDTOOL_MODE_FLIPMINIDVODBRUN,
    ADDTOOL_MODE_CHECKMINIDVODBRUNADDRUN,
    ADDTOOL_MODE_ADDMINIDVODBPROCESSED,
    ADDTOOL_MODE_LISTMINIDVODBPROCESSED,
    ADDTOOL_MODE_REVERTMINIDVODBPROCESSED,
    ADDTOOL_MODE_UPDATEMINIDVODBPROCESSED
} addtoolMode;

pxConfig *addtoolConfig(pxConfig *config, int argc, char **argv);

#endif // ADDTOOL_H

/*
 * camtool.h
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

#ifndef CAMTOOL_H
#define CAMTOOL_H 1

#include "pxtools.h"

typedef enum {
    CAMTOOL_MODE_NONE      = 0x0,
    CAMTOOL_MODE_DEFINEBYQUERY,
    CAMTOOL_MODE_UPDATERUN,
    CAMTOOL_MODE_PENDINGEXP,
    CAMTOOL_MODE_PENDINGIMFILE,
    CAMTOOL_MODE_ADDPROCESSEDEXP,
    CAMTOOL_MODE_PROCESSEDEXP,
    CAMTOOL_MODE_REVERTPROCESSEDEXP,
    CAMTOOL_MODE_UPDATEPROCESSEDEXP,
    CAMTOOL_MODE_BLOCK,
    CAMTOOL_MODE_MASKED,
    CAMTOOL_MODE_UNBLOCK,
    CAMTOOL_MODE_PENDINGCLEANUPRUN,
    CAMTOOL_MODE_PENDINGCLEANUPEXP,
    CAMTOOL_MODE_DONECLEANUP,
    CAMTOOL_MODE_EXPORTRUN,
    CAMTOOL_MODE_IMPORTRUN
} camtoolMode;

pxConfig *camtoolConfig(pxConfig *config, int argc, char **argv);

#endif // CAMTOOL_H

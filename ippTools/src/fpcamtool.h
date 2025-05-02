/*
 * fpcamtool.h
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

#ifndef FPCAMTOOL_H
#define FPCAMTOOL_H 1

#include "pxtools.h"

typedef enum {
    FPCAMTOOL_MODE_NONE      = 0x0,
    FPCAMTOOL_MODE_DEFINEBYQUERY,
    FPCAMTOOL_MODE_UPDATERUN,
    FPCAMTOOL_MODE_PENDINGEXP,
    FPCAMTOOL_MODE_INPUTCHIPS,
    FPCAMTOOL_MODE_INPUTASTROM,
    FPCAMTOOL_MODE_ADDPROCESSEDEXP,
    FPCAMTOOL_MODE_PROCESSEDEXP,
    FPCAMTOOL_MODE_REVERTPROCESSEDEXP,
    FPCAMTOOL_MODE_UPDATEPROCESSEDEXP,
} fpcamtoolMode;

pxConfig *fpcamtoolConfig(pxConfig *config, int argc, char **argv);

#endif // FPCAMTOOL_H

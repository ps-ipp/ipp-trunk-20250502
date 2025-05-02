/*
 * vptool.h
 *
 * Copyright (C) 2011 University of Hawaii IFA 
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

#ifndef VPTOOL_H
#define VPTOOL_H 1

#include "pxtools.h"

typedef enum {
    VPTOOL_MODE_NONE      = 0x0,
    VPTOOL_MODE_DEFINEBYQUERY,
    VPTOOL_MODE_PENDINGRUN,
    VPTOOL_MODE_UPDATERUN,
    VPTOOL_MODE_REVERTRUN,
    VPTOOL_MODE_LISTRUN,
    VPTOOL_MODE_PENDINGIMFILE,
    VPTOOL_MODE_ADDPROCESSEDCELL,
    VPTOOL_MODE_PROCESSEDCELL,
} vptoolMode;

pxConfig *vptoolConfig(pxConfig *config, int argc, char **argv);

#endif // VPTOOL_H

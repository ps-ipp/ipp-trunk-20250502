/*
 * receivetool.h
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

#ifndef RECEIVETOOL_H
#define RECEIVETOOL_H 1

#include "pxtools.h"

typedef enum {
    RECEIVETOOL_MODE_NONE      = 0x0,
    RECEIVETOOL_MODE_DEFINESOURCE,
    RECEIVETOOL_MODE_LIST,
    RECEIVETOOL_MODE_ADDFILESET,
    RECEIVETOOL_MODE_UPDATELAST,
    RECEIVETOOL_MODE_PENDINGFILESET,
    RECEIVETOOL_MODE_UPDATEFILESET,
    RECEIVETOOL_MODE_TOADVANCE,
    RECEIVETOOL_MODE_ADDFILE,
    RECEIVETOOL_MODE_PENDINGFILE,
    RECEIVETOOL_MODE_ADDRESULT,
    RECEIVETOOL_MODE_REVERT,
} receivetoolMode;

pxConfig *receivetoolConfig(pxConfig *config, int argc, char **argv);

#endif // RECEIVETOOL_H

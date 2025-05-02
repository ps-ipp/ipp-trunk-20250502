/*
 * magicdstool.h
 *
 * Copyright (C) 2007  IfA
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

#ifndef MAGICDSTOOL_H
#define MAGICDSTOOL_H 1

#include "pxtools.h"

typedef enum {
    MAGICDSTOOL_MODE_NONE           = 0x0,
    MAGICDSTOOL_MODE_DEFINEBYQUERY,
    MAGICDSTOOL_MODE_DEFINECOPY,
    MAGICDSTOOL_MODE_UPDATERUN,
    MAGICDSTOOL_MODE_TODESTREAK,
    MAGICDSTOOL_MODE_ADDDESTREAKEDFILE,
    MAGICDSTOOL_MODE_ADVANCERUN,
    MAGICDSTOOL_MODE_REVERTDESTREAKEDFILE,
    MAGICDSTOOL_MODE_CLEARSTATEFAULTS,
    MAGICDSTOOL_MODE_GETSKYCELLS,
    MAGICDSTOOL_MODE_TOREMOVE,
    MAGICDSTOOL_MODE_TOREVERT,
    MAGICDSTOOL_MODE_COMPLETEDREVERT,
    MAGICDSTOOL_MODE_TOCLEANUP,
    MAGICDSTOOL_MODE_TOCLEANEDFILE,
    MAGICDSTOOL_MODE_TOFULLFILE,
    MAGICDSTOOL_MODE_SETFILETOUPDATE,
    MAGICDSTOOL_MODE_UPDATEDESTREAKEDFILE,
    MAGICDSTOOL_MODE_LISTRUN,
    MAGICDSTOOL_MODE_DESTREAKEDFILE,
} MAGICDStoolMode;

pxConfig *magicdstoolConfig(pxConfig *config, int argc, char **argv);

#endif // MAGICDSTOOL_H

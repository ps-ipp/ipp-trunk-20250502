/*
 * magictool.h
 *
 * Copyright (C) 2007  Joshua Hoblitt
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

#ifndef MAGICTOOL_H
#define MAGICTOOL_H 1

#include "pxtools.h"

typedef enum {
    MAGICTOOL_MODE_NONE           = 0x0,
    MAGICTOOL_MODE_DEFINEBYQUERY,
    MAGICTOOL_MODE_DEFINERUN,
    MAGICTOOL_MODE_UPDATERUN,
    MAGICTOOL_MODE_ADDINPUTSKYFILE,
    MAGICTOOL_MODE_INPUTSKYFILE,
    MAGICTOOL_MODE_TOTREE,
    MAGICTOOL_MODE_INPUTTREE,
    MAGICTOOL_MODE_REVERTTREE,
    MAGICTOOL_MODE_TOPROCESS,
    MAGICTOOL_MODE_INPUTS,
    MAGICTOOL_MODE_ADDRESULT,
    MAGICTOOL_MODE_REVERTNODE,
    MAGICTOOL_MODE_TOMASK,
    MAGICTOOL_MODE_ADDMASK,
    MAGICTOOL_MODE_REVERTMASK,
    MAGICTOOL_MODE_MASK,
    MAGICTOOL_MODE_CENSORRUN,
    MAGICTOOL_MODE_EXPOSURE,
    MAGICTOOL_MODE_SETGOTOCLEANED,
    MAGICTOOL_MODE_TOCLEANUP,
    MAGICTOOL_MODE_SETWORKDIRSTATE,
} MAGICtoolMode;

pxConfig *magictoolConfig(pxConfig *config, int argc, char **argv);

#endif // MAGICTOOL_H

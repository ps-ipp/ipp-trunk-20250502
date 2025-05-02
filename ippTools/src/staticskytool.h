/*
 * staticskytool.h
 *
 * Copyright (C) 2007,2010  Joshua Hoblitt, Paul Price
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

#ifndef STATICSKYTOOL_H
#define STATICSKYTOOL_H 1

#include "pxtools.h"

typedef enum {
    STATICSKYTOOL_MODE_NONE           = 0x0,
    STATICSKYTOOL_MODE_DEFINEBYQUERY,
    STATICSKYTOOL_MODE_UPDATERUN,
    STATICSKYTOOL_MODE_INPUTS,
    STATICSKYTOOL_MODE_TODO,
    STATICSKYTOOL_MODE_ADDRESULT,
    STATICSKYTOOL_MODE_RESULT,
    STATICSKYTOOL_MODE_REVERT,
    STATICSKYTOOL_MODE_UPDATERESULT,
    STATICSKYTOOL_MODE_EXPORTRUN,
    STATICSKYTOOL_MODE_IMPORTRUN,

    STATICSKYTOOL_MODE_DEFINESKYCALRUN,
    STATICSKYTOOL_MODE_PENDINGSKYCALRUN,
    STATICSKYTOOL_MODE_UPDATESKYCALRUN,
    STATICSKYTOOL_MODE_ADDSKYCALRESULT,
    STATICSKYTOOL_MODE_UPDATESKYCALRESULT,
    STATICSKYTOOL_MODE_SKYCALRESULT,
    STATICSKYTOOL_MODE_REVERTSKYCALRESULT,
    STATICSKYTOOL_MODE_EXPORTSKYCALRUN,
    STATICSKYTOOL_MODE_IMPORTSKYCALRUN,
} staticskytoolMode;

pxConfig *staticskytoolConfig(pxConfig *config, int argc, char **argv);

#endif // STATICSKYTOOL_H

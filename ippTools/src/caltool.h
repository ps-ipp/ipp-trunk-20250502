/*
 * caltool.h
 *
 * Copyright (C) 2006-2007  Joshua Hoblitt
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

#ifndef CALTOOL_H
#define CALTOOL_H 1

#include "pxtools.h"

typedef enum {
    CALTOOL_MODE_NONE           = PXTOOL_MODE_NONE,
    CALTOOL_MODE_DBS,
    CALTOOL_MODE_ADDDB,
    CALTOOL_MODE_ADDRUN,
    CALTOOL_MODE_RUNS
} caltoolMode;

pxConfig *caltoolConfig(pxConfig *config, int argc, char **argv);

#endif // CALTOOL_H

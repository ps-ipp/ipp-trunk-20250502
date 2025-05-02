/*
 * flatcorr.h
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

#ifndef FLATCORR_H
#define FLATCORR_H 1

#include "pxtools.h"

typedef enum {
    FLATCORR_MODE_NONE           = PXTOOL_MODE_NONE,
    FLATCORR_MODE_DEFINEBYQUERY,
    FLATCORR_MODE_DEFINERUN,
    FLATCORR_MODE_ADDCHIP,
    FLATCORR_MODE_DROPCHIP,
    FLATCORR_MODE_ADDCAMERA,
    FLATCORR_MODE_DROPCAMERA,
    FLATCORR_MODE_ADVANCECAMERA,
    FLATCORR_MODE_ADVANCEADDSTAR,
    FLATCORR_MODE_PENDINGPROCESS,
    FLATCORR_MODE_ADDPROCESS,
    FLATCORR_MODE_UPDATERUN,
    FLATCORR_MODE_INPUTEXP,
    FLATCORR_MODE_INPUTIMFILE,
} flatcorrMode;

pxConfig *flatcorrConfig(pxConfig *config, int argc, char **argv);

#endif // FLATCORR_H

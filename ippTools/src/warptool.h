/*
 * warptool.h
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

#ifndef WARPTOOL_H
#define WARPTOOL_H 1

#include "pxtools.h"

typedef enum {
    WARPTOOL_MODE_NONE           = 0x0,
    WARPTOOL_MODE_DEFINEBYQUERY,
    WARPTOOL_MODE_DEFINERUN,
    WARPTOOL_MODE_RUNONE,
    WARPTOOL_MODE_UPDATERUN,
    WARPTOOL_MODE_ADDINPUTEXP,
    WARPTOOL_MODE_EXP,
    WARPTOOL_MODE_IMFILE,
    WARPTOOL_MODE_TOOVERLAP,
    WARPTOOL_MODE_ADDOVERLAP,
    WARPTOOL_MODE_REVERTOVERLAP,
    WARPTOOL_MODE_SCMAP,
    WARPTOOL_MODE_TOWARPED,
    WARPTOOL_MODE_ADDWARPED,
    WARPTOOL_MODE_ADVANCERUN,
    WARPTOOL_MODE_WARPED,
    WARPTOOL_MODE_REVERTWARPED,
    WARPTOOL_MODE_BLOCK,
    WARPTOOL_MODE_MASKED,
    WARPTOOL_MODE_UNBLOCK,
    WARPTOOL_MODE_TOSUMMARY,
    WARPTOOL_MODE_ADDSUMMARY,
    WARPTOOL_MODE_PENDINGCLEANUPRUN,
    WARPTOOL_MODE_PENDINGCLEANUPSKYFILE,
    WARPTOOL_MODE_REVERTCLEANUP,
    WARPTOOL_MODE_DONECLEANUP,
    WARPTOOL_MODE_TOCLEANEDSKYFILE,
    WARPTOOL_MODE_TOPURGEDSKYFILE,
    WARPTOOL_MODE_TOSCRUBBEDSKYFILE,
    WARPTOOL_MODE_TOFULLSKYFILE,
    WARPTOOL_MODE_UPDATESKYFILE,
    WARPTOOL_MODE_EXPORTRUN,
    WARPTOOL_MODE_IMPORTRUN,
    WARPTOOL_MODE_RUNSTATE,
    WARPTOOL_MODE_LISTRUN,
    WARPTOOL_MODE_SETSKYFILETOUPDATE,
} warptoolMode;

pxConfig *warptoolConfig(pxConfig *config, int argc, char **argv);

#endif // WARPTOOL_H

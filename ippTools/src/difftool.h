/*
 * difftool.h
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

#ifndef DIFFTOOL_H
#define DIFFTOOL_H 1

#include "pxtools.h"

typedef enum {
    DIFFTOOL_MODE_NONE           = 0x0,
    DIFFTOOL_MODE_DEFINERUN,
    DIFFTOOL_MODE_UPDATERUN,
    DIFFTOOL_MODE_ADDINPUTSKYFILE,
    DIFFTOOL_MODE_INPUTSKYFILE,
    DIFFTOOL_MODE_TODIFFSKYFILE,
    DIFFTOOL_MODE_ADDDIFFSKYFILE,
    DIFFTOOL_MODE_ADVANCE,
    DIFFTOOL_MODE_TOSUMMARY,
    DIFFTOOL_MODE_ADDSUMMARY,
    DIFFTOOL_MODE_DIFFSKYFILE,
    DIFFTOOL_MODE_REVERTDIFFSKYFILE,
    DIFFTOOL_MODE_DEFINEPOPRUN,
    DIFFTOOL_MODE_DEFINEWARPSTACK,
    DIFFTOOL_MODE_DEFINEWARPWARP,
    DIFFTOOL_MODE_DEFINESTACKSTACK,
    DIFFTOOL_MODE_PENDINGCLEANUPRUN,
    DIFFTOOL_MODE_PENDINGCLEANUPSKYFILE,
    DIFFTOOL_MODE_REVERTCLEANUP,
    DIFFTOOL_MODE_DONECLEANUP,
    DIFFTOOL_MODE_UPDATEDIFFSKYFILE,
    DIFFTOOL_MODE_EXPORTRUN,
    DIFFTOOL_MODE_IMPORTRUN,
    DIFFTOOL_MODE_TOCLEANEDSKYFILE,
    DIFFTOOL_MODE_TOPURGEDSKYFILE,
    DIFFTOOL_MODE_TOSCRUBBEDSKYFILE,
    DIFFTOOL_MODE_TOFULLSKYFILE,
    DIFFTOOL_MODE_LISTRUN,
    DIFFTOOL_MODE_LISTSSRUN,
    DIFFTOOL_MODE_SETSKYFILETOUPDATE,
    //    DIFFTOOL_MODE_DEFINEWARPSTACKOLDMETHOD,
} difftoolMode;

pxConfig *difftoolConfig(pxConfig *config, int argc, char **argv);

#endif // DIFFTOOL_H

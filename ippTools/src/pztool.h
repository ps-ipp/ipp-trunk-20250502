/*
 * pztool.h
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

#ifndef PZTOOL_H
#define PZTOOL_H 1

#include "pxtools.h"

typedef enum {
    PZTOOL_MODE_NONE      = 0x0,
    PZTOOL_MODE_ADDDATASTORE,
    PZTOOL_MODE_DATASTORE,
    PZTOOL_MODE_SEEN,
    PZTOOL_MODE_PENDINGEXP,
    PZTOOL_MODE_PENDINGIMFILE,
    PZTOOL_MODE_COPYDONE,
    PZTOOL_MODE_COPIED,
    PZTOOL_MODE_UPDATECOPIED,
    PZTOOL_MODE_REVERTCOPIED,
    PZTOOL_MODE_CLEARCOMMONFAULTS,
    PZTOOL_MODE_TOADVANCE,
    PZTOOL_MODE_ADVANCE,
    PZTOOL_MODE_UPDATEPZEXP,
    PZTOOL_MODE_UPDATENEWEXP,
    PZTOOL_MODE_UPDATESUMMITEXP,
} pztoolMode;

pxConfig *pztoolConfig(pxConfig *config, int argc, char **argv);

#endif // PZTOOL_H

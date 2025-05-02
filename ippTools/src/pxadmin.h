/*
 * pxadmin.h
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

#ifndef PXADMIN_H
#define PXADMIN_H 1

#include "pxtools.h"

typedef enum {
    PXADMIN_MODE_NONE      = 0x0,
    PXADMIN_MODE_CREATE,
    PXADMIN_MODE_CREATE_MIRROR,
    PXADMIN_MODE_DELETE,
    PXADMIN_MODE_RECREATE
} pxAdminMode;

pxConfig *pxAdminConfig (pxConfig *config, int argc, char **argv);

#endif // PXADMIN_H

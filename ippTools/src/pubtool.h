/*
 * pubtool.h
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

#ifndef PUBTOOL_H
#define PUBTOOL_H 1

#include "pxtools.h"

typedef enum {
    PUBTOOL_MODE_NONE      = 0x0,
    PUBTOOL_MODE_DEFINECLIENT,
    PUBTOOL_MODE_UPDATECLIENT,
    PUBTOOL_MODE_DEFINERUN,
    PUBTOOL_MODE_PENDING,
    PUBTOOL_MODE_ADD,
    PUBTOOL_MODE_REVERT,
    PUBTOOL_MODE_UPDATERUN,
} pubtoolMode;

pxConfig *pubtoolConfig(pxConfig *config, int argc, char **argv);

#endif // PUBTOOL_H

/*
 * labeltool.h
 *
 * Copyright (C) 2010  IfA
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

#ifndef LABELTOOL_H
#define LABELTOOL_H 1

#include "pxtools.h"

typedef enum {
    LABELTOOL_MODE_NONE           = 0x0,
    LABELTOOL_MODE_DEFINELABEL,
    LABELTOOL_MODE_UPDATELABEL,
    LABELTOOL_MODE_DELETELABEL,
    LABELTOOL_MODE_LISTLABEL,
} labeltoolMode;

pxConfig *labeltoolConfig(pxConfig *config, int argc, char **argv);

#endif // LABELTOOL_H

/*
 * sctool.h
 *
 * Copyright (C) 2012 University of Hawaii IFA 
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

#ifndef SCTOOL_H
#define SCTOOL_H 1

#include "pxtools.h"

typedef enum {
    SCTOOL_MODE_NONE      = 0x0,
    SCTOOL_MODE_DEFINESKYCELLS,
    SCTOOL_MODE_LIST,
} sctoolMode;

pxConfig *sctoolConfig(pxConfig *config, int argc, char **argv);

#endif // SCTOOL_H

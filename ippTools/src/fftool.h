/*
 * fftool.h
 *
 * Copyright (C) 2013 IfA Hawaii
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

#ifndef FFTOOL_H
#define FFTOOL_H 1

#include "pxtools.h"

typedef enum {
    FFTOOL_MODE_NONE           = 0x0,
    FFTOOL_MODE_DEFINEBYQUERY,
    FFTOOL_MODE_DEFINEFORSTACKS,
    FFTOOL_MODE_UPDATERUN,
    FFTOOL_MODE_TODO,
    FFTOOL_MODE_ADDRESULT,
    FFTOOL_MODE_RESULT,
    FFTOOL_MODE_REVERT,
    FFTOOL_MODE_UPDATERESULT,
    FFTOOL_MODE_TOADVANCE,
    FFTOOL_MODE_ADDSUMMARY,
    FFTOOL_MODE_REVERTSUMMARY,
    FFTOOL_MODE_UPDATESUMMARY,
    FFTOOL_MODE_SUMMARY,
    FFTOOL_MODE_EXPORTRUN,
    FFTOOL_MODE_IMPORTRUN,
} fftoolMode;

pxConfig *fftoolConfig(pxConfig *config, int argc, char **argv);

#endif // FFTOOL_H

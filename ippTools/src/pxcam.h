/*
 * pxcam.h
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

#ifndef PXCAM_H
#define PXCAM_H 1

#include <pslib.h>

#include "pxtools.h"

bool pxcamRunSetState(pxConfig *config, psS64 cam_id, const char *state, psS64 magicked);
bool pxcamSetSearchArgs (psMetadata *md);
bool pxcamGetSearchArgs (pxConfig *config, psMetadata *where);

bool pxcamQueueByChipID(pxConfig *config,
                        psS64 chip_id,
                        char *workdir,
                        char *label,
                        char *data_group,
                        char *dist_group,
                        char *recipe,
                        char *expgroup,
                        char *dvodb,
                        char *tess_id,
                        char *end_stage,
                        psS64 magicked,
                        char *note);

#endif // PXCAM_H

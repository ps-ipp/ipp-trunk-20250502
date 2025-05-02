/*
 * pxfake.h
 *
 * Copyright (C) 2007-2008  Joshua Hoblitt
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

#ifndef PXFAKE_H
#define PXFAKE_H 1

#include <pslib.h>

#include "pxtools.h"

bool pxfakeRunSetState(pxConfig *config, psS64 fake_id, const char *state);

psS64 pxfakeQueueByCamID(pxConfig *config,
                    psS64 cam_id,
                    char *workdir,
                    char *label,
                    char *data_group,
                    char *dist_group,
                    char *reduction,
                    char *expgroup,
                    char *dvodb,
                    char *tess_id,
                    char *end_stage,
                    char *note);


#endif // PXFAKE_H

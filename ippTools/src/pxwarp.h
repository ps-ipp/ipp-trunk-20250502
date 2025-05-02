/*
 * pxwarp.h
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

#ifndef PXWARP_H
#define PXWARP_H 1

#include <pslib.h>

#include "pxtools.h"

bool pxwarpQueueByFakeID(pxConfig *config,
                         psS64 fake_id,
                         const char *workdir,
                         const char *label,
                         const char *data_group,
                         const char *dist_group,
                         const char *dvodb,
                         const char *tess_id,
                         const char *reduction,
                         const char *end_stage,
                         const char *note);

#endif // PXWARP_H

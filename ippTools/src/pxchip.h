/*
 * pxchip.h
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

#ifndef PXCHIP_H
#define PXCHIP_H 1

#include <pslib.h>

#include "pxtools.h"



bool pxchipRunSetState(pxConfig *config, psS64 chip_id, const char *state, const psS64 magicked);
bool pxchipProcessedImfileSetStateByQuery(pxConfig *config, psMetadata *where, const char *state);

psS64 pxchipQueueByExpTag(pxConfig *config,
                         psS64 exp_id,
                         const char *workdir,
                         const char *label,
                         const char *data_group,
                         const char *dist_group,
                         const char *reduction,
                         const char *expgroup,
                         const char *dvodb,
                         const char *tess_id,
                         const char *end_stage,
                         const char *note);


bool pxchipSetSearchArgs (psMetadata *md);
bool pxchipGetSearchArgs (pxConfig *config, psMetadata *where);

#endif // PXCHIP_H

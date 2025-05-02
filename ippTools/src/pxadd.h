/*
 * pxadd.h
 *
 * Copyright (C) 2009  Joshua Hoblitt, Chris Waters
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

#ifndef PXADD_H
#define PXADD_H 1

#include <pslib.h>

#include "pxtools.h"

bool pxaddRunSetState(pxConfig *config, psS64 add_id, const char *state);
bool pxaddRunSetStateByQuery(pxConfig *config, psMetadata *where, const char *stage, const char *state);
bool pxaddRunSetLabel(pxConfig *config, psS64 add_id, const char *label);
bool pxaddRunSetLabelByQuery(pxConfig *config, psMetadata *where, const char *stage, const char *label);

bool pxaddQueueByCamID(pxConfig *config,
		       char *stage,
		       psS64 stage_id,
		       psS32 stage_extra1,
		       char *workdir,
		       char *reduction,
		       char *label,
                       char *data_group,
		       char *dvodb,
		       char *note,
		       bool image_only,
		       bool minidvodb,
		       char *minidvodb_group,
		       char *minidvodb_name,
		       char *minidvodb_host);

#endif // PXADD_H

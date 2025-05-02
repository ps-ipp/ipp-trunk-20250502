/*
 * pxfpcam.h
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

#ifndef PXFPCAM_H
#define PXFPCAM_H 1

#include <pslib.h>

#include "pxtools.h"

bool pxfpcamSetSearchArgs (psMetadata *md);
bool pxfpcamGetSearchArgs (pxConfig *config, psMetadata *where);

bool pxfpcamRunSetState(pxConfig *config, psS64 cam_id, const char *state);

bool pxfpcamQueueByCamAndChipID(pxConfig *config,
                        psS64 chip_id,
                        psS64 cam_id,
                        char *workdir,
                        char *label,
                        char *data_group,
                        char *dist_group,
                        char *reduction,
                        char *dvodb,
                        char *note);

/* Structure for fpcamInsertRow.  The fpcamRun is different from most because it merges the
   fields of possibly different chipRun and camRuns.  We cannot use the standard camRunRow
   tools because those functions refer to the linked chip_id.
 */

typedef struct {
    psS64           chip_id;
    psS64           cam_id;
    char            *state;
    char            *workdir;
    char            *workdir_state;
    char            *label;
    char            *data_group;
    char            *dist_group;
    char            *reduction;
    char            *dvodb;
    char            *software_ver;
    char            *note;
} fpcamInsertRow;

fpcamInsertRow *fpcamInsertObjectFromMetadata(psMetadata *md);

#endif // PXFPCAM_H

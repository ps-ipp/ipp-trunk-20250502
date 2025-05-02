/*
 * pxmagic.h
 *
 * Copyright (C) 2009  IfA
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

#ifndef PXMAGIC_H
#define PXMAGIC_H 1

#include <pslib.h>

#include "pxtools.h"

extern bool pxmagicRestoreStage(pxConfig *config, psString stage, psString whereClause, psString newState);

extern bool pxmagicAddWhere(pxConfig *config, psString *string, psString table);
extern void pxmagicAddArguments(psMetadata *md);

#endif // PXMAGIC_H

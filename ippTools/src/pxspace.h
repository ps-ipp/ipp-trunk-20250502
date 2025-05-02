/*
 * pxspace.h
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

#ifndef PXSPACE_H
#define PXSPACE_H 1

extern bool pxspaceAddWhere(pxConfig *config, psString *pQuery, psString table);
extern void pxspaceAddArguments(psMetadata *md);

extern bool pxspaceBoxAddWhere(pxConfig *config, psMetadata *where);
extern void pxspaceBoxAddArguments(psMetadata *md);

extern bool pxskycellAddWhere(pxConfig *config, psMetadata *where);
extern void pxskycellAddArguments(psMetadata *md);

#endif // PXSPACE_H

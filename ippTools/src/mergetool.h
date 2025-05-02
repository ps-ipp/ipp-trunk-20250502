/*
 * mergetool.h
 *
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

#ifndef MERGETOOL_H
#define MERGETOOL_H 1

#include "pxtools.h"

typedef enum {
  MERGETOOL_MODE_NONE      = 0x0,
  MERGETOOL_MODE_DEFINEBYQUERY,
  MERGETOOL_MODE_UPDATERUN, 
  MERGETOOL_MODE_PENDINGMERGE, 
  MERGETOOL_MODE_ADDMERGED, 
  MERGETOOL_MODE_LISTMERGED,
  MERGETOOL_MODE_REVERTMERGED,
  MERGETOOL_MODE_UPDATEMERGED,
  MERGETOOL_MODE_DEFINEBYQUERYMERGECOPY,
  MERGETOOL_MODE_LISTMERGEDVODBCOPY,
  MERGETOOL_MODE_REVERTMERGEDVODBCOPY,
  MERGETOOL_MODE_UPDATEMERGEDVODBCOPY
} mergetoolMode;

pxConfig *mergetoolConfig(pxConfig *config, int argc, char **argv);

#endif // MERGETOOL_H

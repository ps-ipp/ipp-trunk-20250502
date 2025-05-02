/*
 * pxerrors.c
 *
 * Copyright (C) 2006  Eugene Magnier
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "pxtools.h"

psExit pxerrorGetExitStatus(void) {

    // psErrorCodeLast returns a psErrorCode, but we are being a bit sleezy and defining a set
    // of enum values that have non-overlapping-ranges (not enforced by the compiler)

    int err = psErrorCodeLast ();
    switch (err) {
      case PXTOOLS_ERR_SYS:
        return PS_EXIT_SYS_ERROR;
      case PXTOOLS_ERR_CONFIG:
        return PS_EXIT_CONFIG_ERROR;
      case PXTOOLS_ERR_PROG:
        return PS_EXIT_PROG_ERROR;
      default:
        return PS_EXIT_UNKNOWN_ERROR;
    }
    return PS_EXIT_UNKNOWN_ERROR;
}

/*
 * pxfault.c
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <pslib.h>

#include "pxtools.h"

bool pxSetFaultCode(psDB *dbh, const char *tableName, psMetadata *where, psS16 code, psS16 quality)
{
    PS_ASSERT_PTR_NON_NULL(dbh, false);
    PS_ASSERT_PTR_NON_NULL(tableName, false);
    PS_ASSERT_PTR_NON_NULL(where, false);

#if 0
    // map code string to numeric fault code
    psU32 code = mapCodeStrToInt(codeStr);
    if (code == (psU32)-1) {
        psError(PS_ERR_UNKNOWN, false, "error resolving error code");
        return false;
    }
#endif

    // update the database
    psMetadata *values = psMetadataAlloc();
    if (!psMetadataAddS16(values, PS_LIST_HEAD, "fault", 0, NULL, code)) {
        psError(PS_ERR_UNKNOWN, false, "failed to add metadata item fault");
        psFree(values);
        return false;
    }
    if (quality) {
        if (!psMetadataAddS16(values, PS_LIST_HEAD, "quality", 0, NULL, quality)) {
            psError(PS_ERR_UNKNOWN, false, "failed to add metadata item quality");
            psFree(values);
            return false;
        }
    }

    long rowsAffected = psDBUpdateRows(dbh, tableName, where, values); 
    psFree(values);
    if (rowsAffected < 0) {
        // database error
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }
    if (rowsAffected < 1) {
        // we didn't do anything
        psError(PS_ERR_UNKNOWN, false, "zero rows were affected - either the search criteria didn't match any rows or the field already had the value being set.");
        return false;
    }

    return true;
}

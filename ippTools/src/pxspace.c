/*
 * pxmagic.c
 *
 * Copyright (C) 2009 IfA
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

// #include <stdlib.h>
// #include <ippdb.h>
// #include <string.h>

#include "pxtools.h"

bool pxspaceAddWhere(pxConfig *config, psString *pQuery, psString table)
{
    PS_ASSERT_PTR_NON_NULL(config, NULL);
    PS_ASSERT_PTR_NON_NULL(table, NULL);

    PXOPT_LOOKUP_F64(radius, config->args, "-radius", false, false);

    if (radius > 0) {
        PXOPT_LOOKUP_F64(ra, config->args, "-ra", false, false);
        PXOPT_LOOKUP_F64(decl, config->args, "-decl", false, false);

        ra   *= PS_RAD_DEG;
        decl *= PS_RAD_DEG;
        radius *= PS_RAD_DEG;

        psStringAppend(pQuery, " AND (ACOS((cos(%s.decl) * cos(%lf) * cos(%s.ra - %lf)) +(sin(%s.decl) * sin(%lf))) < %lf)",
                table, decl, table, ra, table, decl, radius);
    }

    return true;
}

void pxspaceAddArguments(psMetadata *md)
{
    psMetadataAddF64(md, PS_LIST_TAIL, "-radius", 0,           "search for exposures within radius RA DEC (degrees)", false);
    psMetadataAddF64(md, PS_LIST_TAIL, "-ra", 0,               "RA value for radius search (degrees)", false);
    psMetadataAddF64(md, PS_LIST_TAIL, "-decl", 0,             "DEC value for radius search (degrees)", false);
}

bool pxspaceBoxAddWhere(pxConfig *config, psMetadata *where)
{
  PXOPT_LOOKUP_F64(ra_min, config->args, "-ra_min", false, NAN);
  PXOPT_LOOKUP_F64(ra_max, config->args, "-ra_max", false, NAN);
  PXOPT_LOOKUP_F64(dec_min, config->args, "-dec_min", false, NAN);
  PXOPT_LOOKUP_F64(dec_max, config->args, "-dec_max", false, NAN);

  if (isfinite(ra_min)) {
    ra_min *= PS_RAD_DEG;
    psMetadataAddF32(where,PS_LIST_TAIL,"rawExp.ra",PS_META_DUPLICATE_OK, ">=", ra_min);
  }
  if (isfinite(ra_max)) {
    ra_max *= PS_RAD_DEG;
    psMetadataAddF32(where,PS_LIST_TAIL,"rawExp.ra",PS_META_DUPLICATE_OK, "<=", ra_max);
  }
  if (isfinite(dec_min)) {
    dec_min *= PS_RAD_DEG;
    psMetadataAddF32(where,PS_LIST_TAIL,"rawExp.decl",PS_META_DUPLICATE_OK, ">=", dec_min);
  }
  if (isfinite(dec_max)) {
    dec_max *= PS_RAD_DEG;
    psMetadataAddF32(where,PS_LIST_TAIL,"rawExp.decl",PS_META_DUPLICATE_OK, "<=", dec_max);
  }
    
  return true;
}

void pxspaceBoxAddArguments(psMetadata *md)
{
    psMetadataAddF64(md,  PS_LIST_TAIL, "-ra_min",            0, "search by right ascension (degrees)", NAN);
    psMetadataAddF64(md,  PS_LIST_TAIL, "-dec_min",           0, "search by declination (degrees)", NAN);
    psMetadataAddF64(md,  PS_LIST_TAIL, "-ra_max",            0, "search by right ascension (degrees)", NAN);
    psMetadataAddF64(md,  PS_LIST_TAIL, "-dec_max",           0, "search by declination (degrees)", NAN);
}
  

void pxskycellAddArguments(psMetadata *md)
{
    psMetadataAddF32(md,  PS_LIST_TAIL, "-ra_min",            0, "search by right ascension (degrees)", NAN);
    psMetadataAddF32(md,  PS_LIST_TAIL, "-dec_min",           0, "search by declination (degrees)", NAN);
    psMetadataAddF32(md,  PS_LIST_TAIL, "-ra_max",            0, "search by right ascension (degrees)", NAN);
    psMetadataAddF32(md,  PS_LIST_TAIL, "-dec_max",           0, "search by declination (degrees)", NAN);
    psMetadataAddF32(md,  PS_LIST_TAIL, "-glong_min",         0, "search by galactic longitude (degrees)", NAN);
    psMetadataAddF32(md,  PS_LIST_TAIL, "-glat_min",          0, "search by galactic latitude (degrees)", NAN);
    psMetadataAddF32(md,  PS_LIST_TAIL, "-glong_max",         0, "search by galactic longitude (degrees)", NAN);
    psMetadataAddF32(md,  PS_LIST_TAIL, "-glat_max",          0, "search by galactic latitude (degrees)", NAN);
    psMetadataAddF32(md,  PS_LIST_TAIL, "-abs_glat_min",       0, "search by absolute value(galactic latitude) (degrees)", NAN);
    psMetadataAddF32(md,  PS_LIST_TAIL, "-abs_glat_max",       0, "search by absolute value(galactic latitude) (degrees)", NAN);
}

bool pxskycellAddWhere(pxConfig *config, psMetadata *where)
{
    PXOPT_COPY_F32(config->args, where, "-ra_min", "skycell.radeg", ">=");
    PXOPT_COPY_F32(config->args, where, "-dec_min", "skycell.decdeg", ">=");
    PXOPT_COPY_F32(config->args, where, "-ra_max", "skycell.radeg", "<");
    PXOPT_COPY_F32(config->args, where, "-dec_max", "skycell.decdeg", "<");
    PXOPT_COPY_F32(config->args, where, "-glong_min", "skycell.glong", ">=");
    PXOPT_COPY_F32(config->args, where, "-glat_min", "skycell.glat", ">=");
    PXOPT_COPY_F32(config->args, where, "-glong_max", "skycell.glong", "<");
    PXOPT_COPY_F32(config->args, where, "-glat_max", "skycell.glat", "<");
    PXOPT_COPY_F32(config->args, where, "-abs_glat_min", "abs(skycell.glat)", ">=");
    PXOPT_COPY_F32(config->args, where, "-abs_glat_max", "abs(skycell.glat)", "<");
    return true;
}

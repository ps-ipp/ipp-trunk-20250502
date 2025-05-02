/* -*- c-file-style: "Ellemtel" -*-
 *
 * persist_fits.h - definitions for reading and writing persistence info to FITS tables.
 *
 */

#ifndef _INCLUDED_persist_fits_
#define _INCLUDED_persist_fits_

#include "fh/fh.h"
#include "burntool.h"

/* Keywords in primary header that give extension names of burn tables. */
#define PHU_NAME_BURN_AREA       "BTOOLAR"
#define PHU_NAME_BURN_FIT        "BTOOLFIT"

/* Flag indicating whether burn correction has already been applied. */
#define PHU_NAME_BURN_APPLIED    "BTOOLAPP"

/* Comments on keywords - use these if you rewrite them for any reason. */
#define PHU_COMMENT_BURN_AREA    "Name of extension containing burntool streak areas"
#define PHU_COMMENT_BURN_FIT     "Name of extension containing burntool streak fits"
#define PHU_COMMENT_BURN_APPLIED "[T=applied] Burn streaks applied to image data"

fh_result
persist_fits_read(CELL *cell, const char * filename, int apply);

fh_result
persist_fits_write(CELL *cell, HeaderUnit phu);

fh_result
persist_fits_remove_tables(HeaderUnit phu_in, const char * fileout);

#endif /* _INCLUDED_persist_fits_ */

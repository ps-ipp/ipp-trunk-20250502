# include "fakeastro.h"

static AstromOffsetTable *table = NULL;

int save_astrom_table () {

  char mapfile[DVO_MAX_PATH];
  snprintf_nowarn (mapfile, DVO_MAX_PATH, "%s/AstroMap.fits", CATDIR);
  AstromOffsetMapSave (table, mapfile);

  return TRUE;
}

AstromOffsetTable *get_astrom_table () {
  return table;
}

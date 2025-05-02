# include <ohana.h>
# include <gfitsio.h>

int gfits_free_table (FTable *table) {
  
  if (table[0].buffer == (char *) NULL) return (TRUE);
  free (table[0].buffer);
  table[0].buffer = (char *) NULL;
  return (TRUE);
}

int gfits_free_vtable (VTable *table) {
  
  int i;

  if (table[0].buffer == (char **) NULL) return (TRUE);

  for (i = 0; i < table[0].Nrow; i++) {
    free (table[0].buffer[i]);
  }
  free (table[0].buffer);
  free (table[0].row);

  table[0].buffer = NULL;
  table[0].row    = NULL;
  return (TRUE);
}


# include "dvo.h"
# include "tap_ohana.h"

int compare_tables (AstromOffsetTable *table_src, AstromOffsetTable *table_tgt);

int main (void) {

  int i, j, k, status;

  plan_tests (181);

  diag ("libdvo AstromOffsetMapIO.c tests");

  /*** create a small, simple reference AstroMap ***/
  AstromOffsetTable *table = NULL;
  ALLOCATE (table, AstromOffsetTable, 1);

  table->Nmap = 4;
  ALLOCATE (table->map, AstromOffsetMap *, table->Nmap);

  // assign the map values (this allocates just the area needed for each image, not the
  // full 6x6, saving some memory while doing the analysis)
  for (i = 0; i < table->Nmap; i++) {
    ALLOCATE (table->map[i], AstromOffsetMap, 1);

    table->map[i][0].Nx 	  = i + 1;
    table->map[i][0].Ny 	  = i + 1;
    table->map[i][0].tableID      = i + 10;
    table->map[i][0].imageID      = i + 20;
    
    // STORE THESE VALUES?
    table->map[i][0].dX 	  = 2*i + 0.5;
    table->map[i][0].dY 	  = 3*i + 0.5;

    // since this was on disk, we obviously keep it
    table->map[i][0].keep 	  = TRUE;

    ALLOCATE (table->map[i][0].dXv, float *, table->map[i][0].Nx);
    ALLOCATE (table->map[i][0].dYv, float *, table->map[i][0].Nx);

    for (j = 0; j < table->map[i][0].Nx; j++) {
      ALLOCATE (table->map[i][0].dXv[j], float, table->map[i][0].Ny);
      ALLOCATE (table->map[i][0].dYv[j], float, table->map[i][0].Ny);

      for (k = 0; k < table->map[i][0].Ny; k++) {
	table->map[i][0].dXv[j][k] = i+j+k+1;
	table->map[i][0].dYv[j][k] = i+j+k+2;
      }
    }
  }

  status = AstromOffsetMapSave (table, "test.v0.fits");
  ok (status, "wrote test table to file");

  AstromOffsetTable *table_full = AstromOffsetMapLoad ("test.v0.fits", 0, TRUE);
  ok (table_full, "read test table from file");

  /*** compare table_full and table above ***/
  diag ("compare table_full and table");
  compare_tables (table_full, table);

  /*** compare AstromOffsetMapLoad in blocks vs single read ***/
  AstromOffsetTable *table_rows = AstromOffsetMapLoad ("test.v0.fits", 100, TRUE);
  ok (table_rows, "read in table in 100 rows per read");
    
  diag ("compare table_rows and table");
  compare_tables (table_rows, table);
    
  return exit_status();
}

int compare_tables (AstromOffsetTable *table_src, AstromOffsetTable *table_tgt) {

  int i, j, k;

  ok (table_tgt->Nmap == table_src->Nmap, "number of maps matches");
    
  for (i = 0; i < table_src->Nmap; i++) {
    ok (table_tgt->map[i][0].Nx      == table_src->map[i][0].Nx, 	"Nx sizes match");
    ok (table_tgt->map[i][0].Ny      == table_src->map[i][0].Ny, 	"Ny sizes match");
    ok (table_tgt->map[i][0].tableID == table_src->map[i][0].tableID, "tableIDs match");
    ok (table_tgt->map[i][0].imageID == table_src->map[i][0].imageID, "imageIDs match");
    ok (table_tgt->map[i][0].dX      == table_src->map[i][0].dX,      "dX matches");
    ok (table_tgt->map[i][0].dY      == table_src->map[i][0].dY,      "dY matches");
    ok (table_tgt->map[i][0].keep    == table_src->map[i][0].keep,    "keep matches");

    for (j = 0; j < table_src->map[i][0].Nx; j++) {
      for (k = 0; k < table_src->map[i][0].Ny; k++) {
	ok (table_tgt->map[i][0].dXv[j][k] == table_src->map[i][0].dXv[j][k], "dX values match");
	ok (table_tgt->map[i][0].dYv[j][k] == table_src->map[i][0].dYv[j][k], "dY values match"); 
      }
    }
  }
  return TRUE;
}

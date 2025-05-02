# include "dvo.h"

AstromOffsetTable *AstromOffsetMapLoad (char *filename, int Nrows, int VERBOSE) {

  off_t Nmap;

  Header header;
  Matrix matrix;

  Header theader;
  FTable ftable;
  ftable.header = &theader;
  ftable.buffer = NULL;

  FILE *f = fopen (filename, "r");
  if (f == NULL) {
    if (VERBOSE) fprintf (stderr, "can't find Astrom Offset Map file %s\n", filename);
    return (NULL);
  }

  /* load in table data */
  if (!gfits_fread_header (f, &header)) {
    if (VERBOSE) fprintf (stderr, "can't read Astrom Offset Map header\n");
    fclose (f);
    return (NULL);
  }
  if (!gfits_fread_matrix (f, &matrix, &header)) {
    if (VERBOSE) fprintf (stderr, "can't read Astrom Offset Map matrix\n");
    gfits_free_header (&header);
    fclose (f);
    return (NULL);
  }

  // the output table is not what is stored in the FITS file.  read and convert 
  // from the disk file format
  AstromOffsetTable *table = NULL;

  // for now, we only have one flavor (6x6) of disk file format

  // loop over the ftable reading blocks of 100k rows at a time
  if (Nrows) {
    if (!gfits_find_Xheader (f, ftable.header, "ASTROM_OFFSET_MAP_DISK_6x6")) myAbort ("problem 1");
    int NrowsTotal = ftable.header[0].Naxis[1];
    int Nblocks = (NrowsTotal % Nrows) ? (int) (NrowsTotal / Nrows) + 1 : (NrowsTotal / Nrows);

    // allocate the full array up front.  passes through AstromOffsetMapAppendToTable 
    // check the current allocation and will bump the allocation as needed.
    ALLOCATE (table, AstromOffsetTable, 1);
    ALLOCATE (table->map, AstromOffsetMap *, NrowsTotal);
    table->Nmap = 0;
    table->NMAP = NrowsTotal;

    // on each call to gfits_fread_ftable_range, we need to reset the FILE pointer to the
    // beginning of the data block.  again this seems a little wrong.
    off_t diskStart = ftello (f);

    int i;
    for (i = 0; i < Nblocks; i++) {
      if (!gfits_fread_ftable_range (f, FALSE, FALSE, &ftable, i*Nrows, Nrows)) myAbort ("problem 2");
    
      AstromOffsetMap_Disk_6x6 *map_disk = gfits_table_get_AstromOffsetMap_Disk_6x6 (&ftable, &Nmap, NULL, NULL);
      if (!map_disk) myAbort ("ERROR: failed to read Astrom Offset Map\n");

      table = AstromOffsetMapAppendToTable (table, map_disk, Nmap);

      // XXX this is kind of hack-ish: gfits_fread_ftable_range modifies Naxis[1] to the
      // number of rows actually read, but this makes a subsequent read invalid.  I'm
      // resetting the value of Naxis[1] before the next read to deal with this problem.
      // But is there a more consistent way to address this?

      // Answer: the FTable structure could include information about the table
      // representation on disk, including start,Nrows values.  I'm not going to mess
      // around with that just now.

      ftable.header[0].Naxis[1] = NrowsTotal;
      fseeko (f, diskStart, SEEK_SET);
    }
    AstromOffsetTableSetIDs (table);

    gfits_free_header (&theader);
    gfits_free_table  (&ftable);
  } else {

    // convert the blocks into the equivalent table entries and append
    if (!gfits_fread_ftable (f, &ftable, "ASTROM_OFFSET_MAP_DISK_6x6")) {
      if (VERBOSE) fprintf (stderr, "can't read Astrom Offset Map table\n");
      gfits_free_header (&header);
      gfits_free_matrix (&matrix);
      fclose (f);
      return (NULL);
    }
    AstromOffsetMap_Disk_6x6 *map_disk = gfits_table_get_AstromOffsetMap_Disk_6x6 (&ftable, &Nmap, NULL, NULL);
    if (!map_disk) {
      fprintf (stderr, "ERROR: failed to read Astrom Offset Map\n");
      exit (2);
    }

    // AstromOffsetMap_Disk_6x6 *map_disk is an external format stored as an array of maps.
    // Convert the disk array of maps to then internal format in a rich structure:
    table = AstromOffsetMapToTable (map_disk, Nmap);

    gfits_free_header (&theader);
    gfits_free_table  (&ftable);
  }

  gfits_free_header (&header);
  gfits_free_matrix (&matrix);
  fclose (f);

  return (table);
}

int AstromOffsetMapSave (AstromOffsetTable *table, char *filename) {

  off_t Nmap;

  Header header;
  Matrix matrix;
  Header theader;
  FTable ftable;

  FILE *f;

  /* make phu header (no matrix needed) */
  gfits_init_header (&header);
  header.extend = TRUE;
  gfits_create_header (&header);
  gfits_create_matrix (&header, &matrix);

  // AstromOffsetMap_Disk_6x6 *map_disk is an external format stored as an array of maps.
  // Convert the internal format in a rich structure into a disk array of maps.
  AstromOffsetMap_Disk_6x6 *map_disk = AstromOffsetTableToMap (table, &Nmap); 

  ftable.header = &theader;
  gfits_table_set_AstromOffsetMap_Disk_6x6 (&ftable, map_disk, Nmap, TRUE);
  FREE (map_disk);

  f = fopen (filename, "w");
  if (f == (FILE *) NULL) { 
    fprintf (stderr, "cannot open %s for output\n", filename);
    return (FALSE);
  }
  
  gfits_fwrite_header  (f, &header);
  gfits_fwrite_matrix  (f, &matrix);
  gfits_fwrite_Theader (f, &theader);
  gfits_fwrite_table  (f, &ftable);
  fclose (f);

  gfits_free_header (&header);
  gfits_free_matrix (&matrix);
  gfits_free_header (&theader);
  gfits_free_table  (&ftable);

  return (TRUE);
}

AstromOffsetTable *AstromOffsetMapToTable(AstromOffsetMap_Disk_6x6 *map_disk, off_t Nmap) {

  unsigned int i;
  int j, k;

  AstromOffsetTable *table = NULL;
  ALLOCATE (table, AstromOffsetTable, 1);

  table->Nmap = Nmap;
  ALLOCATE (table->map, AstromOffsetMap *, Nmap);

  // find the max value of imageID
  unsigned int MaxTableID = 0;
  unsigned int MaxImageID = 0;
  for (i = 0; i < Nmap; i++) {
    MaxTableID = MAX(MaxTableID, map_disk[i].tableID);
    MaxImageID = MAX(MaxImageID, map_disk[i].imageID);
  }
  table->MaxTableID = MaxTableID;
  table->MaxImageID = MaxImageID;

  // generate the index and init values to -1
  ALLOCATE (table->imageIDtoTableSeq, int, MaxImageID + 1);
  for (i = 0; i <= MaxImageID; i++) {
    table->imageIDtoTableSeq[i] = -1;
  }

  // assign the ID values
  for (i = 0; i < Nmap; i++) {
    int ImageID = map_disk[i].imageID;
    myAssert (table->imageIDtoTableSeq[ImageID] == -1, "oops, duplicate image IDs");
    table->imageIDtoTableSeq[ImageID] = i;
  }

  // assign the map values (this allocates just the area needed for each image, not the
  // full 6x6, saving some memory while doing the analysis)
  for (i = 0; i < Nmap; i++) {
    ALLOCATE (table->map[i], AstromOffsetMap, 1);

    table->map[i][0].Nx 	  = map_disk[i].Nx;
    table->map[i][0].Ny 	  = map_disk[i].Ny;
    table->map[i][0].tableID      = map_disk[i].tableID;
    table->map[i][0].imageID      = map_disk[i].imageID;
    
    // STORE THESE VALUES?
    table->map[i][0].dX 	  = map_disk[i].dX;
    table->map[i][0].dY 	  = map_disk[i].dY;

    // since this was on disk, we obviously keep it
    table->map[i][0].keep 	  = TRUE;

    int Nx = table->map[i][0].Nx;
    int Ny = table->map[i][0].Ny;

    ALLOCATE (table->map[i][0].dXv, float, Nx*Ny);
    ALLOCATE (table->map[i][0].dYv, float, Nx*Ny);

    for (j = 0; j < Nx; j++) {
      for (k = 0; k < Ny; k++) {
	table->map[i][0].dXv[j + k*Nx] = map_disk[i].dXv[j][k];
	table->map[i][0].dYv[j + k*Nx] = map_disk[i].dYv[j][k];
      }
    }
  }
  return table;
}

AstromOffsetMap_Disk_6x6 *AstromOffsetTableToMap(AstromOffsetTable *table, off_t *Nmap) {

  int i, j, k;

  AstromOffsetMap_Disk_6x6 *map_disk = NULL;
  ALLOCATE (map_disk, AstromOffsetMap_Disk_6x6, table->Nmap);

  // assign the map values (this allocates just the area needed for each image, not the
  // full 6x6, saving some memory while doing the analysis)
  // some maps in the table should be skipped because their image no longer uses them
  int Ndisk = 0;
  for (i = 0; i < table->Nmap; i++) {
    if (!table->map[i][0].keep) continue;
    map_disk[Ndisk].Nx 	    = table->map[i][0].Nx;
    map_disk[Ndisk].Ny 	    = table->map[i][0].Ny;
    map_disk[Ndisk].tableID = table->map[i][0].tableID;
    map_disk[Ndisk].imageID = table->map[i][0].imageID;
    
    map_disk[Ndisk].dX 	    = table->map[i][0].dX;
    map_disk[Ndisk].dY 	    = table->map[i][0].dY;

    int Nx = table->map[i][0].Nx;
    int Ny = table->map[i][0].Ny;

    for (j = 0; j < Nx; j++) {
      for (k = 0; k < Ny; k++) {
	map_disk[Ndisk].dXv[j][k] = table->map[i][0].dXv[j + k*Nx];
	map_disk[Ndisk].dYv[j][k] = table->map[i][0].dYv[j + k*Nx];
      }
      for (k = map_disk[Ndisk].Ny; k < 6; k++) {
	map_disk[Ndisk].dXv[j][k] = 0.0;
	map_disk[Ndisk].dYv[j][k] = 0.0;
      }
    }
    for (j = map_disk[Ndisk].Nx; j < 6; j++) {
      for (k = 0; k < 6; k++) {
	map_disk[Ndisk].dXv[j][k] = 0.0;
	map_disk[Ndisk].dYv[j][k] = 0.0;
      }
    }
    Ndisk ++;
  }
  *Nmap = Ndisk;
  return map_disk;
}

AstromOffsetTable *AstromOffsetMapAppendToTable(AstromOffsetTable *table, AstromOffsetMap_Disk_6x6 *map_disk, off_t Nmap) {

  int i, j, k, Nstart;

  if (table == NULL) {
    ALLOCATE (table, AstromOffsetTable, 1);
    Nstart = 0;
    table->Nmap = Nmap;
    ALLOCATE (table->map, AstromOffsetMap *, Nmap);
  } else {
    Nstart = table->Nmap;
    if (table->Nmap + Nmap > table->NMAP) {
      table->NMAP = table->Nmap + Nmap;
      REALLOCATE (table->map, AstromOffsetMap *, table->NMAP);
    }
    table->Nmap += Nmap;
  }

  // append map values (this allocates just the area needed for each image, not the
  // full 6x6, saving some memory while doing the analysis)
  for (i = 0; i < Nmap; i++) {
    int N = i + Nstart;
    ALLOCATE (table->map[N], AstromOffsetMap, 1);

    table->map[N][0].Nx 	  = map_disk[i].Nx;
    table->map[N][0].Ny 	  = map_disk[i].Ny;
    table->map[N][0].tableID 	  = map_disk[i].tableID;
    table->map[N][0].imageID 	  = map_disk[i].imageID;
    
    // STORE THESE VALUES?
    table->map[N][0].dX 	  = map_disk[i].dX;
    table->map[N][0].dY 	  = map_disk[i].dY;

    // since this was on disk, we obviously keep it
    table->map[N][0].keep 	  = TRUE;

    int Nx = table->map[N][0].Nx;
    int Ny = table->map[N][0].Ny;

    ALLOCATE (table->map[N][0].dXv, float, Nx*Ny);
    ALLOCATE (table->map[N][0].dYv, float, Nx*Ny);

    for (j = 0; j < Nx; j++) {
      for (k = 0; k < Ny; k++) {
	table->map[N][0].dXv[j + k*Nx] = map_disk[i].dXv[j][k];
	table->map[N][0].dYv[j + k*Nx] = map_disk[i].dYv[j][k];
      }
    }
  }
  return table;
}

int AstromOffsetTableSetIDs (AstromOffsetTable *table) {

  // find the max value of imageID
  unsigned int MaxTableID = 0;
  unsigned int MaxImageID = 0;
  {
    int j;
    for (j = 0; j < table->Nmap; j++) {
      MaxTableID = MAX(MaxTableID, table->map[j][0].tableID);
      MaxImageID = MAX(MaxImageID, table->map[j][0].imageID);
    }
    table->MaxTableID = MaxTableID;
    table->MaxImageID = MaxImageID;
  }

  // generate the index and init values to -1
  {
    unsigned int i;
    ALLOCATE (table->imageIDtoTableSeq, int, MaxImageID + 1);
    for (i = 0; i <= MaxImageID; i++) {
      table->imageIDtoTableSeq[i] = -1;
    }
  }

  // assign the ID values
  { 
    int j;
    for (j = 0; j < table->Nmap; j++) {
      int ImageID = table->map[j][0].imageID;
      myAssert (table->imageIDtoTableSeq[ImageID] == -1, "oops, duplicate image IDs");
      table->imageIDtoTableSeq[ImageID] = j;
    }
  }
  return TRUE;
}

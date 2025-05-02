# include <dvo.h>

int AstromOffsetTableMatchChips (Image *images, off_t Nimages, AstromOffsetTable *table) {

  // we have a table of astrometry offset maps.  we want to find the chips that match to
  // each of the maps so we can assign the map to the image.coords.offsetMap entry

  // we have the lookup table of table->imageIDtoTableSeq to find the match by imageID
  
  // we are just going to assume that table->imageIDtoTableSeq is correct and complete
  off_t i;
  for (i = 0; i < Nimages; i++) {
    // if (images[i].imageID < 0) continue;

    unsigned int imageID = images[i].imageID;
    if (imageID > table->MaxImageID) continue;
    
    int seq = table->imageIDtoTableSeq[imageID];
    if (seq < 0) continue;
    if (seq >= table->Nmap) continue; // this one is probably not valid, right?

    images[i].coords.offsetMap = table->map[seq];    
  }
  return (TRUE);
}

AstromOffsetMap *AstromOffsetMapInit (int Nx, int Ny) {

  AstromOffsetMap *map = NULL;
  ALLOCATE (map, AstromOffsetMap, 1);

  map->dX = NAN; 
  map->dY = NAN; 
  map->Nx = Nx; // output map size
  map->Ny = Ny; // output map size
 
  map->keep = TRUE; // output map size

  ALLOCATE (map->dXv, float, Nx*Ny);
  ALLOCATE (map->dYv, float, Nx*Ny);

  int j, k;
  for (j = 0; j < Nx; j++) {
    for (k = 0; k < Ny; k++) {
      map->dXv[j + k*Nx] = 0.0;
      map->dYv[j + k*Nx] = 0.0;
    }
  }

  return map;
}

void AstromOffsetMapFree (AstromOffsetMap *map) {

  if (!map) return;

  free (map->dXv);
  free (map->dYv);

  free (map);

  return;
}

void AstromOffsetMapSetOrder (AstromOffsetMap *map, int Nx, int Ny, Image *image) {

  int j, k;

  // rather than try to figure out how to resize, and free/allocate, lets just free the old arrays and make new ones
  free (map->dXv);
  free (map->dYv);

  map->Nx = Nx; // output map size
  map->Ny = Ny; // output map size
  map->dX = Nx / (float) image->NX;
  map->dY = Ny / (float) image->NY;
 
  map->keep = TRUE; // output map size

  ALLOCATE (map->dXv, float, Nx*Ny);
  ALLOCATE (map->dYv, float, Nx*Ny);

  for (j = 0; j < Nx; j++) {
    for (k = 0; k < Ny; k++) {
      map->dXv[j + k*Nx] = 0.0;
      map->dYv[j + k*Nx] = 0.0;
    }
  }
  return;
}

AstromOffsetMap *AstromOffsetMapCopy (AstromOffsetMap *map) {

  if (!map) return NULL;

  AstromOffsetMap *tgt = AstromOffsetMapInit (map->Nx, map->Ny);

  tgt->dX = map->dX; 
  tgt->dY = map->dY; 
  tgt->imageID = map->imageID; 
  tgt->tableID = map->tableID; 

  int Nx = map->Nx;
  int Ny = map->Ny;

  int j, k;
  for (j = 0; j < Nx; j++) {
    for (k = 0; k < Ny; k++) {
      tgt->dXv[j + k*Nx] = map->dXv[j + k*Nx];
      tgt->dYv[j + k*Nx] = map->dYv[j + k*Nx];
    }
  }
  return tgt;
}

// copy the data from one to another, assuming a pre-allocated structure
void AstromOffsetMapCopyData (AstromOffsetMap *tgt, AstromOffsetMap *src) {

  myAssert (tgt->Nx == src->Nx, "programming error");
  myAssert (tgt->Ny == src->Ny, "programming error");

  tgt->dX = src->dX; 
  tgt->dY = src->dY; 
  tgt->imageID = src->imageID; 
  tgt->tableID = src->tableID; 

  tgt->keep = src->keep; 

  int Nx = src->Nx;
  int Ny = src->Ny;

  int j, k;
  for (j = 0; j < Nx; j++) {
    for (k = 0; k < Ny; k++) {
      tgt->dXv[j + k*Nx] = src->dXv[j + k*Nx];
      tgt->dYv[j + k*Nx] = src->dYv[j + k*Nx];
    }
  }
  return;
}

void AstromOffsetTableFree (AstromOffsetTable *table) {

  if (!table) return;

  int i;
  for (i = 0; i < table->Nmap; i++) {
    if (!table->map[i]) continue;
    FREE (table->map[i][0].dXv);
    FREE (table->map[i][0].dYv);
    FREE (table->map[i]);
  }
  FREE (table->imageIDtoTableSeq);
  FREE (table->map);
}

int AstromOffsetTableNewMap (AstromOffsetTable *table, int Nx, int Ny, Image *image) {

  off_t i;
  if (image->imageID > table->MaxImageID) {
    int oldMaxID = table->MaxImageID;
    table->MaxImageID = image->imageID;
    REALLOCATE (table->imageIDtoTableSeq, int, table->MaxImageID + 1);
    for (i = oldMaxID + 1; i < table->MaxImageID + 1; i++) {
      table->imageIDtoTableSeq[i] = -1;
    }
  }
  int seq = table->imageIDtoTableSeq[image->imageID];
  if (seq != -1) {
    // already assigned (but not used)
    AstromOffsetMapSetOrder (table->map[seq], Nx, Ny, image);
    image[0].coords.offsetMap = table->map[seq];    
    return TRUE;
  }

  int Nmap = table->Nmap;

  table->Nmap++;
  REALLOCATE (table->map, AstromOffsetMap *, table->Nmap);

  myAssert (table->imageIDtoTableSeq[image->imageID] == -1, "table IDtoSeq not initiazed or image collision");
  table->imageIDtoTableSeq[image->imageID] = Nmap;
  
  table->map[Nmap] = AstromOffsetMapInit (Nx, Ny);

  table->MaxTableID ++;
  table->map[Nmap][0].tableID = table->MaxTableID;
  table->map[Nmap][0].imageID = image->imageID;
  
  table->map[Nmap][0].dX = Nx / (float) image->NX;
  table->map[Nmap][0].dY = Ny / (float) image->NY;

  image[0].coords.offsetMap = table->map[Nmap];
  return TRUE;    
}

int AstromOffsetTableAddMapFromImage (AstromOffsetTable *table, Image *image) {

  int Nmap = table->Nmap;

  table->Nmap++;
  REALLOCATE (table->map, AstromOffsetMap *, table->Nmap);

  // find the imageID and update the imageIDtoTableSeq allocation if needed
  off_t i;
  if (image->imageID > table->MaxImageID) {
    int oldMaxID = table->MaxImageID;
    table->MaxImageID = image->imageID;
    REALLOCATE (table->imageIDtoTableSeq, int, table->MaxImageID + 1);
    for (i = oldMaxID + 1; i < table->MaxImageID + 1; i++) {
      table->imageIDtoTableSeq[i] = -1;
    }
  }
  myAssert (table->imageIDtoTableSeq[image->imageID] == -1, "table IDtoSeq not initiazed or image collision");
  table->imageIDtoTableSeq[image->imageID] = Nmap;
  
  table->map[Nmap] = image[0].coords.offsetMap;
  return TRUE;    
}

AstromOffsetTable *AstromOffsetTableInit() {

  AstromOffsetTable *table = NULL;
  ALLOCATE (table, AstromOffsetTable, 1);

  table->Nmap = 0;
  ALLOCATE (table->map, AstromOffsetMap *, 1);

  table->MaxTableID = 0;
  table->MaxImageID = 0;

  // generate the index and init values to -1
  ALLOCATE (table->imageIDtoTableSeq, int, 1);
  table->imageIDtoTableSeq[0] = -1;

  return table;
}

void AstromOffsetMapPrint (AstromOffsetMap *map, char *filename) {

  FILE *f = stderr;
  if (strcasecmp(filename, "stderr") && strcasecmp(filename, "stdout")) {
    f = fopen (filename, "w");
    if (!f) {
      fprintf (stderr, "failed to open output file %s\n", filename);
    }
  }

  int Nx = map->Nx;
  int Ny = map->Ny;

  fprintf (f, "imageID: %d, tableID: %d (dX: %f, dY: %f), keep: %d\n", map->imageID, map->tableID, map->dX, map->dY, map->keep);
  int ix, iy;
  fprintf (f, "dXv map:\n");
  for (ix = 0; ix < Nx; ix++) {
    for (iy = 0; iy < Ny; iy++) {
      fprintf (f, "%9.5f ", map->dXv[ix + iy*Nx]);
    }
    fprintf (f, "\n");
  }
  fprintf (f, "dYv map:\n");
  for (ix = 0; ix < Nx; ix++) {
    for (iy = 0; iy < Ny; iy++) {
      fprintf (f, "%9.5f ", map->dYv[ix + iy*Nx]);
    }
    fprintf (f, "\n");
  }
  if (f != stderr) fclose (f);
  return;
}

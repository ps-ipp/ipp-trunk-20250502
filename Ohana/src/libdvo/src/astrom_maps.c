# include <dvo.h>

int MatchChipsToAstromMaps (Image *images, off_t Nimages, AstromOffsetTable *table) {

  // we have a table of astrometry offset maps.  we want to find the chips that match to
  // each of the maps so we can assign the map to the image.coords.offsetMap entry

  // we have the lookup table of table->IDtoSeq to find the match by imageID
  
  // we are just going to assume that table->IDtoSeq is correct and complete
  for (i = 0; i < Nimages; i++) {
    int imageID = images[i].imageID;
    if (imageID < 0) continue;
    if (imageID > table->maxID) continue;
    
    int seq = table->IDtoSeq[imageID];
    if (seq < 0) continue;
    if (seq >= table->Nmap) continue; // this one is probably not valid, right?

    images[i].coords.offsetMap = &table->map[seq];    
  }

  return (TRUE);
}

int AstromOffsetTableNewMap (AstromOffsetTable *table, int order, Image *image) {

  int Nmap = table->Nmap;

  table->Nmap++;
  REALLOCATE (table->map, AstromOffsetMap, table->Nmap);

  int Nx = order;
  int Ny = order;

  if (image->imageID > table->MaxID) {
    int oldMaxID = table->MaxID;
    table->MaxID = image->imageID;
    REALLOCATE (table->IDtoSeq, int, table->MaxID + 1);
    for (i = oldMaxID + 1; i < table->MaxID + 1; i++) {
      table->IDtoSeq[i] = -1;
    }
  }
  myAssert (table->IDtoSeq[image->imageID] == -1, "table IDtoSeq not initiazed or image collision");
  table->IDtoSeq[image->imageID] = Nmap;
  
  table->map[Nmap].Nx      = Nx;
  table->map[Nmap].Ny      = Ny;
  table->map[Nmap].ID      = table->maxID; table->maxID ++;
  table->map[Nmap].imageID = image->imageID;
  
  table->map[Nmap].dX = Nx / image->Nx;
  table->map[Nmap].dY = Ny / image->Ny;

  ALLOCATE (table->map[Nmap].dXv, float *, Nx);
  ALLOCATE (table->map[Nmap].dYv, float *, Nx);

  for (j = 0; Nx; j++) {
    ALLOCATE (table->map[i].dXv[j], float, Ny);
    ALLOCATE (table->map[i].dYv[j], float, Ny);

    for (k = 0; k < Ny; k++) {
      table->map[i].dXv[j][k] = 0.0;
      table->map[i].dYv[j][k] = 0.0;
    }
  }
  image[0].coords.offsetMap = &table->map[Nmap];
  return TRUE;    
}


# include "Ximage.h"

bDrawBuffer *bDrawIt (png_color *palette, int Npalette, int Nbyte) {

  Graphic *graphic = GetGraphic();
  bDrawColor black = KapaColorByName ("black");

  // get the number of sections
  int Nsection = GetNumberOfSections ();

  // in order to allow the anti-aliasing to affect the text & graphs but not the images
  // I need to generate the images in one buffer and the graphs in a second buffer
  // then merge the two buffers.

  // base will hold the images
  bDrawBuffer *base = bDrawBufferCreate (graphic->dxwin, graphic->dywin, Nbyte, palette, Npalette);
  bDrawSetStyle (base, black, 0, 0, 1.0);
  
  for (int i = 0; i < Nsection; i++) {
    Section *section = GetSectionByNumber (i);
    bDrawImage (base, section->image, graphic);
  }

  // graph will hold the graphic overlay
  bDrawBuffer *graph = bDrawBufferCreate (graphic->dxwin, graphic->dywin, Nbyte, palette, Npalette);
  bDrawSetStyle (graph, black, 0, 0, 1.0);

  for (int i = 0; i < Nsection; i++) {
    Section *section = GetSectionByNumber (i);
    for (int j = 0; section->image && (j < NOVERLAYS); j++) {
      if (section->image->overlay[j].active) bDrawOverlay (graph, section->image, j);
    }
    bDrawGraph (graph, section->graph);
  }

  // apply anti-aliasing only to the graph
  if (graphic->smooth_sigma > 0.0) {
    // anything > 1.1 blurs the image too much
    graphic->smooth_sigma = MIN (graphic->smooth_sigma, 1.1);
    bDrawSmooth (graph, graphic->smooth_sigma);
  }
  
  // place graph on base
  bDrawMerge (base, graph);
  bDrawBufferFree (graph);

  return (base);
}

void bDrawGraph (bDrawBuffer *buffer, KapaGraphWidget *graph) {
  if (graph == NULL) return;
  bDrawFrame (buffer, graph); 
  bDrawObjects (buffer, graph);
  bDrawLabels (buffer, graph);
  bDrawTextlines (buffer, graph);
}

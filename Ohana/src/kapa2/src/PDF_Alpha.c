# include "Ximage.h"

static int   Nalpha = 0;
static int   NALPHA = 0;
static float *alpha = NULL;

void PDF_AlphaInit () {

  if (alpha) FREE (alpha);
  
  Nalpha =  0;
  NALPHA = 64;

  ALLOCATE (alpha, float, NALPHA);
}

void PDF_AlphaSet (Gobjects *object, IOBuffer *buffer) {

  if (object->alpha >= 1.0) {
    PrintIOBuffer (buffer, "q\n");
    return;
  }

  PrintIOBuffer (buffer, "q /GS%d gs\n", Nalpha);
  alpha[Nalpha] = object->alpha;
  Nalpha ++;

  if (Nalpha >= NALPHA) {
    NALPHA += 64;
    REALLOCATE (alpha, float, NALPHA);
  }
  return;
}

void PDF_AlphaDump (PDF_FILE *obj) {
  
  // write out the graphic states accumulated above
  PDF_Print (obj, 6, "6 0 obj <<\n");
  for (int i = 0; i < Nalpha; i++) {
    PDF_Print (obj, 0, "/GS%d << /Type /ExtGState /CA %.3f /ca %.3f >>\n", i, alpha[i], alpha[i]);
  }
  PDF_Print (obj, 0, ">> endobj\n");
  return;
}

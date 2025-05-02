# include "gastro2.h"

void rotate (RefCatalog *Subset, RefCatalog *Ref, double angle) {
  
  int i;
  double dX, dY, DX, DY, CS, SN;
  double theta;
  StarData *in, *out;

  Ref[0] = Subset[0];

  ALLOCATE (Ref[0].stars, StarData, MAX (1, Ref[0].N));
  bcopy (Subset[0].stars, Ref[0].stars, Ref[0].N*sizeof(StarData));

  if (angle == 0.0) return;

  theta = (angle*RAD_DEG);
  CS = cos (theta);
  SN = sin (theta);

  in  = Subset[0].stars;
  out = Ref[0].stars;

  for (i = 0; i < Ref[0].N; i++) {
    dX = in[i].X;
    dY = in[i].Y;
    
    DX = dX * CS - dY * SN;
    DY = dX * SN + dY * CS;
    
    out[i].X = DX;
    out[i].Y = DY;
  }
    
}

/* rotate the star list by an angle ccw from x axis */

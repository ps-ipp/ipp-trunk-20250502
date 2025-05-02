# include "mosastro.h"

void ChipToSky (StarData *stars, int Nstars, Coords *coords) {
  int i;
  for (i = 0; i < Nstars; i++) {
    XY_to_RD (&stars[i].R, &stars[i].D, stars[i].X, stars[i].Y, coords);
    stars[i].R = ohana_normalize_angle (stars[i].R);
  }
}

void ChipToFP (StarData *stars, int Nstars, Coords *coords) {
  int i;
  for (i = 0; i < Nstars; i++) {
    XY_to_RD (&stars[i].L, &stars[i].M, stars[i].X, stars[i].Y, coords);
  }
}

void FPtoTP (StarData *stars, int Nstars, Coords *coords) {
  int i;
  for (i = 0; i < Nstars; i++) {
    XY_to_RD (&stars[i].P, &stars[i].Q, stars[i].L, stars[i].M, coords);
  }
}

void TPtoSky (StarData *stars, int Nstars, Coords *coords) {
  int i;
  for (i = 0; i < Nstars; i++) {
    XY_to_RD (&stars[i].R, &stars[i].D, stars[i].P, stars[i].Q, coords);
    stars[i].R = ohana_normalize_angle (stars[i].R);
  }
}

void SkyToTP (StarData *stars, int Nstars, Coords *coords) {
  int i;
  for (i = 0; i < Nstars; i++) {
    RD_to_XY (&stars[i].P, &stars[i].Q, stars[i].R, stars[i].D, coords);
  }
}

void TPtoFP (StarData *stars, int Nstars, Coords *coords) {
  int i;
  for (i = 0; i < Nstars; i++) {
    RD_to_XY (&stars[i].L, &stars[i].M, stars[i].P, stars[i].Q, coords);
  }
}

void FPtoChip (StarData *stars, int Nstars, Coords *coords) {
  int i;
  for (i = 0; i < Nstars; i++) {
    RD_to_XY (&stars[i].X, &stars[i].Y, stars[i].L, stars[i].M, coords);
  }
}

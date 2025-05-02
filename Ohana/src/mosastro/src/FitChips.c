# include "mosastro.h"

void FitChips (int Norder) {

  int i;

  for (i = 0; i < Nchip; i++) {
    chip[i].map.Npolyterms = Norder;
    FitChip (chip[i].raw, chip[i].ref, chip[i].Nmatch, &chip[i].map);
  }
  deproject_raw ();
  project_ref ();
}

void FitChip (StarData *raw, StarData *ref, int Nmatch, Coords *coords) {

  int i;

  fit_init (coords[0].Npolyterms);
  for (i = 0; i < Nmatch; i++) {
    if (raw[i].mask) continue;
    fit_add (raw[i].X, raw[i].Y, ref[i].L, ref[i].M);
  }
  fit_eval ();
  fit_apply_coords (coords);
}

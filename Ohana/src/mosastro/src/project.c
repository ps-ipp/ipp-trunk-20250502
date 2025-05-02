# include "mosastro.h"

int project_refcat (StarData *refcat, int Nrefcat) {

  SkyToTP (refcat, Nrefcat, &field.project);
  TPtoFP  (refcat, Nrefcat, &field.distort);

  return (1);
}

int deproject_stars () {

  int i;

  for (i = 0; i < Nchip; i++) {
    ChipToSky (chip[i].stars, chip[i].Nstars, &chip[i].coords);
  }
  return (1);
}

int project_stars () {

  int i;

  for (i = 0; i < Nchip; i++) {
    SkyToTP (chip[i].stars, chip[i].Nstars, &field.project);
    TPtoFP  (chip[i].stars, chip[i].Nstars, &field.distort);
  }
  return (1);
}

int deproject_raw () {

  int i;

  for (i = 0; i < Nchip; i++) {
    ChipToFP (chip[i].raw, chip[i].Nmatch, &chip[i].map);
    FPtoTP   (chip[i].raw, chip[i].Nmatch, &field.distort);
    TPtoSky  (chip[i].raw, chip[i].Nmatch, &field.project);
  }
  return (1);
}

int project_ref () {

  int i;

  for (i = 0; i < Nchip; i++) {
    SkyToTP  (chip[i].ref, chip[i].Nmatch, &field.project);
    TPtoFP   (chip[i].ref, chip[i].Nmatch, &field.distort);
    FPtoChip (chip[i].ref, chip[i].Nmatch, &chip[i].map);
  }
  return (1);
}

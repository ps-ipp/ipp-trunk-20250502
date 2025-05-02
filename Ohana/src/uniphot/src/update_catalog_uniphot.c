# include "uniphot.h"

// update a single catalog for uniphot
void update_catalog_uniphot (Catalog *catalog, Group *sgroup, int warn) {

  int i, j, m, found;
  int Nsec, Nsecfilt;
  PhotCode *code;

  Nsec = GetPhotcodeNsec (photcode[0].code);
  Nsecfilt = GetPhotcodeNsecfilt ();

  found = 0;    
  for (i = 0; i < catalog[0].Naverage; i++) {
    
    if (!isnan(catalog[0].secfilt[i*Nsecfilt+Nsec].MpsfChp)) {
      catalog[0].secfilt[i*Nsecfilt+Nsec].MpsfChp += sgroup[0].M;
    }

    m = catalog[0].average[i].measureOffset;
    for (j = 0; j < catalog[0].average[i].Nmeasure; j++, m++) {
      code = GetPhotcodebyCode (catalog[0].measure[m].photcode);
      if (code == NULL) continue;
      if (code[0].type != PHOT_DEP) continue;
      if (code[0].equiv != photcode[0].code) continue;
      catalog[0].measure[m].McalPSF  -= sgroup[0].M;
      catalog[0].measure[m].McalAPER -= sgroup[0].M;
      found ++;
    }
  }

  if (found) {
    fprintf (stderr, "found %d matches\n", found);
    if (warn) fprintf (stderr, "warning: updated values, perhaps out of range?\n");
  }
}

# include "fixhsc.h"

void update_catalog_fixhsc (Catalog *catalog) {

  for (int i = 0; i < catalog[0].Nmeasure; i++) {
    
    // we have a measure with a given photcode and time:
    e_time time = catalog[0].measure[i].t;

    int newbase = get_rules_fixhsc (time);
    if (!newbase) continue;
    
    unsigned short photcode = catalog[0].measure[i].photcode;

    // the rule is the new base photcode
    // this is robust against multiple passes
    short chipcode = photcode % 200;

    short newcode  = newbase + chipcode;
    catalog[0].measure[i].photcode = newcode;
  }
}

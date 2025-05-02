# include "fixhsc.h"

static int    Nrule = 0;
static e_time *startRule     = NULL;
static e_time *stopRule      = NULL;
static int    *photbaseRule  = NULL;

int load_rules_fixhsc (char *filename) {

  // the rule file contains
  // Start Stop Photbase

  FILE *f = fopen (filename, "r");
  if (!f) { fprintf (stderr, "file %s not found", filename); abort(); }

  int NRULE = 10000;

  ALLOCATE (startRule, e_time, NRULE);
  ALLOCATE (stopRule,  e_time, NRULE);
  ALLOCATE (photbaseRule, int, NRULE);

  double startMJD, stopMJD;

  // rules are in MJD, convert to e_time values
  while (fscanf (f, "%lf %lf %d", &startMJD, &stopMJD, &photbaseRule[Nrule]) != EOF) {

    startRule[Nrule] = ohana_mjd_to_sec (startMJD);
    stopRule[Nrule]  = ohana_mjd_to_sec (stopMJD);
    Nrule ++;

    if (Nrule >= NRULE) {
      NRULE += 10000;
      REALLOCATE (startRule, e_time, NRULE);
      REALLOCATE (stopRule,  e_time, NRULE);
      REALLOCATE (photbaseRule, int, NRULE);
    }
  }

  // dsortpair (ruleR, ruleD, Nrule); 

  return TRUE;
}

int get_rules_fixhsc (e_time time) {

  // find the rule that encompases this time, return the rule

  for (int i = 0; i < Nrule; i++) {
    if (time < startRule[i]) continue;
    if (time > stopRule[i]) continue;

    return photbaseRule[i];
  }

  // if we return false, no match was found, keep the old photcode
  return FALSE;

}

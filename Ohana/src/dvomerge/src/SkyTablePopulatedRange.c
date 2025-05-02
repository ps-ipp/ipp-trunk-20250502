# include "dvomerge.h"

int SkyTablePopulatedRange (off_t *ns, off_t *ne, SkyTable *sky, off_t Nstart) {

  off_t Ns, Ne;

  // given the starting sky region, find the populated range at or below this region

  Ns = Nstart;
  Ne = Nstart + 1;
  while (!sky[0].regions[Nstart].table) {
    Ns = sky[0].regions[Nstart].childS;
    Ne = sky[0].regions[Nstart].childE;
    if (Ns == 0) {
      fprintf (stderr, "no populated tables at an appropriate depth\n");
      exit (1);
    }
    Nstart = Ns;
  }

  *ns = Ns;
  *ne = Ne;
  return (TRUE);
}

int SkyListPopulatedRange (off_t *ns, off_t *ne, SkyList *sky, off_t Nstart) {

  off_t Ns, Ne;

  // given the starting sky region, find the populated range at or below this region

  Ns = Nstart;
  Ne = Nstart + 1;
  while (!sky[0].regions[Nstart][0].table) {
    Ns = sky[0].regions[Nstart][0].childS;
    Ne = sky[0].regions[Nstart][0].childE;
    if (Ns == 0) {
      fprintf (stderr, "no populated tables at an appropriate depth\n");
      exit (1);
    }
    Nstart = Ns;
  }

  *ns = Ns;
  *ne = Ne;
  return (TRUE);
}

SkyList *SkyTablePopulatedList (SkyTable *sky) {

  off_t i, N, NREGIONS;
  SkyList *list;

  N = 0;
  NREGIONS = 100;
  ALLOCATE (list, SkyList, 1);
  ALLOCATE (list[0].regions,  SkyRegion *, NREGIONS);
  ALLOCATE (list[0].filename,  char *, NREGIONS);
  list[0].Nregions = N;
  list[0].ownElements = FALSE; // this list is only holding a view to the elements

  // add populated entries in this set to the list
  for (i = 0; i < sky[0].Nregions; i++) {
    if (!sky[0].regions[i].table) continue;
    fprintf (stderr, "add: %s\n", sky[0].filename[i]);
    list[0].regions[N] = &sky[0].regions[i];
    list[0].filename[N] = sky[0].filename[i];
    N++;
    if (N >= NREGIONS) {
      NREGIONS += 100;
      REALLOCATE (list[0].regions, SkyRegion *, NREGIONS);
      REALLOCATE (list[0].filename, char *, NREGIONS);
    }
  }
  list[0].Nregions = N;
  return list;
}

SkyList *SkyTablePopulatedList_old (SkyTable *sky, off_t Ns, off_t Ne) {

  off_t i, ns, ne, N, NREGIONS;
  SkyList *list, *subset;

  N = 0;
  NREGIONS = 10;
  ALLOCATE (list, SkyList, 1);
  ALLOCATE (list[0].regions,  SkyRegion *, NREGIONS);
  ALLOCATE (list[0].filename,  char *, NREGIONS);
  list[0].Nregions = N;
  list[0].ownElements = FALSE; // this list is only holding a view to the elements

  // add populated entries in this set to the list
  for (i = Ns; i < Ne; i++) {
    if (!sky[0].regions[i].table) continue;
    fprintf (stderr, "add: %s\n", sky[0].filename[i]);
    list[0].regions[N] = &sky[0].regions[i];
    list[0].filename[N] = sky[0].filename[i];
    N++;
    if (N >= NREGIONS) {
      NREGIONS += 10;
      REALLOCATE (list[0].regions, SkyRegion *, NREGIONS);
      REALLOCATE (list[0].filename, char *, NREGIONS);
    }
  }
  list[0].Nregions = N;

  // add un-populated entries in this set to the list
  for (i = Ns; i < Ne; i++) {
    if (sky[0].regions[i].table) continue;

    ns = sky[0].regions[i].childS;
    ne = sky[0].regions[i].childE;
    if (ns == 0) {
      fprintf (stderr, "no populated tables at an appropriate depth\n");
      exit (1);
    }

    subset = SkyTablePopulatedList_old (sky, ns, ne);
    SkyListMerge (&list, subset);
    SkyListFree (subset);
  }
  return list;
}

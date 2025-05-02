# include "dvodist.h"
# define DEBUG 1

int AssignSkyToHost (SkyList *skylist, HostTable *table) {

  int i;
  long A, B;

  // init rnd seed
  A = time(NULL);
  for (B = 0; A == time(NULL); B++);
  srand48(B);

  for (i = 0; i < skylist->Nregions; i++) {
    
    // assign or re-assign? if a region is already assigned, do we keep it?

    // XXX some options;
# if (0)
    if (CLEAR_ERRORS && (skylist->regions[i]->hostFlags & DATA_COPY_FAILURE)) {
      skylist->regions[i]->hostID = 0;
    }
# endif

    // do not reassign tables; this lets us run this program multiple times for different regions
    // tables already assigned are left where they are
    if (skylist->regions[i]->hostID) {
      continue;
    }

    // assign all regions to one of the Nhost hosts
    float f = drand48();
    float n = table->Nhosts * f;
    int N = MIN(table->Nhosts - 1, n);
    skylist->regions[i]->hostID = table->hosts[N].hostID;
    // fprintf  (stderr, "%f  %f  %d  %d\n", f, n, N, skylist->regions[i]->hostID);
  }    

  return (TRUE);
}

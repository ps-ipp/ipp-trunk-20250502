# include "astro.h"

typedef struct {
  int *friend; // indices for all tested relationships
  float *dist; // distance for this relationship
  int Ndist;   // number of tested relationships
  int NDIST;   // number of allocated relationships
  int group;   // assigned group
  int Nfriend; // number of friends brought into group
} Object;

typedef struct {
  int *entry;
  int Nentry;
  int NENTRY;
} Group;

static Group  *group = NULL;
static int Ngroup = 0;
static int NGROUP = 0;

static Object *object = NULL;
static int Nobject = 0;
// static int NOBJECT = 0;

void add_to_group (int Nobj, float scale, int Nfriends) {

  int j;

  // fprintf (stderr, "add object %d to group %d\n", Nobj, Ngroup);

  object[Nobj].group = Ngroup;

  // add to this group
  int N = group[Ngroup].Nentry;
  group[Ngroup].entry[N] = Nobj;
  group[Ngroup].Nentry ++;
  CHECK_REALLOCATE (group[Ngroup].entry, int, group[Ngroup].NENTRY, group[Ngroup].Nentry, 100);

  // add all friends of this object (up to Nfriensd)
  // friends are already sorted by distance, so we add closest friends first
  for (j = 0; (object[Nobj].Nfriend < Nfriends) && (j < object[Nobj].Ndist); j++) {
    if (object[Nobj].dist[j] > scale) continue;

    int Nnew = object[Nobj].friend[j];

    if (object[Nnew].group != -1) continue;

    // found a friend : 
    object[Nobj].Nfriend ++;
    add_to_group (Nnew, scale, Nfriends);
  }
  return;
}

void sortvecset (opihi_flt *X, opihi_int *IDX1, opihi_int *IDX2, int N) {

# define SWAPFUNC(A,B){ opihi_flt tmp; opihi_int itmp; \
  tmp  = X[A];    X[A]    = X[B];    X[B]    = tmp; \
  itmp = IDX1[A]; IDX1[A] = IDX1[B]; IDX1[B] = itmp; \
  itmp = IDX2[A]; IDX2[A] = IDX2[B]; IDX2[B] = itmp; \
}

# define COMPARE(A,B)(X[A] < X[B])

  OHANA_SORT (N, COMPARE, SWAPFUNC);

# undef SWAPFUNC
# undef COMPARE

}

void sortfriends (float *X, int *IDX1, int N) {

# define SWAPFUNC(A,B){ float tmp; int itmp; \
  tmp  = X[A];    X[A]    = X[B];    X[B]    = tmp; \
  itmp = IDX1[A]; IDX1[A] = IDX1[B]; IDX1[B] = itmp; \
}

# define COMPARE(A,B)(X[A] < X[B])

  OHANA_SORT (N, COMPARE, SWAPFUNC);

# undef SWAPFUNC
# undef COMPARE

}

int mkclusters (int argc, char **argv) {
  
  int i, j;
  Vector *index1, *index2, *distance;

  if (argc != 6) goto usage;

  if ((index1   = SelectVector (argv[1], OLDVECTOR, TRUE)) == NULL) goto escape;
  if ((index2   = SelectVector (argv[2], OLDVECTOR, TRUE)) == NULL) goto escape;
  if ((distance = SelectVector (argv[3], OLDVECTOR, TRUE)) == NULL) goto escape;
  float scale = atof (argv[4]);
  int Nfriends = atoi (argv[5]);

  // XXX enforce matching lengths on the three vectors

  CastVector (index1, OPIHI_INT);
  CastVector (index2, OPIHI_INT);

  Nobject = 0;
  for (i = 0; i < index1->Nelements; i++) {
    Nobject = MAX (Nobject, index1->elements.Int[i]);
    Nobject = MAX (Nobject, index2->elements.Int[i]);
  }
  Nobject ++;  // after the loop, Nobject has the value of the highest index, not the count

  // allocate the list of Object -- these list the possible friends of the given object[i] 
  ALLOCATE (object, Object, Nobject);
  for (i = 0; i < Nobject; i++) {
    ALLOCATE (object[i].friend, int, Nobject);
    memset (object[i].friend, 0, Nobject*sizeof(int));
    ALLOCATE (object[i].dist, float, Nobject);
    memset (object[i].dist, 0, Nobject*sizeof(float));
    object[i].NDIST = Nobject;
    object[i].Ndist = 0;
    object[i].group = -1; // unassigned
    object[i].Nfriend = 0; // no friends yet
  }

  // generate the set of vectors of all distances for each entry
  // the object seq number (object[i]) will be used to match the index1 and index2 values
  for (i = 0; i < index1->Nelements; i++) {
    int N, m;
    N = index1->elements.Int[i];
    if (N >= Nobject) abort();
    m = object[N].Ndist;
    object[N].friend[m] = index2->elements.Int[i];
    object[N].dist[m] = distance->elements.Flt[i];
    object[N].Ndist ++;
    if (object[N].Ndist == object[N].NDIST) {
      object[N].NDIST += 100;
      REALLOCATE (object[N].dist, float, object[N].NDIST);
      REALLOCATE (object[N].friend, int, object[N].NDIST);
    }

    N = index2->elements.Int[i];
    if (N >= Nobject) abort();
    m = object[N].Ndist;
    object[N].friend[m] = index1->elements.Int[i];
    object[N].dist[m] = distance->elements.Flt[i];
    object[N].Ndist ++;
    if (object[N].Ndist == object[N].NDIST) {
      object[N].NDIST += 100;
      REALLOCATE (object[N].dist, float, object[N].NDIST);
      REALLOCATE (object[N].friend, int, object[N].NDIST);
    }
  }

  for (i = 0; i < Nobject; i++) {
    sortfriends (object[i].dist, object[i].friend, object[i].Ndist);
  }

  // generate the set of groups. each group is a list of friends and their friends
  Ngroup = 0;
  NGROUP = Nobject;
  ALLOCATE (group, Group, NGROUP);
  for (i = 0; i < NGROUP; i++) {
    group[i].Nentry = 0;
    group[i].NENTRY = 100;
    ALLOCATE (group[i].entry, int, group[i].NENTRY);
  }

  sortvecset (distance->elements.Flt, index1->elements.Int, index2->elements.Int, index2->Nelements);
  
  for (i = 0; i < distance->Nelements; i++) {
    int Nnew;
    int found;

    if (distance->elements.Flt[i] > scale) continue;

    found = FALSE;

    Nnew = index1->elements.Int[i];
    if (object[Nnew].group == -1) {
      add_to_group (Nnew, scale, Nfriends);
      // fprintf (stderr, "new group %d part 1 with %d elements\n", Ngroup, group[Ngroup].Nentry);
      found = TRUE;
    }

    Nnew = index2->elements.Int[i];
    if (object[Nnew].group == -1) {
      add_to_group (Nnew, scale, Nfriends);
      // fprintf (stderr, "new group %d part 1 with %d elements\n", Ngroup, group[Ngroup].Nentry);
      found = TRUE;
    }

    if (found) {
      fprintf (stderr, "group %d with %d elements\n", Ngroup, group[Ngroup].Nentry);
      Ngroup ++;
      if (Ngroup >= NGROUP) abort();
    }
  }

  if (1) {
    int Nmiss = 0;
    for (i = 0; i < Nobject; i++) {
      if (object[i].group > -1) continue;
      Nmiss ++;
      // fprintf (stderr, "object %d not assigned\n", i);
      for (j = 0; FALSE && j < object[i].Ndist; j++) {
	fprintf (stderr, "friend %d, dist %f\n", object[i].friend[j], object[i].dist[j]);
      }
    }
    fprintf (stderr, "%d objects not assigned\n", Nmiss);
  } 

  return TRUE;
  
 escape: 
  gprint (GP_ERR, "invalid vector\n");
  return FALSE;
  
usage:
  gprint (GP_ERR, "USAGE: mkclusters (index1) (index2) (distance) scale\n");
  return FALSE;
}

/* this function takes 3 vectors: a set of distances and a pair of indicies identifying the end points.  
   
   the goal is to generate groups of the end points that are relatively closer.

   note that the distances may not be Euclidean: nothing can be assumed about relationships between d(1,2), d(2,3), and d(1,3)

   the indicies should be sequential.  the algorithm does not require it, but storage is much cleaner if it is

*/

# include <stdio.h>
# include <stdlib.h>
# include <stdarg.h>
# include <stdint.h>
# include <pthread.h>
# include <string.h>
# include <errno.h>
# include <malloc.h>
# include <sys/mman.h>

# define TEST_SAVE_FREE_BLOCKS 0
# define TEST_SAVE_DROP_BLOCKS 0
# define USE_MMAP_SIZE 0x80000000
# define USE_MMAP_LOCAL FALSE
/* # define USE_MMAP_SIZE 0x100000 - 1M */
/* USE_MMAP_SIZE is used to tell ohana memory functions to switch to raw mmap/munmap call
   instead of malloc / realloc / free.  This is in an attempt to avoid a poorly-understood
   bug in malloc which I've identfied.  The bug is seen in programs running on linux
   kernel 3.7.6 for glibc 2.8.
   This did not work as hoped; instead I've had to prevent the use of MMAP all together by 
   setting M_MMAP_MAX to 0 with mallopt
*/

# undef OHANA_MEMORY
# include "ohana_allocate.h"

/* need an internal version that does not use ohana_memory functions */
# define FALSE 0 
# define TRUE 1
# define MAX(X,Y) ((X) > (Y) ? (X) : (Y))

// a no-op to mark unused parameters in a function
# define OHANA_UNUSED_PARAM(x)(void)(x)

static OhanaMemblock *lastBlock = NULL;
static int Nblock = 0;

# if (TEST_SAVE_FREE_BLOCKS) 
static OhanaMemblock *freeBlock = NULL;
# endif

# if (TEST_SAVE_DROP_BLOCKS) 
static OhanaMemblock *dropBlock = NULL;
# endif

static pthread_mutex_t memBlockListMutex = PTHREAD_MUTEX_INITIALIZER;

static int NblockMaxDump = 100;

void ohana_meminit () {
  return;
}

void ohana_memabort (char *format, ...) {
  va_list argp;  

  va_start (argp, format);
  vfprintf (stderr, format, argp);
  va_end (argp);
  abort();
}


/* these are the measured values, but below we round the ends up and down
# define PS_BAD_MALLOC_RANGE_1_MIN 39637060
# define PS_BAD_MALLOC_RANGE_1_MAX 39649156

# define PS_BAD_MALLOC_RANGE_2_MIN 79288100
# define PS_BAD_MALLOC_RANGE_2_MAX 79294312
*/

# define PS_BAD_MALLOC_RANGE_1_MIN 39636000
# define PS_BAD_MALLOC_RANGE_1_MAX 39650000

# define PS_BAD_MALLOC_RANGE_2_MIN 79288000
# define PS_BAD_MALLOC_RANGE_2_MAX 79298000

void *ohana_malloc (const char *file, int line, const char *func, size_t Nelem, size_t esize) {

  // we want to call mallopt if we have not yet used the memory system
  // this disables the use of mmap by malloc and forces it to use on sbrk
  pthread_mutex_lock(&memBlockListMutex);
  if (!lastBlock) {
    mallopt (M_MMAP_MAX, 0);
  }
  pthread_mutex_unlock(&memBlockListMutex);

  char *ptr;		      // actual user memory allocated
  OhanaMemblock *new;	      // new memblock created to track the user memory

  size_t myNelem = MAX (1, Nelem);
  size_t size = myNelem * esize; // total number of bytes requested (not items)
  size_t fullSize = sizeof(OhanaMemblock) + size + 2*sizeof(void *);   // total size is : memblock + data + endpost
  // if (size % 8) size += (8 - size % 8); // do NOT round to 8-byte boundary (did not fix corruption)

  // for gcc 4.3.2, linux 3.7.6 (at least) there are bad malloc sizes.  if a request
  // is made for one of these bad ranges, actually allocate a larger amount 
  if ((fullSize > PS_BAD_MALLOC_RANGE_1_MIN) && (fullSize < PS_BAD_MALLOC_RANGE_1_MAX)) { fullSize = PS_BAD_MALLOC_RANGE_1_MAX; }
  if ((fullSize > PS_BAD_MALLOC_RANGE_2_MIN) && (fullSize < PS_BAD_MALLOC_RANGE_2_MAX)) { fullSize = PS_BAD_MALLOC_RANGE_2_MAX; }

  // this was another attempt to fix corruption problems probably due to the gcc error described above
  if (USE_MMAP_LOCAL && (fullSize > USE_MMAP_SIZE)) {
    fprintf (stderr, "** mmap %ld (%s, %d, %s)\n", fullSize, file, line, func);
    new = (OhanaMemblock *) mmap (NULL, fullSize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (new == (void *) -1) ohana_memabort ("failed to allocate memory with mmap (%s, %d, %s)\n", file, line, func);
  } else {
    new = (OhanaMemblock *) malloc (fullSize); 
    if (new == NULL) ohana_memabort ("failed to allocate memory (%s, %d, %s)\n", file, line, func);
  }
  // if (errno == ENOMEM) abort(); I should not check the value of errno if new is not NULL

  // pointer to the start of the user memory
  ptr = (char *)(new + 1);

  // set the end-post values
  new->startblock = OHANA_MEMMAGIC;
  new->endblock = OHANA_MEMMAGIC;
  *(uint32_t *)(ptr + size) = OHANA_MEMMAGIC;

  // set the memory metadata
  new->size = size;
  new->file = file;
  new->line = line;
  new->func = func;
  new->freed = FALSE;
  new->Nalloc = 1;

  // poison the whole memory: we should not rely of 0'ed memory
  memset (ptr, 0x7f, new->size);

  // new memblock becomes the 'lastBlock':
  // lastBlock = new
  // new->prev = (new - 1)
  // (new - 1)->next = new

  // for memory allocated in the order m0, m1, m2
  // m0->next = m1, m1->next = m2, m2->next = NULL
  // m2->prev = m1, m1->prev = m0, m0->prev = NULL
  // lastBlock = m2

  // set up the pointers
  new->nextBlock = NULL;

  // protect the list during this operation
  pthread_mutex_lock(&memBlockListMutex);

  if (lastBlock) {
    lastBlock->nextBlock = new;
  }
  new->prevBlock = lastBlock;
  lastBlock = new;

  Nblock ++;
  pthread_mutex_unlock(&memBlockListMutex);

  if (!new->nextBlock && !new->prevBlock && (Nblock > 1)) abort();

  return (ptr);
}

void *ohana_realloc (const char *file, int line, const char *func, void *in, size_t Nelem, size_t esize) {

  OhanaMemblock *old;	      // original memblock associated with user memory
  OhanaMemblock *new;	      // new memblock associated with user memory
  size_t size;

  // just allocate if not previously allocated
  if (!in) {
    void *ptr = ohana_malloc (file, line, func, Nelem, esize);
    return ptr;
  }

  // memblock of supplied pointer
  old = (OhanaMemblock *) in - 1;

  if (old->startblock != OHANA_MEMMAGIC) ohana_memabort ("corrupt memory (%s, %d, %s)\n", file, line, func);
  if (old->endblock != OHANA_MEMMAGIC) ohana_memabort ("corrupt memory (%s, %d, %s)\n", file, line, func);

  Nelem = MAX (1, Nelem);
  size = Nelem * esize;
  // if (size % 8) size += (8 - size % 8); // do NOT round to 8-byte boundary (did not fix corruption)

  // requested same size as current allocation
  if (size == old->size) {
    return in;
  }
  size_t oldsize = old->size;

  // total size is : memblock + data + endpost
  size_t fullSize = sizeof(OhanaMemblock) + size + 2*sizeof(void *);
  size_t oldFullSize = sizeof(OhanaMemblock) + oldsize + 2*sizeof(void *);

  // for gcc 4.3.2, linux 3.7.6 (at least) there are bad malloc sizes.  if a request
  // is made for one of these bad ranges, actually allocate a larger amount 
  if ((fullSize > PS_BAD_MALLOC_RANGE_1_MIN) && (fullSize < PS_BAD_MALLOC_RANGE_1_MAX)) { fullSize = PS_BAD_MALLOC_RANGE_1_MAX; }
  if ((fullSize > PS_BAD_MALLOC_RANGE_2_MIN) && (fullSize < PS_BAD_MALLOC_RANGE_2_MAX)) { fullSize = PS_BAD_MALLOC_RANGE_2_MAX; }
  if ((oldFullSize > PS_BAD_MALLOC_RANGE_1_MIN) && (oldFullSize < PS_BAD_MALLOC_RANGE_1_MAX)) { oldFullSize = PS_BAD_MALLOC_RANGE_1_MAX; }
  if ((oldFullSize > PS_BAD_MALLOC_RANGE_2_MIN) && (oldFullSize < PS_BAD_MALLOC_RANGE_2_MAX)) { oldFullSize = PS_BAD_MALLOC_RANGE_2_MAX; }

  pthread_mutex_lock(&memBlockListMutex);

  OhanaMemblock *nextBlock = old->nextBlock;
  OhanaMemblock *prevBlock = old->prevBlock;

  int isLast = (old == lastBlock);

  // ask for new memory

# if (TEST_SAVE_DROP_BLOCKS) 
  // XXX for a test, we are going to always alloc a new block, copy the old data to the new block
  // poison the old block, then free it
  if (USE_MMAP_LOCAL && (fullSize > USE_MMAP_SIZE)) {
    fprintf (stderr, "** mmap %ld (%s, %d, %s)\n", fullSize, file, line, func);
    new = (OhanaMemblock *) mmap (NULL, fullSize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (new == (void *) -1) ohana_memabort ("failed to allocate memory with mmap (%s, %d, %s)\n", file, line, func);
  } else {
    new = (OhanaMemblock *) malloc (fullSize);
    if (new == NULL) ohana_memabort ("failed to reallocate memory (%s, %d, %s)\n", file, line, func);
  }
  char *ptr_new = (char *) (new + 1);
  char *ptr_old = (char *) (old + 1);
  size_t copy_bytes = old->size < size ? old->size : size;
  memcpy (ptr_new, ptr_old, copy_bytes);
  memset (ptr_old, 0x7f, old->size);
  new->nextBlock = old->nextBlock;
  new->prevBlock = old->prevBlock;
  new->startblock = OHANA_MEMMAGIC;
  new->endblock = OHANA_MEMMAGIC;
  new->freed = FALSE;
# else
  // ask for new memory

  if (USE_MMAP_LOCAL && (fullSize > USE_MMAP_SIZE)) {
    if (oldFullSize > USE_MMAP_SIZE) {
      // new = (OhanaMemblock *) mremap (old, oldFullSize, fullSize, 0); NOTE: mremap is a Linux / GNU only feature
      // mmap to mmap (allocate new, copy user bytes, free old)
      fprintf (stderr, "** mmap 1 %ld to %ld (%s, %d, %s)\n", oldFullSize, fullSize, file, line, func);
      new = (OhanaMemblock *) mmap (NULL, fullSize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
      if (new == (void *) -1) ohana_memabort ("failed to allocate memory with mmap (%s, %d, %s)\n", file, line, func);
      char *ptr_new = (char *) (new + 1);
      char *ptr_old = (char *) (old + 1);
      size_t copy_bytes = old->size < size ? old->size : size;
      memcpy (ptr_new, ptr_old, copy_bytes);
      memset (ptr_old, 0x7f, old->size);
      new->nextBlock = old->nextBlock;
      new->prevBlock = old->prevBlock;
      new->startblock = OHANA_MEMMAGIC;
      new->endblock = OHANA_MEMMAGIC;
      new->freed = FALSE;
      fprintf (stderr, "** munmap %ld (%s, %d, %s)\n", oldFullSize, file, line, func);
      int mstatus = munmap (old, oldFullSize); // XXX ???
      if (mstatus != 0) ohana_memabort ("failed to reallocate memory (%s, %d, %s)\n", file, line, func);
    } else {
      // malloc to mmap (allocate new, copy user bytes, free old)
      fprintf (stderr, "** mmap 2 %ld to %ld (%s, %d, %s)\n", oldFullSize, fullSize, file, line, func);
      new = (OhanaMemblock *) mmap (NULL, fullSize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
      if (new == (void *) -1) ohana_memabort ("failed to allocate memory with mmap (%s, %d, %s)\n", file, line, func);
      char *ptr_new = (char *) (new + 1);
      char *ptr_old = (char *) (old + 1);
      size_t copy_bytes = old->size < size ? old->size : size;
      memcpy (ptr_new, ptr_old, copy_bytes);
      memset (ptr_old, 0x7f, old->size);
      new->nextBlock = old->nextBlock;
      new->prevBlock = old->prevBlock;
      new->startblock = OHANA_MEMMAGIC;
      new->endblock = OHANA_MEMMAGIC;
      new->freed = FALSE;
      free (old); // XXX ???
    } 
  } else { 
    if (USE_MMAP_LOCAL && (oldFullSize > USE_MMAP_SIZE)) {
      // mmap to malloc (allocate new, copy user bytes, free old)
      new = (OhanaMemblock *) malloc (fullSize);
      if (new == NULL) ohana_memabort ("failed to reallocate memory (%s, %d, %s)\n", file, line, func);
      char *ptr_new = (char *) (new + 1);
      char *ptr_old = (char *) (old + 1);
      size_t copy_bytes = old->size < size ? old->size : size;
      memcpy (ptr_new, ptr_old, copy_bytes);
      memset (ptr_old, 0x7f, old->size);
      new->nextBlock = old->nextBlock;
      new->prevBlock = old->prevBlock;
      new->startblock = OHANA_MEMMAGIC;
      new->endblock = OHANA_MEMMAGIC;
      new->freed = FALSE;
      fprintf (stderr, "** munmap %ld (%s, %d, %s)\n", oldFullSize, file, line, func);
      int mstatus = munmap (old, oldFullSize); // XXX ???
      if (mstatus != 0) ohana_memabort ("failed to reallocate memory (%s, %d, %s)\n", file, line, func);
    } else {
      // mmap to mmap
      new = (OhanaMemblock *) realloc (old, fullSize);
      if (new == NULL) ohana_memabort ("failed to reallocate memory (%s, %d, %s)\n", file, line, func);
    }
  }
  char *ptr_new = (char *) (new + 1);
# endif
  
  if (new->startblock != OHANA_MEMMAGIC) ohana_memabort ("corrupt memory (%s, %d, %s)\n", file, line, func);
  if (new->endblock != OHANA_MEMMAGIC) ohana_memabort ("corrupt memory (%s, %d, %s)\n", file, line, func);

  // set the memory metadata
  new->size = size; 
  new->file = file;
  new->line = line;
  new->func = func;
  new->Nalloc++;

  // update the endpost (the others were set originally and maintained through the realloc)
  *(uint32_t *)(ptr_new + size) = OHANA_MEMMAGIC;

  // poison the new memory: we should not rely of 0'ed memory
  if (new->size > oldsize) {
    memset (&ptr_new[oldsize], 0x7f, new->size - oldsize);
  }

  // need to reset lastBlock in case we moved
  if (isLast) {
    lastBlock = new;
  }

  // need to adjust the neighbors' pointers:
  if (nextBlock) {
    nextBlock->prevBlock = new;
  }
  if (prevBlock) {
    prevBlock->nextBlock = new;
  }
  
  // XXX FOR TESTING, save the realloc blocks
# if (TEST_SAVE_DROP_BLOCKS)
  if (dropBlock) {
    dropBlock->nextBlock = old;
  }
  old->nextBlock = NULL;
  old->prevBlock = dropBlock;
  dropBlock = old;
# endif

  pthread_mutex_unlock(&memBlockListMutex);

  return (void *) ptr_new;
}

// XXX this function is only used for memory allocated outside of the ohana_allocate system
void real_free (void *in) {

  if (!in) return;
  free (in);
  return;
}

// this is very slow.  should we speed this up by indexing on the ptr?
void ohana_free (const char *file, int line, const char *func, void *in) {

  OhanaMemblock *ref;

  if (!in) return;

  if (!lastBlock) ohana_memabort ("no memory allocated yet (%s@%d : %s)\n", file, line, func);

  // fprintf (stderr, "free %zx\n", (size_t) in);

  ref = (OhanaMemblock *) in - 1;
  if (!ref->nextBlock && !ref->prevBlock && (Nblock > 1)) ohana_memabort ("orphan block?? (%s@%d : %s)\n", file, line, func);
  size_t fullSize = sizeof(OhanaMemblock) + ref->size + 2*sizeof(void *);   // total size is : memblock + data + endpost

  if (ref->freed) ohana_memabort ("memory already freed (%s, %d, %s [%d bytes])\n", ref->file, ref->line, ref->func, ref->size);
  // if (!ref->nextBlock && !ref->prevBlock && (Nblock > 1)) ohana_memabort ("orphan block?? (%s@%d : %s)\n", file, line, func);

  ref->freed = TRUE;
  if (!ref->nextBlock && !ref->prevBlock && (Nblock > 1)) ohana_memabort ("orphan block?? (%s@%d : %s)\n", file, line, func);

  // fprintf (stderr, "  file: %s, line: %d, func: %s, size: %zd, addr: %zx\n", 
  // ref->file, ref->line, ref->func, ref->size, (size_t) ref);

  if (!lastBlock) ohana_memabort ("corruption? (%s@%d : %s)\n", file, line, func);
  if (!ref->nextBlock && !ref->prevBlock && (Nblock > 1)) ohana_memabort ("orphan block?? (%s@%d : %s)\n", file, line, func);

  pthread_mutex_lock(&memBlockListMutex);
  
  OhanaMemblock *nextBlock = ref->nextBlock;
  OhanaMemblock *prevBlock = ref->prevBlock;

  if (!nextBlock && !prevBlock && (Nblock > 1)) ohana_memabort ("orphan block?? (%s@%d : %s)\n", file, line, func);

  // remove this memBlock from the list
  if (nextBlock) {
    nextBlock->prevBlock = prevBlock;
  }
  if (prevBlock) {
    prevBlock->nextBlock = nextBlock;
  }
  if (lastBlock == ref) {
    lastBlock = prevBlock;
  }
  
  if (!lastBlock && (Nblock > 1)) ohana_memabort ("corruption? (%s@%d : %s)\n", file, line, func);

  // XXX FOR TESTING, save the freed blocks
# if (TEST_SAVE_FREE_BLOCKS)
  if (freeBlock) {
    freeBlock->nextBlock = ref;
  }
  ref->nextBlock = NULL;
  ref->prevBlock = freeBlock;
  freeBlock = ref;
# endif

  // pointer to the start of the user memory
  char *ptr = (char *)(ref + 1);
  memset (ptr, 0x7f, ref->size);

  Nblock --;
  if (Nblock < 0) ohana_memabort ("excess frees? (%s@%d : %s)\n", file, line, func);

  pthread_mutex_unlock(&memBlockListMutex);
  
# if (!TEST_SAVE_FREE_BLOCKS)
  memset (ref, 0x7f, sizeof(OhanaMemblock));
  if (USE_MMAP_LOCAL && (fullSize > USE_MMAP_SIZE)) {
    fprintf (stderr, "** munmap %ld (%s, %d, %s)\n", fullSize, file, line, func);
    int mstatus = munmap (ref, fullSize);
    if (mstatus != 0) ohana_memabort ("failed to free memory (%s, %d, %s)\n", file, line, func);
  } else {
    free (ref);
  }
# endif

  return;
}

int ohana_memcheck_noop (int allmemory) {
  OHANA_UNUSED_PARAM(allmemory);
  return TRUE;
}

int ohana_memcheck_func (const char *myFile, int myLine, const char *myFunc, int VERBOSE) {

  if (!lastBlock) {
    if (VERBOSE) fprintf (stderr, "no memory allocated\n");
  }

  OhanaMemblock *thisBlock = lastBlock;

  size_t Ngood  = 0;
  size_t Nbad   = 0;
  size_t Ntotal = 0;
  size_t Nbytes = 0;
  int status = TRUE;

  while (thisBlock) {

    int good = TRUE;
    
    if (thisBlock->startblock != OHANA_MEMMAGIC) good = FALSE;
    if (thisBlock->endblock != OHANA_MEMMAGIC) good = FALSE;

    // pointer to the start of the user memory
    char *ptr = (char *)(thisBlock + 1) + thisBlock->size;
    uint32_t endpost = *(uint32_t *)ptr;
    if (endpost != OHANA_MEMMAGIC) good = FALSE;

    // XXX keep checking even if memory is corrupted?
    if (!good) {
      if (Nbad < 1) {
	fprintf (stderr, "memory corruption\n");
      }
      if (Nbad < NblockMaxDump) {
	fprintf (stderr, "  file: %s, line: %d, func: %s\n", thisBlock->file, thisBlock->line, thisBlock->func);
      }
      Nbad ++;
    } else {
      Ngood ++;
    }
    Ntotal ++;
    Nbytes += thisBlock->size;

    thisBlock = thisBlock->prevBlock;
  }

  if (Ntotal || VERBOSE) {
    fprintf (stderr, "%zd memory blocks allocated (%zd bytes total), %zd good, %zd bad ", Ntotal, Nbytes, Ngood, Nbad);
    fprintf (stderr, "@ %s:%d (%s)\n", myFile, myLine, myFunc);
  }

  if (Nbad) status = FALSE;

# if (TEST_SAVE_FREE_BLOCKS) 
 
  thisBlock = freeBlock;

  size_t Ngood_free  = 0;
  size_t Nbad_free   = 0;
  size_t Ntotal_free = 0;
  size_t Nbytes_free = 0;

  while (thisBlock) {

    int i;
    int good = TRUE;
    
    if (thisBlock->startblock != OHANA_MEMMAGIC) good = FALSE;
    if (thisBlock->endblock != OHANA_MEMMAGIC) good = FALSE;

    // pointer to the end of the user memory
    char *ptr = (char *)(thisBlock + 1) + thisBlock->size;
    uint32_t endpost = *(uint32_t *)ptr;
    if (endpost != OHANA_MEMMAGIC) good = FALSE;

    // pointer to the start of the user memory
    ptr = (char *)(thisBlock + 1);
    for (i = 0; i < thisBlock->size; i++, ptr++) {
      if (*ptr != 0x77) good = FALSE;
    }

    // XXX keep checking even if memory is corrupted?
    if (!good) {
      if (Nbad_free < 1) {
	fprintf (stderr, "memory corruption\n");
      }
      if (Nbad_free < 100) {
	fprintf (stderr, "  file: %s, line: %d, func: %s\n", thisBlock->file, thisBlock->line, thisBlock->func);
      }
      Nbad_free ++;
    } else {
      Ngood_free ++;
    }
    Ntotal_free ++;
    Nbytes_free += thisBlock->size;

    thisBlock = thisBlock->prevBlock;
  }

  if (Ntotal_free || VERBOSE) {
    fprintf (stderr, "%zd memory blocks freed     (%zd bytes total), %zd good, %zd bad\n", Ntotal_free, Nbytes_free, Ngood_free, Nbad_free);
  }

  if (Nbad_free) status = FALSE;

# endif
 
# if (TEST_SAVE_DROP_BLOCKS)
  thisBlock = dropBlock;

  size_t Ngood_drop  = 0;
  size_t Nbad_drop   = 0;
  size_t Ntotal_drop = 0;
  size_t Nbytes_drop = 0;

  while (thisBlock) {

    int i;
    int good = TRUE;
    
    if (thisBlock->startblock != OHANA_MEMMAGIC) good = FALSE;
    if (thisBlock->endblock != OHANA_MEMMAGIC) good = FALSE;

    // pointer to the end of the user memory
    char *ptr = (char *)(thisBlock + 1) + thisBlock->size;
    uint32_t endpost = *(uint32_t *)ptr;
    if (endpost != OHANA_MEMMAGIC) good = FALSE;

    // pointer to the start of the user memory
    ptr = (char *)(thisBlock + 1);
    for (i = 0; i < thisBlock->size; i++, ptr++) {
      if (*ptr != 0x7f) good = FALSE;
    }

    // XXX keep checking even if memory is corrupted?
    if (!good) {
      if (Nbad_drop < 1) {
	fprintf (stderr, "memory corruption\n");
      }
      if (Nbad_drop < 100) {
	fprintf (stderr, "  file: %s, line: %d, func: %s\n", thisBlock->file, thisBlock->line, thisBlock->func);
      }
      Nbad_drop ++;
    } else {
      Ngood_drop ++;
    }
    Ntotal_drop ++;
    Nbytes_drop += thisBlock->size;

    thisBlock = thisBlock->prevBlock;
  }

  if (Ntotal_drop || VERBOSE) {
    fprintf (stderr, "%zd memory blocks dropped   (%zd bytes total), %zd good, %zd bad", Ntotal_drop, Nbytes_drop, Ngood_drop, Nbad_drop);
    fprintf (stderr, "@ %s:%d (%d)\n", myFile, myLine, myFunc);
  }

  if (Nbad_drop) status = FALSE;
# endif
 
  return status;
}

void ohana_memcheck_block_func (const char *myFile, int myLine, const char *myFunc, void *in) {

  OhanaMemblock *ref;

  if (!in) return;

  if (!lastBlock) ohana_memabort ("no memory allocated yet (%s@%d : %s)\n", myFile, myLine, myFunc);

  ref = (OhanaMemblock *) in - 1;

  if (ref->freed) ohana_memabort ("memory already freed (%s, %d, %s [%d bytes])\n", ref->file, ref->line, ref->func, ref->size);

  OhanaMemblock *nextBlock = ref->nextBlock;
  OhanaMemblock *prevBlock = ref->prevBlock;

  if (!nextBlock && !prevBlock) ohana_memabort ("orphan block?? (%s@%d : %s)\n", myFile, myLine, myFunc);
}

void ohana_memdump_file (FILE *f, int VERBOSE) {

  if (!lastBlock) {
    if (VERBOSE) fprintf (f, "no memory allocated\n");
    return;
  }

  OhanaMemblock *thisBlock = lastBlock;

  size_t Ntotal = 0;
  size_t Nbytes = 0;

  fprintf (f, " entry | bytes | cumulative | STATUS | file | line | function\n");

  while (thisBlock) {

    int good = TRUE;
    
    if (thisBlock->startblock != OHANA_MEMMAGIC) good = FALSE;
    if (thisBlock->endblock != OHANA_MEMMAGIC) good = FALSE;

    // pointer to the start of the user memory
    char *ptr = (char *)(thisBlock + 1) + thisBlock->size;
    uint32_t endpost = *(uint32_t *)ptr;
    if (endpost != OHANA_MEMMAGIC) good = FALSE;

    // XXX keep checking even if memory is corrupted?
    Ntotal ++;
    Nbytes += thisBlock->size;

    if (Ntotal < NblockMaxDump) {
      if (good) {
	fprintf (f, "  %zd  %zd  %zd  GOOD  %s %d, func: %s\n", Ntotal, thisBlock->size, Nbytes, thisBlock->file, thisBlock->line, thisBlock->func);
      } else {
	fprintf (f, "  %zd  %zd  %zd  BAD   %s %d, func: %s\n", Ntotal, thisBlock->size, Nbytes, thisBlock->file, thisBlock->line, thisBlock->func);
      }
    }

    thisBlock = thisBlock->prevBlock;
  }

  if (Ntotal || VERBOSE) {
    fprintf (f, "%zd memory blocks allocated (%zd bytes total)\n", Ntotal, Nbytes);
  }

  return;
}

void ohana_memdump_func (int VERBOSE) {
  ohana_memdump_file (stderr, VERBOSE);
  return;
}

OhanaMemstats ohana_memstats (int allmemory) {
  OHANA_UNUSED_PARAM(allmemory);

  OhanaMemstats memstats;

  memstats.exists = 0;
  memstats.Ntotal = 0;
  memstats.Nbytes = 0;
  memstats.Ngood = 0;
  memstats.Nbad = 0;
  
  OhanaMemblock *thisBlock = lastBlock;

  while (thisBlock) {

    memstats.exists = TRUE;

    int good = TRUE;
    
    if (thisBlock->startblock != OHANA_MEMMAGIC) good = FALSE;
    if (thisBlock->endblock != OHANA_MEMMAGIC) good = FALSE;

    // pointer to the start of the user memory
    char *ptr = (char *)(thisBlock + 1) + thisBlock->size;
    uint32_t endpost = *(uint32_t *)ptr;
    if (endpost != OHANA_MEMMAGIC) good = FALSE;

    memstats.Ntotal ++;
    memstats.Nbytes += thisBlock->size;

    if (good) {
      memstats.Ngood ++;
    } else {
      memstats.Nbad ++;
    }

    thisBlock = thisBlock->prevBlock;
  }

  return memstats;
}

void ohana_memdump_strings_file (FILE *f, int VERBOSE) {

  if (!lastBlock) {
    if (VERBOSE) fprintf (f, "no memory allocated\n");
    return;
  }

  OhanaMemblock *thisBlock = lastBlock;

  size_t Ntotal = 0;
  size_t Nbytes = 0;
  size_t Nstring = 0;

  fprintf (f, "    entry |    bytes | cumulative : line : string\n");

  while (thisBlock) {

    // pointer to the start of the user memory
    char *ptr = (char *)(thisBlock + 1);

    Ntotal ++;
    Nbytes += thisBlock->size;

    int N = strlen(thisBlock->file);
    if (N > 22) {
      int found = !strcmp("libohana/src/string.c", &thisBlock->file[N - 21]);
      if (found) {
	if (Nstring < NblockMaxDump) {
	  fprintf (f, " %8zd | %8zd | %10zd : %4d : %s\n", Ntotal, thisBlock->size, Nbytes, thisBlock->line, ptr);
	}
	Nstring ++;
      }
      thisBlock = thisBlock->prevBlock;
    }
  }

  if (Nstring || VERBOSE) {
    fprintf (f, "%zd strings allocated\n", Nstring);
  }

  return;
}

void ohana_memdump_set_maxlines (int N) {
  NblockMaxDump = N;
}

void ohana_memblock_show_stats (void *in) {

  OhanaMemblock *ref;

  if (!in) { fprintf (stderr, "void pointer\n"); return; }

  if (!lastBlock) { fprintf (stderr, "no memory allocated yet\n"); return; }

  ref = (OhanaMemblock *) in - 1;

  fprintf (stderr, "file: %s\n", ref->file);
  fprintf (stderr, "line: %d\n", ref->line);
  fprintf (stderr, "func: %s\n", ref->func);
  
  fprintf (stderr, "size: %zd\n", ref->size);

  fprintf (stderr, "start post: 0x%08x\n", ref->startblock);
  fprintf (stderr, "end  block: 0x%08x\n", ref->endblock);

  // pointer to the start of the user memory
  char *ptr = (char *)(in) + ref->size;
  uint32_t endpost = *(uint32_t *) ptr;

  fprintf (stderr, "end   post: 0x%08x\n", endpost);
}

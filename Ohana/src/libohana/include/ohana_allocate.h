# ifndef OHANA_ALLOCATE
# define OHANA_ALLOCATE

# define OHANA_MEMMAGIC (uint32_t) 0xdeadbeef

typedef struct OhanaMemblock {
  uint32_t startblock;	      // endpost marker
  struct OhanaMemblock *prevBlock; // previously allocated memory
  struct OhanaMemblock *nextBlock; // next allocated memory
  size_t size;		      // size of memory
  const char *file;	      // file (re)allocated 
  const char *func;	      // func (re)allocated 
  int line;		      // line (re)allocated
  int freed;		      // memory has been freed
  int Nalloc;		      // need to put endblock on the endpost
  uint32_t endblock;	      // endpost marker
} OhanaMemblock;

typedef struct {
  int    exists; // has the memory management system been created?
  size_t Ntotal; // number of blocks currently allocated
  size_t Nbytes; // number of bytes currently allocated
  size_t Ngood;  // number of good blocks
  size_t Nbad;   // number of bad blocks
} OhanaMemstats;

OhanaMemstats ohana_memstats (int mode);

/* EAM 2012.01.18 : comparative tests on my desktop 'pikake' (Intel Core 2 Quad) show a
 * modest speed loss when using the Ohana memory managment stuff.  the tests runs a ~1.2M
 * allocations followed by the same number of frees.  with the Ohana memory code, these
 * ran at ~0.69 sec per block; without the time was ~0.56 sec per block
 */

/* default is to use basic system memory functions */

# ifdef OHANA_MEMORY

void *ohana_malloc (const char *file, int line, const char *func, size_t Nelem, size_t esize);
void *ohana_realloc (const char *file, int line, const char *func, void *in, size_t Nelem, size_t esize);
void  ohana_free (const char *file, int line, const char *func, void *in);
void  ohana_memdump_func (int mode);
int   ohana_memcheck_func (const char *myFile, int myLine, const char *myFunc, int mode);
void ohana_memcheck_block_func (const char *myFile, int myLine, const char *myFunc, void *in);
void  real_free (void *in);

void ohana_memdump_strings_file (FILE *f, int VERBOSE);
void ohana_memdump_set_maxlines (int N);
void ohana_memblock_show_stats (void *in);

# define ohana_memcheck(X) ohana_memcheck_func (__FILE__, __LINE__, __func__, X)
# define ohana_memcheck_block(X) ohana_memcheck_block_func (__FILE__, __LINE__, __func__, X)
# define ohana_memdump(X) ohana_memdump_func (X);

# define ALLOCATE(PTR,TYPE,SIZE) { \
    PTR = (TYPE *) ohana_malloc (__FILE__, __LINE__, __func__, (SIZE), sizeof(TYPE)); \
  }

# define ALLOCATE_PTR(PTR,TYPE,SIZE) \
  TYPE *PTR = (TYPE *) ohana_malloc (__FILE__, __LINE__, __func__, (SIZE), sizeof(TYPE));

# define ALLOCATE_ZERO(PTR,TYPE,SIZE) { \
    PTR = (TYPE *) ohana_malloc (__FILE__, __LINE__, __func__, (SIZE), sizeof(TYPE)); \
    memset (PTR, 0, (SIZE)*sizeof(TYPE)); \
  }

# define REALLOCATE(PTR,TYPE,SIZE) { \
    PTR = (TYPE *) ohana_realloc(__FILE__, __LINE__, __func__, PTR, (SIZE), sizeof(TYPE)); \
  }

# define CHECK_REALLOCATE(PTR,TYPE,SIZE,NCURR,DELTA) { \
  if ((NCURR) >= (SIZE)) { \
    SIZE += DELTA; \
    PTR = (TYPE *) ohana_realloc(__FILE__, __LINE__, __func__, PTR, (SIZE), sizeof(TYPE)); \
  } }

# define FREE(PTR) { if (PTR != NULL) { ohana_free (__FILE__, __LINE__, __func__, PTR); } }
# define free(PTR) { ohana_free(__FILE__, __LINE__, __func__, PTR); }

# else  /* below: not OHANA_MEMORY */

int   ohana_memcheck_noop (int mode);

# define ohana_memcheck(X) ohana_memcheck_noop (X)
# define ohana_memcheck_block(X) ohana_memcheck_noop (X)
# define ohana_memdump(X) /* NOP */
void  real_free (void *in);

# define ALLOCATE(PTR,TYPE,SIZE) {					\
  PTR = (TYPE *) malloc ((size_t)(MAX(((SIZE)*((int)sizeof(TYPE))),1))); \
  if (PTR == NULL) {							\
    fprintf(stderr,"failed malloc at %d in %s\n", __LINE__, __FILE__);	\
    exit (10); } } 	       

# define ALLOCATE_ZERO(PTR,TYPE,SIZE) {					\
  PTR = (TYPE *) malloc ((size_t)(MAX(((SIZE)*((int)sizeof(TYPE))),1))); \
  if (PTR == NULL) {							\
    fprintf(stderr,"failed malloc at %d in %s\n", __LINE__, __FILE__);	\
    exit (10); \
  } memset (PTR, 0, (SIZE)*sizeof(TYPE)); }

# define REALLOCATE(PTR,TYPE,SIZE) { 					\
  PTR = (TYPE *) realloc(PTR,(size_t)(MAX(((SIZE)*((int)sizeof(TYPE))),1))); \
  if (PTR == NULL) {							\
    fprintf(stderr,"failed realloc at %d in %s\n", __LINE__, __FILE__);	\
    exit (10); } }

# define CHECK_REALLOCATE(PTR,TYPE,SIZE,NCURR,DELTA) { 	\
  if ((NCURR) >= (SIZE)) {				\
    SIZE += DELTA;							\
    PTR = (TYPE *) realloc(PTR,(size_t)(MAX(((SIZE)*((int)sizeof(TYPE))),1))); \
    if (PTR == NULL) {							\
      fprintf(stderr,"failed realloc increment at %d in %s\n", __LINE__, __FILE__); \
      exit (10); } } }

# define FREE(PTR) { if (PTR != NULL) { free (PTR); } }
# endif /* OHANA_MEMORY */

void  ohana_memdump_file (FILE *f, int mode);

# endif /* OHANA_ALLOCATE */

/*** random C-functions ***/

void
  bcopy
  (char *b1,
   char *b2,
   int   length);

void
  bzero
  (char *b,
   int   length);

# ifndef HAS_ANSI_PROTOTYPES
int
   fprintf 
   (FILE *stream,
    char *format,...);

int
   fscanf 
   (FILE *stream,
    char *format,...);

int 
  fread
  (char *ptr,
   int size, 
   int nitems,
   FILE *stream);

/*
char *
  malloc
  (unsigned size);

char *
  realloc
  (char *ptr,
   unsigned size);
*/

int 
  fwrite
  (char *ptr,
   int size, 
   int nitems,
   FILE *stream);

int
  fseek 
  (FILE *f,
   int offset,
   int from);

int
  fclose
  (FILE *f);
# endif /* HAS_ANSI_PROTOTYPES */


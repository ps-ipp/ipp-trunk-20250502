
# ifndef ALLOCATE
# define ALLOCATE(X,T,S)  \
  X=(T *)malloc((unsigned) (MAX(((S)*sizeof(T)),1)));\
  if(X==NULL) \
    { \
      fprintf(stderr,"failed to malloc X\n");\
        exit (10);\
    } 
# define REALLOCATE(X,T,S) \
  X=(T *)realloc(X,(unsigned) (MAX(((S)*sizeof(T)),1))); \
  if(X==NULL) \
    { \
       fprintf(stderr,"failed to realloc X\n"); \
       exit (10); \
    }
# endif /* ALLOCATE */

float mrq2dinit (int *, int *, float *, float *, int, float *, int, float (funcs)(int *, int *, float *, float *)); 
float mrq2dmin (int *, int *, float *, float *, int, float *, int, float (funcs)(int *, int *, float *, float *)); 
float **mrq2dcovar (int);

# define NPARS 8
# define MIN(X,Y) ((X) < (Y) ? (X) : (Y))
# define MAX(X,Y) ((X) > (Y) ? (X) : (Y))
# define SQ(X)    (double) (((double)(X))*((double)(X)))
# define SWAP(X,Y) {double tmp=(X); (X) = (Y); (Y) = tmp;}

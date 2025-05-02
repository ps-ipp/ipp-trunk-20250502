# include <math.h>
# include <ohana.h>

typedef struct {
  double  X, Y;
  char   *line;
  int     match;
} value_type;

typedef struct {
  value_type *values;
  char       *buffer;
  int        Nvalues;
} data_type;


typedef struct {
  char *line1;
  char *line2;
  double dX, dY;
} match_type;

/******* PROTOTYPES ***********/

void               help              PROTO((void));
data_type          input             PROTO((char *, int, int, int));
match_type        *compare           PROTO((data_type, data_type, int *, double, double, double, double));
match_type        *gc_compare        PROTO((data_type, data_type, int *, double, double, double, double));
int                get_argument      PROTO((int, char **, char *));
int                remove_argument   PROTO((int, int *, char **));
void               data_sort         PROTO((data_type));
void               output            PROTO((data_type, data_type, match_type *, int, int, int, int, int));
char              *nextword          PROTO((char *));
int                parse             PROTO((double *, int, char *));
char              *nextline          PROTO((char *));
int                scan_line         PROTO((FILE *, char *));

extern double hypot PROTO((double, double));
/*  this seems to be a problem: is not included from math.h with the -ansi flag */

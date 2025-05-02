# include "external.h"
# include "kapa.h"
# include "dvomath.h"

# ifndef DISPLAY_H
# define DISPLAY_H

/*** kapa graph functions ***/
int           PlotVectorSingle      PROTO((int kapa, Vector *vec, char *mask, char *mode));
int           PlotVectorPair        PROTO((int kapa, Vector *xVec, Vector *yVec, char *mask, Graphdata *graphmode));
int           PlotVectorPairErrors  PROTO((int kapa, Vector *xVec, Vector *yVec, Vector *dyValues, Graphdata *graphmode));

int           PlotVectorTriplet     PROTO((int kapa, Vector *xVec, Vector *yVec, Vector *zValues, char *mask, Graphdata *graphmode));
// int           GetGraphData          PROTO((Graphdata *data, int *kapa, char *name));
int           GetGraph              PROTO((Graphdata *data, int *kapa, char *name));
int           SetGraph              PROTO((Graphdata *data));

/*** kapa image functions */
int           GetImageData          PROTO((KapaImageData *data, int *kapa, char *name));
int           GetImage              PROTO((KapaImageData *data, int *kapa, char *name));
int           SetImage              PROTO((KapaImageData *data));

void	      QuitKapa              PROTO((void));
void	      InitKapa		    PROTO((void));
int 	      open_kapa		    PROTO((int entry));
int 	      close_kapa	    PROTO((char *name));
int 	      AddKapaDevice	    PROTO((char *name));
int 	      DelKapaDevice	    PROTO((char *name));
int 	      FindKapaDevice	    PROTO((char *name));
char	     *GetKapaName	    PROTO((void));

/* calling program need to define a function 'get_variable' which
 * returns the name of the executable for each of KAPA and KII
 */
char         *get_variable          PROTO((char *name));

int SendLabel (char *string, int Xgraph, int mode);

int SendGraphMessage (int device, char *format, ...) OHANA_FORMAT(printf, 2, 3);
int SendGraphCommand (int device, int length, char *format, ...) OHANA_FORMAT(printf, 3, 4);
int SendGraphCommandV (int device, int length, char *format, va_list argp);

# endif

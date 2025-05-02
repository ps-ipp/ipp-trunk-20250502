# include <ohana.h>
# include <dvo.h>
# include <signal.h>

/* global variables set in parameter file */
# define DVO_MAX_PATH 1024
char         ImageCat[DVO_MAX_PATH];
char        *CATDIR;
int          HOST_ID;
char        *HOSTDIR;
char        *SINGLE_CPT;
int          VERBOSE;
int          UPDATE;
int          PARALLEL;
int          PARALLEL_MANUAL;
int          PARALLEL_SERIAL;

SkyRegion    UserPatch;

/***** prototypes ****/
int           main                              PROTO((int argc, char **argv));
void          ConfigInit                        PROTO((int *argc, char **argv));

void          usage_ckids                       PROTO((void));
void          initialize_ckids                  PROTO((int argc, char **argv));
int           args_ckids                        PROTO((int argc, char **argv));
					            
void          usage_ckids_client                PROTO((void));
void          initialize_ckids_client           PROTO((int argc, char **argv));
int           args_ckids_client                 PROTO((int argc, char **argv));

int           update_dvo_ckids                  PROTO((void));
int           update_dvo_ckids_parallel         PROTO((SkyList *sky));
					            
void          update_catalog_ckids              PROTO((Catalog *catalog, FILE *foutput));

int           Shutdown                        PROTO((char *format, ...) OHANA_FORMAT(printf, 1, 2) );
void          TrapSignal                      PROTO((int sig));
void          SetProtect                      PROTO((int mode));
int           SetSignals                      PROTO((void));


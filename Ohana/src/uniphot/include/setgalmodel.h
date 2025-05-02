# include <ohana.h>
# include <dvo.h>
# include <signal.h>

/* global variables set in parameter file */
# define DVO_MAX_PATH 1024
char         ImageCat[DVO_MAX_PATH];
char        *CATDIR;
int          HOST_ID;
char        *HOSTDIR;
int          VERBOSE;
int          UPDATE;
int          PARALLEL;
int          PARALLEL_MANUAL;
int          PARALLEL_SERIAL;
char        *UPDATE_CATFORMAT;
char        *SINGLE_CPT;

float        TEST_SCALE;
char        *GALAXY_MODEL;
SkyRegion    UserPatch;

/***** prototypes ****/
int           main                            PROTO((int argc, char **argv));

void          GetConfig                       PROTO((char *config, char *field, char *format, int N, void *ptr));
void          ConfigInit                      PROTO((int *argc, char **argv));

void          usage_setgalmodel                 PROTO((void));
void          initialize_setgalmodel            PROTO((int argc, char **argv));
int           args_setgalmodel                  PROTO((int argc, char **argv));

void          usage_setgalmodel_client          PROTO((void));
void          initialize_setgalmodel_client     PROTO((int argc, char **argv));
int           args_setgalmodel_client           PROTO((int argc, char **argv));

int           Shutdown                        PROTO((char *format, ...) OHANA_FORMAT(printf, 1, 2) );
void          TrapSignal                      PROTO((int sig));
void          SetProtect                      PROTO((int mode));
int           SetSignals                      PROTO((void));

int           update_dvo_setgalmodel            PROTO((void));
int           update_dvo_setgalmodel_parallel   PROTO((SkyTable *sky));

int           update_catalog_setgalmodel        PROTO((Catalog *catalog));

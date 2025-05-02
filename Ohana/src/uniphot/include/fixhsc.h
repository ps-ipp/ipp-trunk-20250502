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

void          initialize_fixhsc                 PROTO((int argc, char **argv));
void          initialize_fixhsc_client          PROTO((int argc, char **argv));

int           load_rules_fixhsc                 PROTO((char *filename));
int           get_rules_fixhsc                  PROTO((e_time time));

void          set_db                            PROTO((FITS_DB *in));
int           Shutdown                          PROTO((char *format, ...) OHANA_FORMAT(printf, 1, 2) );
void          TrapSignal                        PROTO((int sig));
void          SetProtect                        PROTO((int mode));
int           SetSignals                        PROTO((void));

Image        *load_images_fixhsc                PROTO((FITS_DB *db, off_t *Nimage));
void          update_images_fixhsc              PROTO((Image *image, off_t Nimage));

int           update_dvo_fixhsc                 PROTO((void));
int           update_dvo_fixhsc_parallel        PROTO((SkyTable *sky));
					        
void          update_catalog_fixhsc             PROTO((Catalog *catalog));

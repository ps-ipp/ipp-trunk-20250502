# include <ohana.h>
# include <dvo.h>
# include <signal.h>
# include "mysql.h"
# include "limits.h"

# define DVO_MAX_PATH 1024
# define MAX_BUFFER 0x080000

typedef struct {
  uint64_t objID;
  uint64_t detectID;
  uint64_t ippObjID;
  unsigned int ippDetectID;
  int imageID;
  int catID;
  double ra;
  double dec;
  float raErr;
  float decErr;
  float zpPSF;
  float zpFactorPSF;
  float zpAPER;
  float zpFactorAPER;
  float telluricExt;
  float airmass;
  float expTime;

  float Mpsf;
  float dMpsf;
  float Mkron;
  float dMkron;
  float Map;
  float dMap;

  unsigned int flags;
  unsigned int objflags;
  unsigned int filtflags;
} Detections;

/* global variables set in parameter file */
char         ImageCat[DVO_MAX_PATH];

char        *CATDIR;
int          HOST_ID;
char        *HOSTDIR;
int          VERBOSE;

int          SAVE_REMOTE;
int          TEST_MODE;

int          PARALLEL;
int          PARALLEL_MANUAL;
int          PARALLEL_SERIAL;

char        *TIME_START;
char        *TIME_END;

int          PHOTCODE_START;
int          PHOTCODE_END;

char        *SINGLE_CPT;

char        *RESULT_FILE;

char        *DATABASE_HOST;
char        *DATABASE_USER;
char        *DATABASE_PASS;
char        *DATABASE_NAME;

SkyRegion    UserPatch;

/***** prototypes ****/
int    main                          	  PROTO((int argc, char **argv));
       
void   GetConfig                     	  PROTO((char *config, char *field, char *format, int N, void *ptr));
void   ConfigInit                    	  PROTO((int *argc, char **argv));
       
void   usage_dvopsps                 	  PROTO((void));
void   initialize_dvopsps            	  PROTO((int argc, char **argv));
int    args_dvopsps                  	  PROTO((int argc, char **argv));
       
void   usage_dvopsps_client          	  PROTO((void));
void   initialize_dvopsps_client     	  PROTO((int argc, char **argv));
int    args_dvopsps_client           	  PROTO((int argc, char **argv));
       
MYSQL *mysql_dvopsps_connect              PROTO((MYSQL *mysqlBase));

int    create_manifest_mysql              PROTO((char *basename, MYSQL *mysql));
int    insert_manifest_mysql              PROTO((char *basename, MYSQL *mysql, int regionID, char *regionName));

int    insert_detections_dvopsps          PROTO((void));
int    insert_detections_dvopsps_parallel PROTO((SkyTable *sky));
int    insert_detections_dvopsps_catalog  PROTO((Catalog *catalog, MYSQL *mysql));
      
int    insert_detections_mysql_commit     PROTO((IOBuffer *buffer, MYSQL *mysql));
// int    insert_detections_mysql_value      PROTO((IOBuffer *buffer, Average *average, Measure *measure));
int    insert_detections_mysql_init       PROTO((IOBuffer *buffer));

int    insert_detections_mysql_array      PROTO((MYSQL *mysql, Detections *detections, int Ndetections));
int    insert_detections_mysql_detvalue   PROTO((IOBuffer *buffer, Detections *detection));
int    assign_detection_values            PROTO((Detections *detection, Measure *measure, Average *average, SecFilt *secfilt));

int    init_detections                    PROTO((void));
int    append_detections_dvopsps_catalog  PROTO((Catalog *catalog));
int    save_detections_dvopsps            PROTO((void));

int    insert_objects_dvopsps             PROTO((void));
int    insert_objects_dvopsps_parallel    PROTO((SkyList *sky));
int    insert_objects_dvopsps_catalog     PROTO((Catalog *catalog, char *basename, MYSQL *mysql));
      				          
int    insert_objects_mysql_create_tables PROTO((char *basename, MYSQL *mysql));
int    insert_objects_mysql_commit        PROTO((IOBuffer *ave_buffer, IOBuffer *sec_buffer, MYSQL *mysql));
int    insert_objects_mysql_value         PROTO((IOBuffer *ave_buffer, IOBuffer *sec_buffer, Average *average, SecFilt *secfilt, int Nsecfilt));
int    insert_objects_mysql_init          PROTO((IOBuffer *ave_buffer, IOBuffer *sec_buffer, char *basename));

int    insert_FWobjects_dvopsps             PROTO((void));
int    insert_FWobjects_dvopsps_parallel    PROTO((SkyList *sky));
int    insert_FWobjects_dvopsps_catalog     PROTO((Catalog *catalog, char *basename, MYSQL *mysql));
      				          
int    insert_FWobjects_mysql_create_tables PROTO((char *basename, MYSQL *mysql));
int    insert_FWobjects_mysql_init   	    PROTO((IOBuffer *ave_buffer, IOBuffer *sec_buffer, IOBuffer *cpy_buffer, char *basename));
int    insert_FWobjects_mysql_value  	    PROTO((IOBuffer *ave_buffer, IOBuffer *sec_buffer, IOBuffer *cpy_buffer, Average *average, SecFilt *secfilt, int Nsecfilt, Lensobj *lensobj, int Nlensobj));
int    insert_FWobjects_mysql_commit 	    PROTO((IOBuffer *ave_buffer, IOBuffer *sec_buffer, IOBuffer *cpy_buffer, MYSQL *mysql));

int    insert_FGshape_dvopsps               PROTO((void));
int    insert_FGshape_dvopsps_parallel      PROTO((SkyList *sky));
int    insert_FGshape_dvopsps_catalog       PROTO((Catalog *catalog, char *basename, MYSQL *mysql));
      				          
int    insert_FGshape_mysql_create_tables   PROTO((char *basename, MYSQL *mysql));
int    insert_FGshape_mysql_init   	    PROTO((IOBuffer *ave_buffer, IOBuffer *cpy_buffer, char *basename));
int    insert_FGshape_mysql_value  	    PROTO((IOBuffer *ave_buffer, IOBuffer *cpy_buffer, Average *average, GalPhot *galphot, int Ngalphot));
int    insert_FGshape_mysql_commit 	    PROTO((IOBuffer *ave_buffer, IOBuffer *cpy_buffer, MYSQL *mysql));

int    insert_skytable                    PROTO((void));

int    insert_skytable_mysql_commit       PROTO((IOBuffer *buffer, MYSQL *mysql));
int    insert_skytable_mysql_value        PROTO((IOBuffer *buffer, SkyRegion *region));
int    insert_skytable_mysql_init         PROTO((IOBuffer *buffer));

int    insert_diffobj_dvopsps             PROTO((void));
int    insert_diffobj_dvopsps_parallel    PROTO((SkyList *sky));
int    insert_diffobj_dvopsps_catalog     PROTO((Catalog *catalog, char *basename, MYSQL *mysql));

int    insert_diffobj_mysql_create_tables PROTO((char *basename, MYSQL *mysql));
int    insert_diffobj_mysql_init          PROTO((IOBuffer *ave_buffer, IOBuffer *sec_buffer, char *basename));
int    insert_diffobj_mysql_value         PROTO((IOBuffer *ave_buffer, IOBuffer *sec_buffer, Average *average, SecFilt *secfilt, int Nsecfilt));
int    insert_diffobj_mysql_commit        PROTO((IOBuffer *ave_buffer, IOBuffer *sec_buffer, MYSQL *mysql));

Detections *DetectionsLoad        	  PROTO((char *filename, int *Ndetections));
int         DetectionsSave        	  PROTO((char *filename, Detections *detections, int Ndetections));

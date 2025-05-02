# include <ohana.h>
# include <dvo.h>
# include <signal.h>

enum {
  BY_NOTHING,
  BY_REGION,
  BY_RADIUS,
  BY_CATALOG,
  BY_IMAGE,
  BY_IMMATCH,
  BY_IMLIST,
};

int       VERBOSE;
int       MODE;
SkyRegion REGION;
char     *IMAGENAME;

char OUTPUT[256];
char OUTFORMAT[256];
char GSCFILE[256];
char CATDIR[256];
char CATMODE[16];    /* raw, mef, split, mysql */
char CATFORMAT[16];  /* internal, elixir, loneos, panstarrs */
char SKY_TABLE[256];
int  SKY_DEPTH;

int MaxDensityUse;
float MaxDensityValue;
int MinMagUse;
float MinMagValue;
int  MagLimitUse;
float MagLimitValue;
PhotCode *photcode;

int  args             	     PROTO((int argc, char **argv));
int  ConfigInit       	     PROTO((int *argc, char **argv));
int  Shutdown         	     PROTO((char *format, ...)) OHANA_FORMAT(printf, 1, 2);
int  load_pt_catalog  	     PROTO((Catalog *catalog, SkyRegion *region));
int  select_by_region 	     PROTO((Catalog *output, Catalog *catalog, SkyRegion *region, int start, int end));
void set_db           	     PROTO((FITS_DB *in));
void wcatalog         	     PROTO((char *filename, Catalog *catalog));
void mkcatalog        	     PROTO((Catalog *catalog));
void init_catalog     	     PROTO((Catalog *catalog));
void TrapSignal       	     PROTO((int sig));
void SetProtect       	     PROTO((int mode));
int  SetSignals       	     PROTO((void));
int  gcatalog         	     PROTO((Catalog *catalog));
int  write_catalog           PROTO((Catalog *catalog));
int  write_getstar_PS1_DEV_0 PROTO((Catalog *catalog));
int  write_getstar_PS1_DEV_1 PROTO((Catalog *catalog));
int  write_getstar_PS1_DEV_2 PROTO((Catalog *catalog));

# include <ohana.h>
# include <dvo.h>
# include <signal.h>
# include <sys/time.h>
# include <time.h>
# include <zlib.h>

/* solaris requires both of these instead of ip.h:
   # include <sys/socket.h>
   # include <netinet/in.h>
*/

/* linux is happy with this, not solaris */
# include <netinet/ip.h>
# include <netdb.h>
# include <arpa/inet.h>
# include <glob.h>

typedef enum {
  DVOUTILS_NONE,
  DVOUTILS_UNIQ_IMAGES,
  DVOUTILS_CHECK_IMAGES,
} DVOUTILS_OP_TYPE;

typedef struct {
  int   *externID;
  short *photcode;
  int Nimages;
} ImageData;

int VERBOSE;
int DVOUTILS_OP;

char *IMAGES_LIST;
char *CATDIR;
char *EXTERN_ID_LIST;

int main (int argc, char **argv);
int dvoutils_args (int *argc, char **argv);
int dvoutils_uniq_images (char *filename);
int dvoutils_check_images (void);

ImageData *dvoutils_load_image_index (char *filename);

Image *dvoutils_load_image_table (char *filename, int *nimage);

int        SetSignals             PROTO((void));
void       SetProtect             PROTO((int mode));
void       TrapSignal             PROTO((int sig));
int        Shutdown               PROTO((char *format, ...)) OHANA_FORMAT(printf, 1, 2);


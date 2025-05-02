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

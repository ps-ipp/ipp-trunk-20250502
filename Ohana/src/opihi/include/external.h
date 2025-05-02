# include <signal.h>
# include <unistd.h>
# include <sys/uio.h>
# include <fcntl.h>
# include <sys/types.h>
# include <sys/wait.h>
# include <sys/socket.h>
# include <sys/un.h>
# include <sys/time.h>
# include <time.h>
# include <errno.h>
# include <pthread.h>

# include <netinet/ip.h>
# include <netdb.h>
# include <arpa/inet.h>

# include <ohana.h>
# include <dvo.h>

/* provide missing external defines */
# ifdef MISSING_SOCKET_INFO
#   define F_SETFL         4   
#   define O_NONBLOCK       0200000  
#   define AF_UNIX         1          
#   define SOCK_STREAM     1          
#   define ENOENT          2 
# endif

# define strlen(A) ((int)strlen(A))

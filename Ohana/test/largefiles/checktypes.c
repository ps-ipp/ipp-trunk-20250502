# include <stdio.h>
# include <fcntl.h>
# include <math.h>
# include <float.h>
# include <errno.h>
# include <time.h>
# include <stdlib.h>
# include <stdint.h>
# include <string.h>
# include <sys/socket.h>
# include <sys/time.h>
# include <sys/types.h>
# include <sys/stat.h>
# include <sys/uio.h>
# include <sys/un.h>
# include <unistd.h>
# include <stdarg.h> 

typedef off_t e_off;
typedef long long e_off_print;

main (int argc, char **argv) {

  e_off offInt;
  long long LLint;

  offInt = 1;
  LLint = 2;

  fprintf (stderr, "off: %lld, LL: %lld\n", (e_off_print) offInt, LLint);
  fprintf (stderr, "size of off: %ld, size of LL: %ld\n", sizeof(offInt), sizeof(LLint));

  exit (0);
}

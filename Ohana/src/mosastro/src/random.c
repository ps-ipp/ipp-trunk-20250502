# include <stdio.h>
# include <time.h>
# include <stdlib.h>

void init_random () {

  long A, B;

  A = time(NULL);
  for (B = 0; A == time(NULL); B++);
  srand48(B);
}

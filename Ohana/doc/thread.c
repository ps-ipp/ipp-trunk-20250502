# include <stdio.h>
# include <pthread.h>

static int loop1, loop2;

void *subloop (void *arg) {
  
  char *input;
  
  input = (char *) arg;
  fprintf (stderr, "starting thread %s\n", input);

  while (loop1) {
    fprintf (stderr, "loop2: %d\n", loop2);
    usleep (300000);
  }
  pthread_exit (0);
}

main () {

  int var;
  pthread_t thread1;
  pthread_attr_t thread_attr;

  loop1 = 1;
  loop2 = 10;
  pthread_create (&thread1, NULL, subloop, "test");

  while (fscanf (stdin, "%d", &var) != EOF) {
    loop2 = var;
    if (loop2 == 0) loop1 = 0;
  }
}

// a comment

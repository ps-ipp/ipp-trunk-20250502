# include "opihi.h"

run_tests () {

  /* test1 ();
     test1b ();
     test2 ();
     test3 (); */
  test4 (); 
  test5 ();
}

test1 () {

  int i, size;
  char *test, *line;

  gprint (GP_ERR, "starting test 1...  ");
  ALLOCATE (test, char, 256);
  sprintf (test, "100");
  for (i = 0; i < 1000000; i++) {
    line = dvomath (1, &test, &size, 0);
    if (line != NULL) free (line);
  }
  gprint (GP_ERR, "done with test 1\n");
  sleep (1);
}

test1b () {

  int i, size;
  char *test, *line;

  gprint (GP_ERR, "starting test 1b...  ");
  ALLOCATE (test, char, 256);
  sprintf (test, "dsin(45)");
  for (i = 0; i < 1000000; i++) {
    line = dvomath (1, &test, &size, 0);
    if (line != NULL) free (line);
  }
  gprint (GP_ERR, "done with test 1b\n");
  sleep (1);
}

test2 () {

  int i, size;
  char *test, *line;

  gprint (GP_ERR, "starting test 2...  ");
  ALLOCATE (test, char, 256);
  sprintf (test, "5 * 100");
  for (i = 0; i < 1000000; i++) {
    line = dvomath (1, &test, &size, 0);
    if (line != NULL) free (line);
  }
  gprint (GP_ERR, "done with test 2\n");
  sleep (1);
}

test3 () {

  int i, size;
  char *test, *line;

  gprint (GP_ERR, "starting test 3...  ");
  ALLOCATE (test, char, 256);
  sprintf (test, "5 * dsin(45)");
  for (i = 0; i < 1000000; i++) {
    line = dvomath (1, &test, &size, 0);
    if (line != NULL) free (line);
  }
  gprint (GP_ERR, "done with test 3\n");
  sleep (1);
}

test4 () {

  int i;
  char *test, *line;

  gprint (GP_ERR, "starting test 4...  ");
  for (i = 0; i < 1000000; i++) {
    ALLOCATE (test, char, 256);
    sprintf (test, "$N = 100");
    line = parse (test);
    if (line != NULL) free (line);
  }
  gprint (GP_ERR, "done with test 4\n");
  sleep (1);
}

test5 () {

  int i;
  char *test, *line;

  gprint (GP_ERR, "starting test 5...  ");
  for (i = 0; i < 1000000; i++) {
    ALLOCATE (test, char, 256);
    sprintf (test, "echo {100 * 800}");
    line = parse (test);
    if (line != NULL) free (line);
  }
  gprint (GP_ERR, "done with test 5\n");
  sleep (1);
}


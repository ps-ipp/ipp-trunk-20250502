# include "ohana.h"
# include "tap_ohana.h"

# define DO_FLOATS 1
# define DO_DOUBLES 1
# define NTESTS 300000

void test_float (float value) {
  char output[128], string[128], altout[128];

  unsigned int Noutput = sprintf_float(output, value);
  ok (Noutput == strlen(output), "right length");
  sprintf (string, "%+14.7e", value);

  // due to rounding error (where?), the last digit may differ between sprintf and sprintf_float
  if (!strcmp (output, string)) {
    pass ("right format: %s, string: %s", output, string);
    return;
  }

  strcpy (altout, output);
  int Co = output[9] - '0';
  int Ci = string[9] - '0';
  int valid = (Co - 1 == Ci); // e.g., Co = 8, Ci = 9
  valid = valid || (Co + 1 == Ci); // e.g., Co = 8, Ci = 7
  valid = valid || ((Co == 0) && Ci == 9);  // e.g., Co = 0, Ci = 9
  valid = valid || ((Ci == 0) && Co == 9);  // e.g., Co = 9, Ci = 0
  if (!valid) {
    fail ("wrong format: %s, string: %s", output, string);
    return;
  }
  altout[9] = string[9];
  if ((Co == 0) && (Ci == 9)) {
    altout[8] = string[8];
  }
  if ((Co == 9) && (Ci == 0)) {
    altout[8] = string[8];
  }
  if (!strcmp (altout, string)) {
    pass ("fixed format: %s, string: %s", output, string);
  } else {
    fail ("wrong format: %s, string: %s", output, string);
  }

  return;
}

void test_double (double value) {
  char output[128], string[128], altout[128];

  unsigned int Noutput = sprintf_double(output, value);
  ok (Noutput == strlen(output), "right length");
  sprintf (string, "%+21.14e", value);

  // due to rounding error (where?), the last digit may differ between sprintf and sprintf_float
  if (!strcmp (output, string)) {
    pass ("right format: %s, string: %s", output, string);
    return;
  }

  strcpy (altout, output);
  int Co = output[16] - '0';
  int Ci = string[16] - '0';
  int valid = (Co - 1 == Ci); // e.g., Co = 8, Ci = 9
  valid = valid || (Co + 1 == Ci); // e.g., Co = 8, Ci = 7
  valid = valid || ((Co == 0) && Ci == 9);  // e.g., Co = 0, Ci = 9
  valid = valid || ((Ci == 0) && Co == 9);  // e.g., Co = 9, Ci = 0
  if (!valid) {
    fail ("wrong format: %s, string: %s", output, string);
    return;
  }
  altout[16] = string[16];
  if ((Co == 0) && (Ci == 9)) {
    altout[15] = string[15];
    fprintf (stderr, "last two digits .. ");
  }
  if ((Co == 9) && (Ci == 0)) {
    altout[15] = string[15];
    fprintf (stderr, "last two digits .. ");
  }
  if (!strcmp (altout, string)) {
    pass ("fixed format: %s, string: %s", output, string);
  } else {
    fail ("wrong format: %s, string: %s", output, string);
  }

  return;
}

int main (void) {

  // plan_tests (2013);
  plan_tests (4026);

  diag ("libohana print_float.c tests");

  /*** sprint_float ***/
  if (DO_FLOATS) {
    unsigned int Noutput;
    char output[128];

    // do a bunch of special values:
    Noutput = sprintf_float(output, 0.0);
    ok (Noutput == strlen(output), "right length for 0.0");
    
    test_float (1.0);
    test_float (10.0);
    test_float (100.0);

    test_float (0.1);
    test_float (0.01);
    test_float (0.001);

    // init with a fixed seed:
    srand48(0);

    int i;
    float value;
    for (i = 0; i < 1000; i++) {
      value = 1e7 * (drand48() - 0.5);
      test_float (value);
    }

    INITTIME;
    for (i = 0; i < NTESTS; i++) {
      value = 1e7 * (drand48() - 0.5);
      sprintf_float (output, value);
    }
    MARKTIME("convert %d floats with sprintf_float: %f\n", NTESTS, dtime);

    gettimeofday (&startTimer, (void *) NULL);
    for (i = 0; i < NTESTS; i++) {
      value = 1e7 * (drand48() - 0.5);
      sprintf (output, "%+14.7e", value);
    }
    MARKTIME("convert %d float with sprintf: %f\n", NTESTS, dtime);
  }
  
  /*** sprint_double ***/
  if (DO_DOUBLES) {
    unsigned int Noutput;
    char output[128];

    // do a bunch of special values:
    Noutput = sprintf_double(output, 0.0);
    ok (Noutput == strlen(output), "right length for 0.0");
    
    test_double (1.0);
    test_double (10.0);
    test_double (100.0);
    test_double (0.1);
    test_double (0.01);
    test_double (0.001);

    // init with a fixed seed:
    srand48(0);
    
    int i;
    double value;
    for (i = 0; i < 1000; i++) {
      value = 1e15 * (drand48() - 0.5);
      test_double (value);
    }

    INITTIME;
    for (i = 0; i < NTESTS; i++) {
      value = 1e15 * (drand48() - 0.5);
      sprintf_double (output, value);
    }
    MARKTIME("convert %d doubles with sprintf_double: %f\n", NTESTS, dtime);

    gettimeofday (&startTimer, (void *) NULL);
    for (i = 0; i < NTESTS; i++) {
      value = 1e15 * (drand48() - 0.5);
      sprintf (output, "%+21.14e", value);
    }
    MARKTIME("convert %d doubles with sprintf: %f\n", NTESTS, dtime);
  }
  return exit_status();
}

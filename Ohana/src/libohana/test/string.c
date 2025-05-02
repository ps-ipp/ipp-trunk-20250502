# include "ohana.h"
# include "tap_ohana.h"

int main (void) {

  plan_tests (20);

  diag ("libohana string.c tests");

  /*** stripwhite ***/
  {
    int status;
    char string[128];

    status = stripwhite (NULL);
    
    ok (!status, "stripwhite status for NULL is FALSE");

    sprintf (string, "  a test line  ");
    status = stripwhite (string);
    
    ok (status, "stripwhite returns %d", status);
    ok (!strcmp (string, "a test line"), "string is stripped");

    sprintf (string, "a test line");
    status = stripwhite (string);
    
    ok (status, "stripwhite returns %d", status);
    ok (!strcmp (string, "a test line"), "string is stripped");
  }
    
  /*** strsubs ***/
  {
    char *output;

    output = strsubs (NULL, "needle", "noodle");
    ok (output == NULL, "strsubs for NULL is NULL");

    output = strsubs ("needle in haystack", NULL, "noodle");
    ok (output == NULL, "strsubs for NULL is NULL");

    output = strsubs ("needle in haystack", "needle", NULL);
    ok (output == NULL, "strsubs for NULL is NULL");

    output = strsubs ("needle in haystack", "needle", "noodle");
    ok (output != NULL, "stripwhite status is TRUE");
    skip_start (output == NULL, 1, "Skipping 1 test because strsubs returned NULL");
    ok (!strcmp (output, "noodle in haystack"), "successful sustitution at beginning: %s", output);
    skip_end();

    output = strsubs ("needle in haystack", "in", "noodle");
    ok (output != NULL, "stripwhite status is TRUE");
    skip_start (output == NULL, 1, "Skipping 1 test because strsubs returned NULL");
    ok (!strcmp (output, "needle noodle haystack"), "successful sustitution in middle: %s", output);
    skip_end();

    output = strsubs ("needle in haystack", "stack", "noodle");
    ok (output != NULL, "stripwhite status is TRUE");
    skip_start (output == NULL, 1, "Skipping 1 test because strsubs returned NULL");
    ok (!strcmp (output, "needle in haynoodle"), "successful sustitution at end: %s", output);
    skip_end();

    output = strsubs ("needle in haystack", "junk", "noodle");
    ok (output != NULL, "stripwhite status is TRUE");
    skip_start (output == NULL, 1, "Skipping 1 test because strsubs returned NULL");
    ok (!strcmp (output, "needle in haystack"), "successful sustitution without match: %s", output);
    skip_end();
  }

  /*** strextend ***/
  {
    int status;
    char *string = NULL;

    status = strextend(&string, "hello %s", "there");
    ok (status, "strextend for NULL input");
    ok (!strcmp(string, "hello there"), "strextend format");
    
    status = strextend(&string, "more words %d", 2);
    ok (status, "strextend for non-NULL input");
    ok (!strcmp(string, "hello there more words 2"), "strextend format");
  }
  return exit_status();
}

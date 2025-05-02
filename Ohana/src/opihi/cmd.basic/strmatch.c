# include "basic.h"

int strmatch (int argc, char **argv) {

  /* returns position of the first given character class */ 
  int i, N;

  char *startName = NULL;
  char *lengthName = NULL;
  if ((N = get_argument (argc, argv, "-output"))) {
    remove_argument (N, &argc, argv);
    startName = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
    lengthName = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }

  if (argc != 3) {
    gprint (GP_ERR, "USAGE: strchr (string) (class) [-output start length]\n");
    gprint (GP_ERR, "  return start and end position of given character class (alpha, upper, lower, float, int)\n");
    return (FALSE);
  }

  // 'class' may be alpha, float, int, 

  if (!strcasecmp(argv[2], "alpha")) {
    int myLength = strlen(argv[1]);
    int found = FALSE;
    int start = -1;
    int length = -1;
    for (i = 0; i < myLength; i++) {
      int isUpper = (argv[1][i] >= 'A') && (argv[1][i] <= 'Z');
      int isLower = (argv[1][i] >= 'a') && (argv[1][i] <= 'z');
      if (!found && (isLower || isUpper)) {
	start = i;
	found = TRUE;
	continue;
      }
      if (found && !(isLower || isUpper)) {
	length = i - start;
	break;
      }
    }
    if (found && (length < 0)) {
      length = myLength - start;
    }
    if (startName && lengthName) {
      set_int_variable (startName, start);
      set_int_variable (lengthName, length);
    } else {
      gprint (GP_LOG, "match: %d %d\n", start, length);
    }
    return TRUE;
  }

  if (!strcasecmp(argv[2], "upper")) {
    int myLength = strlen(argv[1]);
    int found = FALSE;
    int start = -1;
    int length = -1;
    for (i = 0; i < myLength; i++) {
      int isUpper = (argv[1][i] >= 'A') && (argv[1][i] <= 'Z');
      // int isLower = (argv[1][i] >= 'a') && (argv[1][i] <= 'z');
      if (!found && isUpper) {
	start = i;
	found = TRUE;
	continue;
      }
      if (found && !isUpper) {
	length = i - start;
	break;
      }
    }
    if (found && (length < 0)) {
      length = myLength - start;
    }
    if (startName && lengthName) {
      set_int_variable (startName, start);
      set_int_variable (lengthName, length);
    } else {
      gprint (GP_LOG, "match: %d %d\n", start, length);
    }
    return TRUE;
  }

  if (!strcasecmp(argv[2], "lower")) {
    int myLength = strlen(argv[1]);
    int found = FALSE;
    int start = -1;
    int length = -1;
    for (i = 0; i < myLength; i++) {
      // int isUpper = (argv[1][i] >= 'A') && (argv[1][i] <= 'Z');
      int isLower = (argv[1][i] >= 'a') && (argv[1][i] <= 'z');
      if (!found && isLower) {
	start = i;
	found = TRUE;
	continue;
      }
      if (found && !isLower) {
	length = i - start;
	break;
      }
    }
    if (found && (length < 0)) {
      length = myLength - start;
    }
    if (startName && lengthName) {
      set_int_variable (startName, start);
      set_int_variable (lengthName, length);
    } else {
      gprint (GP_LOG, "match: %d %d\n", start, length);
    }
    return TRUE;
  }

  if (!strcasecmp(argv[2], "float")) {
    int myLength = strlen(argv[1]);
    int found = FALSE;
    int start = -1;
    int length = -1;
    for (i = 0; i < myLength; i++) {
      int isDigit = (argv[1][i] >= '0') && (argv[1][i] <= '9');
      int isFloat = isDigit || (argv[1][i] == '-') || (argv[1][i] == '.');
      if (!found && isFloat) {
	start = i;
	found = TRUE;
	continue;
      }
      if (found && !isFloat) {
	length = i - start;
	break;
      }
    }
    if (found && (length < 0)) {
      length = myLength - start;
    }
    if (startName && lengthName) {
      set_int_variable (startName, start);
      set_int_variable (lengthName, length);
    } else {
      gprint (GP_LOG, "match: %d %d\n", start, length);
    }
    return TRUE;
  }

  if (!strcasecmp(argv[2], "int")) {
    int myLength = strlen(argv[1]);
    int found = FALSE;
    int start = -1;
    int length = -1;
    for (i = 0; i < myLength; i++) {
      int isDigit = (argv[1][i] >= '0') && (argv[1][i] <= '9');
      if (!found && isDigit) {
	start = i;
	found = TRUE;
	continue;
      }
      if (found && !isDigit) {
	length = i - start;
	break;
      }
    }
    if (found && (length < 0)) {
      length = myLength - start;
    }
    if (startName && lengthName) {
      set_int_variable (startName, start);
      set_int_variable (lengthName, length);
    } else {
      gprint (GP_LOG, "match: %d %d\n", start, length);
    }
    return TRUE;
  }

  gprint (GP_ERR, "unknown character class %s\n", argv[2]);
  return (TRUE);
}

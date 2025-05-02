# include "basic.h"
# define NCHAR 1024

// XXX this function should ALLOCATE the output buffers
int fprintf_opihi (int argc, char **argv) {

  int i;
  char line[NCHAR], tmp[NCHAR], fmt[NCHAR];
  char *p1, *p2, *q;

  if (argc < 2) {
    gprint (GP_ERR, "USAGE: fprintf format value value ...\n");
    return (FALSE);
  }

  q  = line;
  bzero (line, NCHAR);

  p1 = argv[1];
  for (i = 2; i < argc; i++) {
    bzero (tmp, NCHAR);
    bzero (fmt, NCHAR);

    /* find next format character */
    p2 = strchr (p1, '%');
    if (p2 == (char *) NULL) {
      gprint (GP_ERR, "mismatch between format and values\n");
      return (FALSE);
    }
    if (strlen(q) + p2 - p1 > NCHAR) {
      gprint (GP_ERR, "line too long");
      return (FALSE);
    }
    memcpy (q, p1, p2-p1);
    q = line + strlen(line);
    
    /* identify type (%NNNs %NNNNd %NNNNf) */
    for (p1 = p2 + 1; (*p1 == '.') || (*p1 == '-') || (*p1 == '+') || (*p1 == ' ') || isdigit(*p1); p1++);
    memcpy (fmt, p2, p1 - p2 + 1);
    switch (*p1) {
      case 'e':
      case 'g':
      case 'f':
      case 'E':
      case 'F':
      case 'G':
	sprintf (tmp, fmt, strtod (argv[i], NULL));
	break;
      case 's':
	sprintf (tmp, fmt, argv[i]);
	break;
      case 'd':
      case 'o':
      case 'i':
      case 'u':
      case 'c':
      case 'x':
      case 'X':
	sprintf (tmp, fmt, strtol(argv[i], NULL, 0));
	break;
      default:
	gprint (GP_ERR, "syntax error in format (e,f,g,E,F,G,c,s,d,i,o,x,X conversions allowed)\n");
	return (FALSE);
    }
    if (strlen(q) + strlen(tmp) > NCHAR) {
      gprint (GP_ERR, "line too long");
      return (FALSE);
    }
    memcpy (q, tmp, strlen(tmp));
    q = line + strlen(line);
    p1++;
  }
  p2 = strchr (p1, '%');
  if (p2 != (char *) NULL) {
    gprint (GP_ERR, "mismatch between format and values\n");
    return (FALSE);
  }
  
  p2 = p1 + strlen (p1);
  if (strlen(q) + p2 - p1 > NCHAR) {
    gprint (GP_ERR, "line too long");
    return (FALSE);
  }
  memcpy (q, p1, p2-p1);
  gprint (GP_LOG, "%s\n", line);

  return (TRUE);
}


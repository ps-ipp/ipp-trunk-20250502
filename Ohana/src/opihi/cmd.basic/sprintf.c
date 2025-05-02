# include "basic.h"
# define NCHAR 1024

// XXX this function uses fixed string lengths....
/* convert line, tmp, fmt to dynamic strings? */

int sprintf_opihi (int argc, char **argv) {

  int i;
  char line[NCHAR], tmp[NCHAR], fmt[NCHAR];
  char *p1, *p2, *q;

  if (argc < 3) {
    gprint (GP_ERR, "USAGE: sprintf var format value value ...\n");
    return (FALSE);
  }

  q  = line;
  bzero (line, NCHAR);

  p1 = argv[2];
  for (i = 3; i < argc; i++) {
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
      case 'f':
	sprintf (tmp, fmt, atof(argv[i]));
	break;
      case 's':
	sprintf (tmp, fmt, argv[i]);
	break;
      case 'd':
      case 'c':
      case 'x':
	sprintf (tmp, fmt, atoi(argv[i]));
	break;
      default:
	gprint (GP_ERR, "syntax error in format (only e,f,s,d,c,x allowed)\n");
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
  set_str_variable (argv[1], line);

  return (TRUE);
}


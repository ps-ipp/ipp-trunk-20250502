# include <ohana.h>

int help ();
int _hms_to_deg (double *h, double *d, char *string, char sep, int Nin);

int main (int argc, char **argv) {

  char line[1000];
  char sep;
  double ra, dec;
  int status, h, m, flag, N;
  double s, hh;

  sep = ' ';
  if ((N = get_argument (argc, argv, "-sep"))) {
    remove_argument (N, &argc, argv);
    sep = argv[N][0];
    remove_argument (N, &argc, argv);
  }

  if (get_argument (argc, argv, "-hms")) {
    while (scan_line (stdin, line) != EOF) {
      status = _hms_to_deg (&ra, &dec, line, sep, 3);
      if (status)
	fprintf (stdout, "%12.8f %12.8f\n", ra, dec);
    }
    exit (0);
  }
    
  if (get_argument (argc, argv, "-hh")) {
    while (scan_line (stdin, line) != EOF) {
      dparse (&hh, 1, line);
      hh /= 15.0;  /* convert from degrees to hours */
      flag = SIGN(hh);
      hh *= flag;
      h = hh;
      m = 60.000001*(hh - h);
      s = 3600*(hh - h - m / 60.0);
      if (flag > 0)
	fprintf (stdout, " %02d%c%02d%c%06.3f  ", h, sep, m, sep, s);
      else
	fprintf (stdout, "-%02d%c%02d%c%06.3f  ", h, sep, m, sep, s);
      dparse (&hh, 2, line);
      flag = SIGN(hh);
      hh *= flag;
      h = hh;
      m = 60.000001*(hh - h);
      s = 3600*(hh - h - m / 60.0);
      if (flag > 0)
	fprintf (stdout, " %02d%c%02d%c%06.3f\n", h, sep, m, sep, s);
      else
	fprintf (stdout, "-%02d%c%02d%c%06.3f\n", h, sep, m, sep, s);
    }
    exit (0);
  }

  fprintf (stderr, "USAGE: %s [-hh/-hms] \n", argv[0]);
  fprintf (stderr, "  -hh:  convert from decimal ra,dec to hours, min sec\n");
  fprintf (stderr, "  -hms: convert from hours, min sec to decimal ra,dec\n");
  exit (2);
}

/**********/
int _hms_to_deg (double *h, double *d, char *string, char sep, int Nin) {
  
  char *c;
  int i, flag_d, flag_h, Nfields;
  double tmp;
  
  *d = *h = 0;
  stripwhite (string);
  for (Nfields = 2, i = 0; i < 2*(Nin - 1); i++) {
    if ((c = strchr (string, sep)) != NULL) {
      Nfields ++;
      *c = ' ';
    }
  }
  if (Nfields != 2*Nin) {
    fprintf (stderr, "warning -- line with too few entries, skipping:  %d != %d\n", Nfields, Nin);
    return (FALSE);
  }

  Nfields /= 2;

  flag_h = dparse (h, 1, string);
  if (!flag_h) {
    fprintf (stderr, "warning -- invalid number / non-ascii characters in string : %s\n", string);
    return FALSE;
  }
  flag_d = dparse (d, Nfields + 1, string);
  if (!flag_d) {
    fprintf (stderr, "warning -- invalid number / non-ascii characters in string : %s\n", string);
    return FALSE;
  }
  *h *= flag_h;
  *d *= flag_d;

  if (Nfields > 1) {
    if (!dparse (&tmp, 2, string)) {
      fprintf (stderr, "warning -- invalid number / non-ascii characters in string : %s\n", string);
      return FALSE;
    }

    *h += tmp/60.0;
    if (!dparse (&tmp, Nfields + 2, string)) {
      fprintf (stderr, "warning -- invalid number / non-ascii characters in string : %s\n", string);
      return FALSE;
    }

    *d += tmp/60.0;
  }

  if (Nfields > 2) {
    if (!dparse (&tmp, 3, string)) {
      fprintf (stderr, "warning -- invalid number / non-ascii characters in string : %s\n", string);
      return FALSE;
    }

    *h += tmp/3600.0;
    if (!dparse (&tmp, Nfields + 3, string)) {
      fprintf (stderr, "warning -- invalid number / non-ascii characters in string : %s\n", string);
      return FALSE;
    }

    *d += tmp/3600.0;
  }
  
  *h *= 15*flag_h;
  *d *= flag_d;

  return (TRUE);
}

# include "astro.h"

int ctimes (int argc, char **argv) {

  int Reference, TimeFormat, N;
  double value;
  time_t time, TimeReference;
  char *date, *Variable;

  Variable = (char *) NULL;
  if ((N = get_argument (argc, argv, "-var"))) {
    remove_argument (N, &argc, argv);
    Variable = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }

  if (argc != 3) {
    gprint (GP_ERR, "USAGE: ctimes [-ref (value) / -abs (date)] [-var name]\n");
    return (FALSE);
  }

  GetTimeFormat (&TimeReference, &TimeFormat);

  Reference = FALSE;
  if (!strcmp (argv[1], "-ref")) Reference = TRUE;

  if (Reference) {

    value = atof (argv[2]);
    time = TimeRef (value, TimeReference, TimeFormat);
    date = ohana_sec_to_date (time);
    
    if (Variable != (char *) NULL) {
      set_str_variable (Variable, date);
      free (Variable);
    } else {
      gprint (GP_ERR, "time: %s\n", date);
    }

    free (date);
    return (TRUE);

  } else {

    if (strcmp (argv[1], "-abs")) {
      gprint (GP_ERR, "syntax error\n");
      return (FALSE);
    }

    if (!ohana_str_to_time (argv[2], &time)) { 
      gprint (GP_ERR, "syntax error\n");
      return (FALSE);
    }
    
    value = TimeValue (time, TimeReference, TimeFormat);
    
    if (Variable != (char *) NULL) {
      set_variable (Variable, value);
      free (Variable);
      return (TRUE);
    }
    gprint (GP_ERR, "time: %f\n", value);
    return (TRUE);
  }
}


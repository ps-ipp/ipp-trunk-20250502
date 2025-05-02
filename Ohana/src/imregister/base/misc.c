# include "imregister.h"

static double tz = 0.0;
void set_timezone (double dt) {
  tz = dt;
}

/* return values:
   0 - no trange arguments
   1 - arguments ok
   2 - arguments bad
*/
int get_trange_arguments (int *argc, char **argv, time_t **Tstart, time_t **Tstop, int *ntimes) {

  int Na, N, Ntimes;
  double trange;
  time_t tmp, *tstart, *tstop;

  /* allocate space for returned lists */
  Ntimes = 10;
  ALLOCATE (tstart, time_t, Ntimes);
  ALLOCATE (tstop,  time_t, Ntimes);

  for (N = 0; ; N++) {

    /* find next -trange arg */
    Na = get_argument (*argc, argv, "-trange");
    if (Na == 0) {
      *ntimes = N;
      *Tstart = tstart;
      *Tstop  = tstop;
      return (TRUE);
    }
    remove_argument (Na, argc, argv);
    
    /* tstart */
    if (!ohana_str_to_time (argv[Na], &tstart[N])) { 
      return (FALSE);
    }

    /* interpret second value */
    remove_argument (Na, argc, argv);
    if (ohana_str_to_dtime (argv[Na], &trange)) { 
      if (trange < 0) {
	tstop[N]  = tstart[N];
	tstart[N] = tstop[N] + trange;
      } else {
	tstop[N]  = tstart[N] + trange;
      }
      remove_argument (Na, argc, argv);
      goto goodvalue;
    }
    if (ohana_str_to_time (argv[Na], &tstop[N])) { 
      if (tstart[N] > tstop[N]) {
	tmp     = tstart[N];
	tstart[N] = tstop[N];
	tstop[N]  = tmp;
      }
      remove_argument (Na, argc, argv);
      goto goodvalue;
    }
    return (FALSE); /* syntax error in 2nd value */

  goodvalue:
    if (N == Ntimes - 1) {
      Ntimes += 10;
      REALLOCATE (tstart, time_t, Ntimes);
      REALLOCATE (tstop,  time_t, Ntimes);
    }
  }
}
  
int get_filter_arguments (int *argc, char **argv, int **Filt, int *Nfilt) {

  int i, N, Na, NF;
  int *filt;

  /* allocate space for returned lists */
  NF = 10;
  ALLOCATE (filt, int, NF);

  for (N = 0; ; N++) {

    /* find next -trange arg */
    Na = get_argument (*argc, argv, "-filter");
    if (Na == 0) {
      *Nfilt = N;
      *Filt = filt;
      return (TRUE);
    }
    if (Na > *argc - 2) return (FALSE); /* -filter F */
    remove_argument (Na, argc, argv);
    
    for (i = 0; i < strlen (argv[Na]); i++) { if (isspace (argv[Na][i])) argv[Na][i] = '.'; }
    for (i = 0; i < NFILTER; i++) {
      if (!strcasecmp (argv[Na], filtername[i])) {
	filt[N] = filternum[i];
      }
    }
    if (filt[N] == FILTER_NONE) return (FALSE);
    remove_argument (Na, argc, argv);

    if (N == NF - 1) {
      NF += 10;
      REALLOCATE (filt, int, NF);
    }
  }
}
  
   
/* replaces WHITESPACE blocks with single . */
void clean_spaces (char *line) {

  char *out, *in;

  if (line == NULL) return;

  out = in = line;
  while (*in) {
    if (OHANA_WHITESPACE(*in)) { 
      *out = '.';
      out ++;
      in ++;
      while (*in && OHANA_WHITESPACE(*in)) in++;
    } else {
      *out = *in;
      out ++;
      in ++;
    }
  }
  *out = 0;
  return;
}

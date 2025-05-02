# include "imregister.h"
# include "detrend.h"

Criteria *MosaicCriteria (Criteria base, char *filename, int *ncrit) {

  int i, Ncrit;
  char line[80];
  Header header;
  Criteria *crit;

  Ncrit = 1;
  ALLOCATE (crit, Criteria, Ncrit);

  /* load options from the image header */
  if (!gfits_read_header (filename, &header)) {
    if (output.verbose) fprintf (stderr, "ERROR: trouble reading image header\n");
    exit (1);
  }

  /* get Time from header */
  base.tstop = base.tstart = parse_time (&header);
  base.TimeSelect = TRUE;

  /* get filter from header */
  if (!gfits_scan (&header, FilterKeyword, "%s", 1, line)) {
    fprintf (stderr, "ERROR: trouble reading FILTER from header\n");
    exit (1);
  }
  for (i = 0; i < strlen (line); i++) { if (isspace (line[i])) line[i] = '.'; }
  for (i = 0; (i < NFILTER) && (base.Filter == FILTER_NONE); i++) {
    if (!strcasecmp (line, filtername[i])) {
      base.Filter = filternum[i];
    }
  }      
  if (base.Filter == FILTER_NONE) {
    fprintf (stderr, "ERROR: invalid filter %s\n", line);
    exit (1);
  }

  /* get exptime from header */
  if (!gfits_scan (&header, ExptimeKeyword, "%f", 1, &base.Exptime)) {
    fprintf (stderr, "ERROR: trouble reading EXPTIME from header\n");
    exit (1);
  }
  
  /* other settings implied by -mosaic */
  base.CCDSelect = FALSE;
  if ((base.Type != T_DARK) && (base.Type != T_BIAS) && (base.Type != T_MASK)) {
    base.FilterSelect = TRUE;
  } else {
    base.FilterSelect = FALSE;
  }      
  if (base.Type == T_DARK) {
    base.ExptimeSelect = TRUE;
  } else {
    base.ExptimeSelect = FALSE;
  }      
  output.Select = TRUE;

  *ncrit = Ncrit;
  crit[0] = base;
  return (crit);
}


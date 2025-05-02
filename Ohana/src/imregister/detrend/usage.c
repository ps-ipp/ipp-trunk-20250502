# include "imregister.h"
# include "detrend.h"

int usage () {
  
  fprintf (stderr, "detsearch : select images from the detrend database\n");
  fprintf (stderr, " USAGE: detsearch [-options...]\n");
  fprintf (stderr, " -h : show this list\n");
  fprintf (stderr, " --help : show this list\n");
  fprintf (stderr, " -image filename (ccd) (mode) : find matching detrend data for this image \n");
  fprintf (stderr, " -type (type) : limit selection by image type (bias, dark, flat, etc)\n");
  fprintf (stderr, " -time yyyy/mm/dd,HH:MM:SS : specify time for selection\n");
  fprintf (stderr, " -ccd (N) : limit selection to this ccd\n");
  fprintf (stderr, " -filter (name) : limit selection to this filter\n");
  fprintf (stderr, " -exptime (value) : limit selection to match this exposure time\n");
  fprintf (stderr, " -tstop : display end of valid time range\n");
  fprintf (stderr, " -treg  : display time of image registration\n");
  fprintf (stderr, " -ve    : Elixir verbose (SUCCESS / ERROR) \n");
  fprintf (stderr, " -quiet : Elixir quiet (no SUCCESS / ERROR) \n");
  fprintf (stderr, " -close : select detrend data which is closest in time, overlap not forced\n");
  fprintf (stderr, " -entry (value) : limit selection to match this entry\n");
  fprintf (stderr, " -match (value) : only list the Nth matched entry\n");
  fprintf (stderr, " -label (word)  : limit selection to match this label\n");
  fprintf (stderr, " -select : force selection of the 'best' match\n");
  fprintf (stderr, " -del    : delete the matched entries\n");
  fprintf (stderr, " -delete : delete the matched entries\n");
  fprintf (stderr, " -modify (entry) (value) : change entry in the selection to value\n");
  fprintf (stderr, "   possible -modify entries: label, order, tstart, tstop\n");
  exit (2);
}

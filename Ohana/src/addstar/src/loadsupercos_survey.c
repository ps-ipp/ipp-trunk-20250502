# include "addstar.h"
# include "supercos.h"

Survey *loadsupercos_survey (char *filename, int *nsurvey) {

  char line[1024];

  FILE *f = fopen (filename, "r");
  if (f == NULL) {
    fprintf (stderr, "failed to open SuperCOSMOS survey table file %s\n", filename);
    exit (1);
  }
  if (scan_line (f, line) == EOF) {
    fprintf (stderr, "error reading header line of %s\n", filename);
    exit (1);
  }

  int NSURVEY = 20;
  int Nsurvey = 0;
  Survey *survey = NULL;
  ALLOCATE (survey, Survey, NSURVEY);
  while (scan_line (f, line) != EOF) {

    iparse_csv (&survey[Nsurvey].survey_id,   1, line);
    dparse_csv (&survey[Nsurvey].longitude,  10, line);
    dparse_csv (&survey[Nsurvey].latitude,   11, line);
    dparse_csv (&survey[Nsurvey].plateScale, 13, line); // arcsec / mm

    if (Nsurvey != survey[Nsurvey].survey_id) {
      fprintf (stderr, "NEED to SORT surveys\n");
      abort();
    }

    Nsurvey ++;

    CHECK_REALLOCATE (survey, Survey, NSURVEY, Nsurvey, 10);
  }
  fclose (f);

  *nsurvey = Nsurvey;
  return survey;
}

// fields in the survey.csv file (this could be read from the header line, but it is too much work)
//   1 surveyID
//   2 surveyName
//   3 systemID
//   4 fieldOfView
//   5 decMin
//   6 decMax
//   7 numFields
//   8 telescope
//   9 telAperture
//  10 telLong
//  11 telLat
//  12 telHeight
//  13 plateScale
//  14 colour
//  15 waveMin
//  16 waveMax
//  17 waveEff
//  18 magLimit
//  19 epochMin
//  20 epochMax
//  21 epTsys
//  22 equinox
//  23 eqTsys
//  24 surveyRef


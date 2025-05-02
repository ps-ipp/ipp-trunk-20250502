# include "dvoshell.h"

/* match code to measure  */
# define TESTCODE(C,M)							\
  if (C != NULL) {							\
    switch (C[0].type) {						\
      case PHOT_DEP:							\
      case PHOT_REF:							\
	if (C[0].code != M.photcode) continue;				\
	break;								\
      case PHOT_SEC:							\
	if (C[0].code != GetPhotcodeEquivCodebyCode (M.photcode)) continue; \
	break;								\
      default:								\
	break;								\
    } }

/* exclusions based on measure.params  */
# define TESTMEASURE(M)							\
  if (ApplySelections[SelectionParam]) {				\
    if (TimeSelect && (M.t < tzero)) continue;				\
    if (TimeSelect && (M.t > tend)) continue;				\
    if (ErrSelect  && (M.dM > ErrValue)) continue;			\
    if (iMagSelect && (PhotInst (&M) < iMagMin)) continue;		\
    if (FlagSelect && (M.dbFlags != FlagValue)) continue;		\
    if (TypeSelect && (TypeValue != GetMeasureTypeCode (&M))) continue; \
  }

# define SETMAG(MOUT,MEAS,MODE)						\
  MOUT = NAN;								\
  if (MODE == MAG_INST) MOUT = PhotInst (&MEAS);			\
  if (MODE == MAG_CAT)  MOUT = PhotCat  (&MEAS);			\
  if (MODE == MAG_SYS)  MOUT = PhotSys  (&MEAS, average, secfilt);	\
  if (MODE == MAG_REL)  MOUT = PhotRel  (&MEAS, average, secfilt);	\
  if (MODE == MAG_CAL)  MOUT = PhotCal  (&MEAS, average, secfilt, measure, GetPhotcodeEquivbyCode (MEAS.photcode)); \
  if (MODE == MAG_AVE)  MOUT = PhotAve  (GetPhotcodeEquivbyCode (MEAS.photcode), average, secfilt); \
  if (MODE == MAG_REF)  MOUT = PhotRef  (GetPhotcodeEquivbyCode (MEAS.photcode), average, secfilt, measure); \
  if (ApplySelections[SelectionParam]) {				\
    if (MagSelect && (MOUT > MagMax)) continue;				\
    if (MagSelect && (MOUT < MagMin)) continue;				\
  }

# if (0)
/* selection criteria */
/* selections based on Measure quantities */
static int TimeSelect;
static time_t tzero, tend;
static int MagSelect;
static double MagMax, MagMin;
static int TypeSelect, TypeValue;
static int ErrSelect;
static double ErrValue;
static int iMagSelect;
static double iMagMin;
static int FlagSelect, FlagValue;
static int TypefracSelect, TypefracType, TypefracSign;
static double TypefracValue;

/* apply selections or not */
static int ApplySelections[4];
static int SelectionParam;

/* applied to Average quantities */
static int PhotcodeSelect;
static PhotCode *PhotcodeValue;
static int PhotcodeMode;

/* selections based on Average quantities */
static int ChiSelect;
static float ChiLimit;

/* selections based on ensemble quantities */
static int NphotSelect, NphotSign, NphotValue;
static int NcodeSelect, NcodeSign, NcodeValue;
static int FWHMSelect, FWHMsign;
static double FWHMvalue, FWHMfrac;

/* time concepts */
static time_t TimeReference;
static int TimeFormat;
# endif

# if (0)
int GetTimeSelection (time_t *tz, time_t *te) {
  *tz = tzero;
  *te = tend;
  return (TimeSelect);
}

int GetPhotcodeInfo (char *string, PhotCode **Code, int *Mode) {

  PhotCode *code;
  int mode, status;
  char *p, *tmpstring;

  /* save local copy */
  tmpstring = strcreate (string);

  /* check for code:mode in photcode name */
  mode = MAG_NONE;
  p = strchr (tmpstring, ':');
  if (p != NULL) {
    mode = GetMagMode (p + 1);
    if (mode == MAG_NONE) {
      gprint (GP_ERR, "syntax error in magnitude mode\n");
      free (tmpstring);
      return (FALSE);
    }
    *p = 0;
  }

  /* how do we handle this elsewhere? */
  if (!strcasecmp (tmpstring, "mag")) {
    /* need to validate mode */
    *Mode = mode;
    *Code = NULL;
    free (tmpstring);
    return (TRUE);
  }

  code = GetPhotcodebyName (tmpstring);
  if (code == NULL) {
    gprint (GP_ERR, "photcode not found in photcode table\n");
    free (tmpstring);
    return (FALSE);
  }

  /* test allowable cases and/or set default values */
  status = FALSE;
  if (code[0].type == PHOT_DEP) {
    if (mode == MAG_NONE) mode = MAG_REL;
    if (mode == MAG_INST) status = TRUE;
    if (mode == MAG_CAT)  status = TRUE;
    if (mode == MAG_SYS)  status = TRUE;
    if (mode == MAG_REL)  status = TRUE;
    if (mode == MAG_CAL)  status = TRUE;
  }  
  if (code[0].type == PHOT_SEC) {
    if (mode == MAG_NONE) mode  = MAG_AVE;
    if (mode == MAG_INST) status = TRUE;
    if (mode == MAG_CAT)  status = TRUE;
    if (mode == MAG_SYS)  status = TRUE;
    if (mode == MAG_REL)  status = TRUE;
    if (mode == MAG_CAL)  status = TRUE;
    if (mode == MAG_AVE)  status = TRUE;
    if (mode == MAG_REF)  status = TRUE;
  }  
  if (code[0].type == PHOT_ALT) {
    if (mode == MAG_NONE) mode  = MAG_AVE;
    if (mode == MAG_AVE)  status = TRUE;
    if (mode == MAG_REF)  status = TRUE;
  }

  if (code[0].type == PHOT_REF) {
    if (mode == MAG_NONE) mode  = MAG_CAT;
    if (mode == MAG_CAT)  status = TRUE;
  }

  if (!status) {
    gprint (GP_ERR, "mismatch in photcode and magmode\n");
    free (tmpstring);
    return (FALSE);
  }
  *Code = code;
  *Mode = mode;
  free (tmpstring);
  return (TRUE);
}
 
int SetSelectionParam (int param) {
  SelectionParam = param;
  return (TRUE);
}

int GetSelectionParam () {
  return (SelectionParam);
}

int GetMeasureParam (char *parname) {

  int param;

  param = MEAS_ZERO;
  if (!strcasecmp (parname, "ra"))   	 param = MEAS_RA;
  if (!strcasecmp (parname, "dec"))  	 param = MEAS_DEC;
  if (!strcasecmp (parname, "mag")) 	 param = MEAS_MAG;
  if (!strcasecmp (parname, "airmass"))  param = MEAS_AIRMASS;
  if (!strcasecmp (parname, "exptime"))  param = MEAS_EXPTIME;
  if (!strcasecmp (parname, "photcode")) param = MEAS_PHOTCODE;
  if (!strcasecmp (parname, "time"))     param = MEAS_TIME;
  if (!strcasecmp (parname, "dR")) 	 param = MEAS_RA_OFFSET;
  if (!strcasecmp (parname, "dD")) 	 param = MEAS_DEC_OFFSET;
  if (!strcasecmp (parname, "fwhm"))   	 param = MEAS_FWHM;
  if (!strcasecmp (parname, "FLAGS"))    param = MEAS_DB_FLAGS;
  if (!strcasecmp (parname, "XCCD"))   	 param = MEAS_XCCD;
  if (!strcasecmp (parname, "YCCD"))   	 param = MEAS_YCCD;
  if (!strcasecmp (parname, "XMOSAIC"))  param = MEAS_XMOSAIC;
  if (!strcasecmp (parname, "YMOSAIC"))  param = MEAS_YMOSAIC;
  if (!strcasecmp (parname, "help")) {
    gprint (GP_ERR, "value may be one of the following:\n");
    gprint (GP_ERR, " ra dR dec dD mag dmag Mrel Mcal photcode time fwhm xccd yccd xmosaic ymosaic flags\n");
    gprint (GP_ERR, "value may also be a valid photcode\n");
    gprint (GP_ERR, "photcodes or 'mag' may have optional magnitude type: mag,[Minst, Mcat, Msys, Mrel, Mcal]\n");
  }
  return (param);
}
  
int GetAverageParam (char *parname) {

  int param;

  param = AVE_ZERO;
  if (!strcasecmp (parname, "RA"))    param = AVE_RA;
  if (!strcasecmp (parname, "DEC"))   param = AVE_DEC;

  if (!strcasecmp (parname, "dRA"))   param = AVE_RA_ERR;
  if (!strcasecmp (parname, "dDEC"))  param = AVE_DEC_ERR;

  if (!strcasecmp (parname, "uRA"))   param = AVE_U_RA;
  if (!strcasecmp (parname, "uDEC"))  param = AVE_U_DEC;
  if (!strcasecmp (parname, "duRA"))  param = AVE_U_RA_ERR;
  if (!strcasecmp (parname, "duDEC")) param = AVE_U_DEC_ERR;

  if (!strcasecmp (parname, "par"))   param = AVE_PAR;
  if (!strcasecmp (parname, "dpar"))  param = AVE_PAR_ERR;

  if (!strcasecmp (parname, "dmag"))  param = AVE_dMAG;
  if (!strcasecmp (parname, "mag"))   param = AVE_MAG;
  if (!strcasecmp (parname, "Nmeas")) param = AVE_NMEAS;
  if (!strcasecmp (parname, "Nmiss")) param = AVE_NMISS;
  if (!strcasecmp (parname, "Xm"))    param = AVE_Xm;
  if (!strcasecmp (parname, "flag"))  param = AVE_OBJ_FLAGS;
  if (!strcasecmp (parname, "type"))  param = AVE_TYPE;
  if (!strcasecmp (parname, "typefrac")) {
    if (!TypefracType) {
      gprint (GP_ERR, "typefrac needs to specify type to use\n");
      return (param);
    }
    param = AVE_TYPEFRAC;
  }
  if (!strcasecmp (parname, "Nphot")) param = AVE_NPHOT;
  if (!strcasecmp (parname, "Ncode")) param = AVE_NCODE;
  // if (!strcasecmp (parname, "Ncrit")) param = AVE_NCRIT;
  return (param);
}

/* I've set some selections - if these require a photcode, check if I set one */
int TestPhotSelections (PhotCode **code, int *mode, int param) {

  int NeedPhotcode, Needcode;

  /* if i've supplied a photcode (code != NULL), i'm not allowed to restrict it */
  if (code[0] != NULL) {
    if (PhotcodeSelect) {
      gprint (GP_ERR, "photcode selection rules violated: cannot restrict photcode with a photcode\n");
      return (FALSE);
    } else {
      return (TRUE);
    }
  }

  /* for measure tests, supply MEAS_ZERO */

  /* if I have an average or ensemble restriction, I must have a PRI/SEC photcode */
  NeedPhotcode = FALSE;
  NeedPhotcode |= ChiSelect;
  NeedPhotcode |= NphotSelect;
  NeedPhotcode |= ErrSelect;
  NeedPhotcode |= TypeSelect;
  NeedPhotcode |= TypefracSelect;
  
  NeedPhotcode |= (param == AVE_Xm);
  NeedPhotcode |= (param == AVE_MAG);
  NeedPhotcode |= (param == AVE_dMAG);
  NeedPhotcode |= (param == AVE_TYPE);
  NeedPhotcode |= (param == AVE_NPHOT);
  Needcode = (param == AVE_NCODE);

  if (NeedPhotcode || Needcode || NcodeSelect || PhotcodeSelect) {
    if (!PhotcodeSelect) {
      gprint (GP_ERR, "photcode selection problem: value requires photcode\n");
      return (FALSE);
    }
    code[0] = PhotcodeValue;
    mode[0] = PhotcodeMode;
  }
  if (NeedPhotcode) {
    if (code[0][0].type == PHOT_SEC) return (TRUE);
    if (code[0][0].type == PHOT_REF) return (TRUE);
    gprint (GP_ERR, "photcode selection problem: average value requires average photcode\n");
    return (FALSE);
  }
  return (TRUE);
}

void GetAverageParamHelp () {
  gprint (GP_ERR, "value may be one of the following:\n");
  gprint (GP_ERR, " ra dec dmag Nmeas Nmiss Xm Nphot Ncode flag type typefrac\n\n");
  gprint (GP_ERR, "value may also be a valid photcode\n");
  gprint (GP_ERR, "photcodes or 'mag' may have optional magnitude mode: mag,[Mave, Mref]\n");
  return;
}
# endif

# if (0)
int ListPhotSelections () {

  gprint (GP_ERR, "TimeSelect: %d, %s - %s\n",      TimeSelect, ctime(&tzero), ctime(&tend));
  gprint (GP_ERR, "MagSelect: %d, %f - %f\n",       MagSelect, MagMax, MagMin);
  gprint (GP_ERR, "TypeSelect: %d, %d\n",           TypeSelect, TypeValue);
  gprint (GP_ERR, "ErrSelect: %d, %f\n",            ErrSelect, ErrValue);
  gprint (GP_ERR, "iMagSelect: %d, %f\n",           iMagSelect, iMagMin);
  gprint (GP_ERR, "FlagSelect: %d, %x\n",           FlagSelect, FlagValue);
  gprint (GP_ERR, "TypefracSelect: %d, %d %d %f\n", TypefracSelect, TypefracType, TypefracSign, TypefracValue);
  gprint (GP_ERR, "ApplySelections: %d,%d,%d,%d : %d\n", ApplySelections[0], ApplySelections[1], ApplySelections[2], ApplySelections[3], SelectionParam);
  if (PhotcodeSelect) {
    gprint (GP_ERR, "PhotcodeSelect: %d, %s\n",       PhotcodeSelect, PhotcodeValue[0].name);
  } else {
    gprint (GP_ERR, "PhotcodeSelect: %d, none\n",       PhotcodeSelect);
  }
  gprint (GP_ERR, "ChiSelect: %d, %f\n",            ChiSelect, ChiLimit);
  gprint (GP_ERR, "NphotSelect: %d, %d - %d\n",     NphotSelect, NphotSign, NphotValue);
  gprint (GP_ERR, "NcodeSelect: %d, %d - %d\n",     NcodeSelect, NcodeSign, NcodeValue);
  gprint (GP_ERR, "FWHMSelect: %d, %d %f %f\n",     FWHMSelect, FWHMsign, FWHMvalue, FWHMfrac);
  return (TRUE);
}

/* remove standard photometry filtering options, set selections */
/* not all functions respect all selections... */
int SetPhotSelections (int *argc, char **argv, int Nparams) {

  int i, N;
  double trange;

  if ((N = get_argument (*argc, argv, "-phothelp"))) {
    gprint (GP_ERR, "optional photometry selection criteria:\n");
    gprint (GP_ERR, " -magrange min max\n");
    gprint (GP_ERR, " -imaglim min\n");
    gprint (GP_ERR, " -flag value\n");
    gprint (GP_ERR, " -chisq value\n");
    gprint (GP_ERR, " -photcode code\n");
    gprint (GP_ERR, " -time start range\n");
    gprint (GP_ERR, " -errorlim value\n");
    gprint (GP_ERR, " -type type\n");
    gprint (GP_ERR, " -nmeas [+/-]N\n");
    gprint (GP_ERR, " -fwhm [+/-]fraction\n");
    return (FALSE);
  }

  /* select based on measured mag (MEASURE ONLY) */
  MagSelect = FALSE;
  if ((N = get_argument (*argc, argv, "-magrange"))) {
    MagSelect = TRUE;
    remove_argument (N, argc, argv);
    MagMin = atof (argv[N]);
    remove_argument (N, argc, argv);
    MagMax = atof (argv[N]);
    remove_argument (N, argc, argv);
  }

  /* select based on instrument mag (MEASURE ONLY) */
  iMagSelect = FALSE;
  if ((N = get_argument (*argc, argv, "-imaglim"))) {
    iMagSelect = TRUE;
    remove_argument (N, argc, argv);
    iMagMin = atof (argv[N]);
    remove_argument (N, argc, argv);
  }

  /* select on value of flag (MEASURE ONLY) */
  FlagSelect = FALSE;
  if ((N = get_argument (*argc, argv, "-flag"))) {
    FlagSelect = TRUE;
    remove_argument (N, argc, argv);
    FlagValue = atof (argv[N]);
    remove_argument (N, argc, argv);
  }

  /* select on value of Chisq (AVERAGE ONLY) */
  SelectionParam = 0;
  for (i = 0; i < 4; i++) ApplySelections[i] = TRUE;
  if ((N = get_argument (*argc, argv, "-apply"))) {
    remove_argument (N, argc, argv);
    if (strlen(argv[N]) != Nparams) {
      gprint (GP_ERR, "-apply selection must define all parameter choices\n");
      return (FALSE);
    }
    for (i = 0; i < Nparams; i++) {
      if (toupper(argv[N][i]) == 'Y') {
	ApplySelections[i] = TRUE;
      } else {
	ApplySelections[i] = FALSE;
      }
    }
    remove_argument (N, argc, argv);
  }

  /* select on value of photcode */
  PhotcodeSelect = FALSE;
  if ((N = get_argument (*argc, argv, "-photcode"))) {
    PhotcodeSelect = TRUE;
    remove_argument (N, argc, argv);
    GetPhotcodeInfo (argv[N], &PhotcodeValue, &PhotcodeMode);
    if (PhotcodeValue == NULL) {
      gprint (GP_ERR, "photcode not found in photcode table\n");
      return (FALSE);;
    }
    remove_argument (N, argc, argv);
  }

  /* selection on basis of time range (MEASURE only) */
  TimeSelect = FALSE;
  if ((N = get_argument (*argc, argv, "-time"))) {
    remove_argument (N, argc, argv);
    if (!ohana_str_to_time (argv[N], &tzero)) { 
      gprint (GP_ERR, "syntax error\n");
      return (FALSE);
    }
    remove_argument (N, argc, argv);
    if (!ohana_str_to_dtime (argv[N], &trange)) { 
      gprint (GP_ERR, "syntax error\n");
      return (FALSE);
    }
    remove_argument (N, argc, argv);
    TimeSelect = TRUE;
    if (trange < 0) {
      trange = fabs (trange);
      tend = tzero;
      tzero -= trange;
    } else {
      tend = tzero + trange;
    }
  }

  /* select by error (on measure or average) */
  ErrSelect = FALSE;
  if ((N = get_argument (*argc, argv, "-errorlim"))) {
    remove_argument (N, argc, argv);
    ErrValue = atof (argv[N]);
    remove_argument (N, argc, argv);
    ErrSelect = TRUE;
  }

  /* select on value of Chisq (AVERAGE ONLY) */
  ChiSelect = FALSE;
  if ((N = get_argument (*argc, argv, "-chisq"))) {
    ChiSelect = TRUE;
    remove_argument (N, argc, argv);
    ChiLimit = atof (argv[N]);
    remove_argument (N, argc, argv);
  }

  /* select on measurement type: 1,2,3 (AVERAGE ONLY) */
  TypeSelect = FALSE;
  if ((N = get_argument (*argc, argv, "-type"))) {
    remove_argument (N, argc, argv);
    TypeValue = atoi (argv[N]);
    remove_argument (N, argc, argv);
    TypeSelect = TRUE;
  }

  /* select on measurement type: 1,2,3 (AVERAGE ONLY) */
  TypefracType = 0;
  if ((N = get_argument (*argc, argv, "-usetype"))) {
    remove_argument (N, argc, argv);
    TypefracType = atoi (argv[N]);
    remove_argument (N, argc, argv);
  }

  /* select on measurement type: 1,2,3 (AVERAGE ONLY) */
  TypefracSelect = FALSE;
  if ((N = get_argument (*argc, argv, "-typefrac"))) {
    remove_argument (N, argc, argv);
    TypefracType  = atoi (argv[N]);
    remove_argument (N, argc, argv);
    TypefracValue = fabs (atof (argv[N]));
    TypefracSign = 0;
    if (argv[N][0] == '-') TypefracSign = -1;
    if (argv[N][0] == '+') TypefracSign = +1;
    remove_argument (N, argc, argv);
    TypefracSelect = TRUE;
  }

  /* select by number of measurements (AVERAGE ONLY) */
  NphotSelect = FALSE;
  if ((N = get_argument (*argc, argv, "-nphot"))) {
    remove_argument (N, argc, argv);
    NphotValue = abs (atoi (argv[N]));
    NphotSign = 0;
    if (argv[N][0] == '-') NphotSign = -1;
    if (argv[N][0] == '+') NphotSign = +1;
    remove_argument (N, argc, argv);
    NphotSelect = TRUE;
  }

  /* select by number of measurements (AVERAGE ONLY) */
  NcodeSelect = FALSE;
  if ((N = get_argument (*argc, argv, "-ncode"))) {
    remove_argument (N, argc, argv);
    NcodeValue = abs (atoi (argv[N]));
    NcodeSign = 0;
    if (argv[N][0] == '-') NcodeSign = -1;
    if (argv[N][0] == '+') NcodeSign = +1;
    remove_argument (N, argc, argv);
    NcodeSelect = TRUE;
  }

  /* -fwhm value frac (AVERAGE ONLY) */
  FWHMSelect = FALSE;
  if ((N = get_argument (*argc, argv, "-fwhm"))) {
    remove_argument (N, argc, argv);
    FWHMvalue = abs (atof (argv[N]));
    FWHMsign = 0;
    if (argv[N][0] == '-') FWHMsign = -1;
    if (argv[N][0] == '+') FWHMsign = +1;
    remove_argument (N, argc, argv);
    FWHMSelect = TRUE;
    remove_argument (N, argc, argv);
    FWHMfrac = atof (argv[N]);
  }

  GetTimeFormat (&TimeReference, &TimeFormat);
  return (TRUE);
}

/* extract a list of measure parameters from the specified average entry based on the pre-set selections */
double *ExtractMeasures (PhotCode *code, int mode, Average *average, SecFilt *secfilt, Measure *measure, off_t *nlist, int param) {

  off_t i, Nlist, NLIST;
  double M, *list;
  
  *nlist = 0; 
  Nlist = 0;
  NLIST = MAX (1, average[0].Nmeasure);
  ALLOCATE (list, double, NLIST);

  /* check selections based on averages & ensembles: chisq, Nphot, etc */
  if (!TestAverage (code, average, secfilt, measure)) return (list); 

  /* look for measures */
  for (i = 0; i < average[0].Nmeasure; i++) {
    TESTCODE (code, measure[i]);  /* skip measurements not matching photcode */
    TESTMEASURE (measure[i]);     /* exclusions based on measure.params  */
    SETMAG (M, measure[i], mode); /* set appropriate magnitude (also does MagSelect) */ 

    /* assign value */
    list[Nlist] = GetMeasure (param, &average[0], &measure[i], M);
    Nlist ++;
  }
  *nlist = Nlist;
  return (list);
}

/* return average.param based on the selection */
double ExtractAverages (PhotCode *code, int mode, Average *average, SecFilt *secfilt, Measure *measure, int param) {

  off_t i;
  double value;

  value = NAN;

  /* this function requires code set for certain value of param.  
     use TestPhotSelectionsAverage to validate code/param choices */

  /* filter by average quantities (eg, chisq, Nphot, etc) */
  if (!TestAverage (code, average, secfilt, measure)) return (NAN);

  /* assign vector values */
  switch (param) {
    case AVE_RA:
      value = average[0].R;
      break;
    case AVE_DEC:
      value = average[0].D;
      break;
    case AVE_RA_ERR:
      value = average[0].dR;
      break;
    case AVE_DEC_ERR:
      value = average[0].dD;
      break;

    case AVE_U_RA:
      value = average[0].uR;
      break;
    case AVE_U_DEC:
      value = average[0].uD;
      break;
    case AVE_U_RA_ERR:
      value = average[0].duR;
      break;
    case AVE_U_DEC_ERR:
      value = average[0].duD;
      break;

    case AVE_PAR:
      value = average[0].P;
      break;
    case AVE_PAR_ERR:
      value = average[0].dP;
      break;


    case AVE_NMEAS:
      value = average[0].Nmeasure;
      break;
    case AVE_NMISS:
      value = average[0].Nmissing;
      break;
    case AVE_OBJ_FLAGS:
      value = average[0].flags;
      break;
    case AVE_MAG:
      switch (mode) {
	case MAG_AVE:
	  value = PhotAve  (code, average, secfilt);
	  break;
	case MAG_REF:
	  value = PhotRef  (code, average, secfilt, measure);
	  break;
	case MAG_INST:
	case MAG_CAT:
	case MAG_SYS:
	case MAG_REL:
	case MAG_CAL:
	  value = NAN;
	  for (i = 0; i < average[0].Nmeasure; i++) {
	    if (code[0].code != measure[i].photcode) continue;
	    value = measure[i].M;
	  }
	  break;
      }
      break;
    case AVE_dMAG:
      value = PhotAveErr (code, average, secfilt);
      break;
    case AVE_Xm:
      value = PhotXm (code, average, secfilt);
      break;
    case AVE_TYPE:
      value = DetermineTypeCode (average, measure, code[0].code);
      break;
    case AVE_TYPEFRAC:
      // value = DetermineTypefrac (average, measure, code);
      value = NAN;
      break;
    case AVE_NCODE:
      value = 0;
      for (i = 0; i < average[0].Nmeasure; i++) {
	if (code[0].code != GetPhotcodeEquivCodebyCode (measure[i].photcode)) continue;
	value ++;
      }
      break;
    case AVE_NPHOT:
      value = 0;
      for (i = 0; i < average[0].Nmeasure; i++) {
	if (code[0].code != GetPhotcodeEquivCodebyCode (measure[i].photcode)) continue;
	if (measure[i].dbFlags & (ID_MEAS_POOR_PHOTOM | ID_MEAS_SKIP_PHOTOM)) continue;
	value ++;
      }
      break;
# if (0)
    case AVE_NCRIT:
      value = 0;
      for (i = 0; i < average[0].Nmeasure; i++) {
	if ((code != NULL) && (code[0].code != GetPhotcodeEquivCodebyCode (measure[i].photcode))) continue;
	if (ErrSelect && (measure[i].dM > ErrValue)) continue;
	if (FlagSelect && (measure[i].dbFlags != FlagValue)) continue;
	if (TypeSelect && (TypeValue != GetMeasureTypeCode (&measure[i]))) continue;
	if (iMagSelect && (PhotInst (&measure[i]) < iMagMin)) continue;
	value ++;
      }
      break;
# endif
  }
  return (value);
}  

/* return fraction of measures (matching code) which have requested type */
double DetermineTypefrac (Average *average, Measure *measure, PhotCode *code) {

  double frac;
  int Nc, Nt;
  off_t k;
  
  Nt = Nc = 0;
  for (k = 0; k < average[0].Nmeasure; k++) {
    if ((code != NULL) && (code[0].code != GetPhotcodeEquivCodebyCode (measure[k].photcode))) continue;
    Nc ++;
    if ((measure[k].photFlags >> 16) != TypefracType) continue;
    Nt ++;
  }
  frac = (double) Nt / (double) Nc;
  return (frac);
}

/* determine the representative dophot type for this photcode (must be PRI/SEC) */
int DetermineTypeCode (Average *average, Measure *measure, int code) {

  off_t k;
  int N, Nt[3];
  
  Nt[0] = Nt[1] = Nt[2] = 0;
  for (k = 0; k < average[0].Nmeasure; k++) {
    if (code != GetPhotcodeEquivCodebyCode (measure[k].photcode)) continue;
    N = GetMeasureTypeCode (&measure[k]);
    Nt[N] ++;
  }
  if (Nt[0]) return (0);
  if (Nt[1]) return (1);
  if (Nt[2]) return (2);
  return (3);
}

int GetMeasureTypeCode (Measure *measure) {
  switch ((measure[0].photFlags >> 16)) {
    case 0:
    case 1:
    case 2:
      return (0);
      break;
    case 3:
    case 4:
    case 5:
    case 7:
    case 9:
      return (1);
      break;
    case 10:
    default:
      return (2);
  }
  return (2);
}  

int Quality (Measure *measure, int IsDophot) {

  return (TRUE);
  
  if (IsDophot) {
    
    if ((measure[0].photFlags >> 16) == 4) return (FALSE);
    return (TRUE);
  
  } else {
    
    if (FromShortPixels(measure[0].FWx < 3.0)) return (FALSE);
    if (FromShortPixels(measure[0].FWx > 10.0)) return (FALSE);

    return (TRUE);

  }
}

/* test if this average object meets the specified selection criteria.
   for photcode-dependent quantities, only test for PRI/SEC photcodes */
int TestAverage (PhotCode *code, Average *average, SecFilt *secfilt, Measure *measure) {

  off_t i, Nm;
  int Type, Select;
  double fwhm, typefrac, dM, Xm;

  /** temporary special case for Jen Katz: exclude REF with Ncode > 1 */
  if ((code != NULL) && (code[0].type == PHOT_REF)) {
    Nm = 0;
    for (i = 0; i < average[0].Nmeasure; i++) {
      TESTCODE (code, measure[i]);
      Nm++;
    }
    if (Nm > 1) return (FALSE);
  }

  if (!ApplySelections[SelectionParam]) return (TRUE);

  /* pass objects with more than FWHMfrac points with FWHM above / below FWHMvalue */ 
  if (FWHMSelect) {
    Nm = 0;
    for (i = 0; i < average[0].Nmeasure; i++) {
      fwhm = FromShortPixels(measure[i].FWx);
      switch (FWHMsign) {
	case 0:
	  if (fwhm == FWHMvalue) break;
	  continue;
	case +1:
	  if (fwhm >= FWHMvalue) break;
	  continue;
	case -1:
	  if (fwhm <= FWHMvalue) break;
	  continue;
      }
      Nm++;
    }
    if (average[0].Nmeasure * FWHMfrac > Nm) return (FALSE);
  }

  /* all selections below require a valid photcode */
  Select = ChiSelect || ErrSelect || NcodeSelect || NphotSelect || TypeSelect || TypefracSelect;
  if (!Select) return (TRUE);

  /* must have a valid code of some kind */
  if (code == NULL) return (FALSE);

  /* for NcodeSelect, count Nmeas for appropriate photcode */
  if (NcodeSelect) {
    Nm = 0;
    for (i = 0; i < average[0].Nmeasure; i++) {
      TESTCODE (code, measure[i]);
      Nm++;
    }
    switch (NcodeSign) {
      case 0:
	if (Nm == NcodeValue) break;
	return (FALSE);
      case 1:
	if (Nm >= NcodeValue) break;
	return (FALSE);
      case -1:
	if (Nm <= NcodeValue) break;
	return (FALSE);
      default:
	return (FALSE);
    }
  }

  /* only PRI/SEC photcodes apply the filter */
  if (code[0].type == PHOT_DEP) return (TRUE);
  if (code[0].type == PHOT_REF) return (TRUE);

  /* exclusions based on average.params  */
  if (ChiSelect) {
    Xm = PhotXm (code, average, secfilt);
    if (Xm == -1) return (FALSE);
    if (Xm > ChiLimit) return (FALSE);
  }
  
  /* for ErrSelect, check average errors */
  if (ErrSelect) {
    dM = PhotAveErr (code, average, secfilt);
    if (dM > ErrValue) return (FALSE);
  }
  
  /* for NphotSelect, count Nmeas for appropriate photcode */
  if (NphotSelect) {
    Nm = 0;
    for (i = 0; i < average[0].Nmeasure; i++) {
      TESTCODE (code, measure[i]);
      if (measure[i].dbFlags && ID_MEAS_SKIP_PHOTOM) continue;
      Nm++;
    }
    switch (NphotSign) {
      case 0:
	if (Nm == NphotValue) break;
	return (FALSE);
      case 1:
	if (Nm >= NphotValue) break;
	return (FALSE);
      case -1:
	if (Nm <= NphotValue) break;
	return (FALSE);
      default:
	return (FALSE);
    }
  }

  /* for TypeSelect, check on TypeCode for this object */
  if (TypeSelect) {
    Type = DetermineTypeCode (average, measure, code[0].code);
    if (Type != TypeValue) return (FALSE);
  }

  /* for TypeSelect, check on TypeCode for this object */
  if (TypefracSelect) {
    typefrac = DetermineTypefrac (average, measure, code);
    switch (TypefracSign) {
      case 0:
	if (typefrac == TypefracValue) break;
	return (FALSE);	
      case +1:
	if (typefrac >= TypefracValue) break;
	return (FALSE);	
      case -1:
	if (typefrac <= TypefracValue) break;
	return (FALSE);	
      default:
	return (FALSE);
    }
  }

  return (TRUE);
}

/* for this function, we don't need to call PhotRel, etc, but we
   do need to multiply by 0.001:
   average[].M is stored as 1000*mag where mag is PhotAbs
   measure[].M for PHOT_REL is the same 
   XXX EAM : note that we are transitioning away from millimag internal storage 
*/ 

/* send in:
   Nphot - photcode number
   Tphot - photcode type
   Ns    - secfilt entry (-1 for PRI)
   &catalog.average[i], 
   &catalog.measure[catalog.average[i].measureOffset], 
   &catalog.secfilt[i*Nsec] 
*/


double *ExtractMagnitudes (PhotCode *code, int mode, Average *average, SecFilt *secfilt, Measure *measure, off_t *n) {
  
  double *M, mag;
  off_t N;

  if ((mode == MAG_AVE) || (mode == MAG_REF)) {
    ALLOCATE (M, double, 1);
    mag = ExtractAverages (code, mode, average, secfilt, measure, AVE_MAG);
    if (isnan(mag)) {
      N = 0;
    } else {
      N = 1;
      M[0] = mag;
    }
  } else {
    M = ExtractMeasures (code, mode, average, secfilt, measure, &N, MEAS_MAG);
  }
  
  *n = N;
  return (M);
}

/* extract delta-mag pairs applying specified selections */
double *ExtractDMag (PhotCode **code, int *mode, Average *average, SecFilt *secfilt, Measure *measure, off_t *nlist) {

  int A1, A2;
  off_t i, j, N1, N2, Np, Nlist, NLIST;
  double *M1, *M2, *list;

  /* check for special case of measure-measure - this is needed to drop self-matches */
  A1 = ((mode[0] == MAG_AVE) || (mode[0] == MAG_REF));
  A2 = ((mode[1] == MAG_AVE) || (mode[1] == MAG_REF));
  if (!A1 && !A2) {
    list = ExtractMeasuresDMag (code, mode, average, secfilt, measure, nlist);
    return (list);
  }

  *nlist = 0; 
  Nlist = 0;
  NLIST = MAX (1, average[0].Nmeasure*average[0].Nmeasure);
  ALLOCATE (list, double, NLIST);
  M1 = M2 = NULL;

  /* one of the two is an average, must do independently */
  M1 = ExtractMagnitudes (code[0], mode[0], average, secfilt, measure, &N1);
  if (N1 == 0) goto skip;
  
  Np = GetSelectionParam ();
  SetSelectionParam (Np + 1);
  M2 = ExtractMagnitudes (code[1], mode[1], average, secfilt, measure, &N2);
  if (N2 == 0) goto skip;

  /* magnitudes may be NAN : set delta to NAN */
  for (i = 0; i < N1; i++) {
    for (j = 0; j < N2; j++) {
      if (isnan(M1[i]) || isnan(M2[j])) {
	list[Nlist] = NAN;
      } else {
	list[Nlist] = M1[i] - M2[j];
      }
      Nlist ++;
    }
  }

skip: 
  if (M1 != NULL) free (M1);
  if (M2 != NULL) free (M2);
  *nlist = Nlist;
  return (list);
}
  
/* extract a list of delta-measure-mags from the specified average entry based on the 
   pre-set selections - does not return self-matched measurements */
double *ExtractMeasuresDMag (PhotCode **code, int *mode, Average *average, SecFilt *secfilt, Measure *measure, off_t *nlist) {

  int Np0, Np1;
  off_t i, j, Nlist, NLIST;
  double *list, M1, M2;
  
  *nlist = 0; 
  Nlist = 0;
  NLIST = MAX (1, average[0].Nmeasure*average[0].Nmeasure);
  ALLOCATE (list, double, NLIST);

  /* must have two code values - drop this test? this is programming case, not a user case */
  if (code == NULL) return (list);
  if (code[0] == NULL) return (list);
  if (code[1] == NULL) return (list);

  /* exclude based on average parameters for both codes  */
  if (!TestAverage (code[0], average, secfilt, measure)) return (list);
  if (!TestAverage (code[1], average, secfilt, measure)) return (list);

  Np0 = GetSelectionParam ();
  Np1 = Np0 + 1;

  /* loop twice over all measures */
  for (i = 0; i < average[0].Nmeasure; i++) {
    SetSelectionParam (Np0);
    TESTCODE (code[0], measure[i]);
    TESTMEASURE (measure[i]);
    SETMAG(M1, measure[i], mode[0]);
    for (j = 0; j < average[0].Nmeasure; j++) {
      if (i == j) continue;
      SetSelectionParam (Np1);
      TESTCODE (code[1], measure[j]);
      TESTMEASURE (measure[j]);
      SETMAG(M2, measure[j], mode[1]);
      if (isnan(M1) || isnan(M2)) {
	list[Nlist] = NAN;
      } else {
	list[Nlist] = M1 - M2;
      }
      Nlist ++;
    }
  }
  *nlist = Nlist;
  return (list);
}

/* extract a measurement list matching the number of dmag entries */
double *ExtractByDMag (PhotCode **code, int *mode, Average *average, SecFilt *secfilt, Measure *measure, off_t *nlist, int param) {

  off_t N1;
  int A1, A2;
  double *list;

  /* check for special case of measure-measure */
  A1 = ((mode[0] == MAG_AVE) || (mode[0] == MAG_REF));
  A2 = ((mode[1] == MAG_AVE) || (mode[1] == MAG_REF));
  if (!A1 && !A2) {
    list = ExtractMeasuresByDMag (code, mode, 1, average, secfilt, measure, nlist, param);
    return (list);
  }

  /* one of the two entries results in a single element. extract the other */
  if (A1) {
    list = ExtractMeasures (code[1], mode[1], average, secfilt, measure, &N1, param);
  } else {
    list = ExtractMeasures (code[0], mode[0], average, secfilt, measure, &N1, param);
  }
  *nlist = N1;
  return (list);
}
  
/* extract a list of delta-measure-mags from the specified average entry based on the pre-set selections */
double *ExtractMeasuresByDMag (PhotCode **code, int *mode, int use_first, Average *average, SecFilt *secfilt, Measure *measure, off_t *nlist, int param) {

  off_t i, j, n, Nlist, NLIST;
  double *list, M1, M2;
  
  *nlist = 0; 
  Nlist = 0;
  NLIST = MAX (1, average[0].Nmeasure*average[0].Nmeasure);
  ALLOCATE (list, double, NLIST);

  /* must have two code values */
  if (code == NULL) return (list);
  if (code[0] == NULL) return (list);
  if (code[1] == NULL) return (list);

  /* chisq, fwhm, Nphot, Ncode */
  if (!TestAverage (code[0], average, secfilt, measure)) return (list);
  if (!TestAverage (code[1], average, secfilt, measure)) return (list);

  /* loop twice over all measures */
  for (i = 0; i < average[0].Nmeasure; i++) {
    TESTCODE (code[0], measure[i]);
    TESTMEASURE (measure[i]);
    SETMAG(M1, measure[i], mode[0]);
    for (j = 0; j < average[0].Nmeasure; j++) {
      if (i == j) continue;
      TESTCODE (code[1], measure[j]);
      TESTMEASURE (measure[j]);
      SETMAG(M2, measure[j], mode[1]);
      n = (use_first) ? i : j;

      /* assign value */
      list[Nlist] = GetMeasure (param, &average[0], &measure[n], (use_first ? M1 : M2));
      Nlist ++;
    }
  }
  *nlist = Nlist;
  return (list);
}
# endif

# if (0)
double GetMeasure (int param, Average *average, Measure *measure, double mag) {

  double ra, dec, x, y;
  double value;
  Image *image;
  // Coords *mosaic;

  value = 0;
  switch (param) {
    case MEAS_MAG: /* magnitudes are already determined above */
      value = mag;
      break;
    case MEAS_RA: /* OK */
      value = measure[0].R;
      break;
    case MEAS_DEC: /* OK */
      value = measure[0].D;
      break;
    case MEAS_DOPHOT: /* OK */
      value = (measure[0].photFlags >> 16);
      break;
    case MEAS_AIRMASS: /* OK */
      value = measure[0].airmass;
      break;
    case MEAS_EXPTIME: /* OK */
      value = pow (10.0, measure[0].dt * 0.4);
      break;
    case MEAS_PHOTCODE: /* OK */
      value = measure[0].photcode;
      break;
    case MEAS_TIME: /* OK */
      value = TimeValue (measure[0].t, TimeReference, TimeFormat);
      break;
    case MEAS_RA_OFFSET: /* OK */
      value = dvoOffsetR(measure, average);
      break;
    case MEAS_DEC_OFFSET: /* OK */
      value = dvoOffsetD(measure, average);
      break;
    case MEAS_FWHM: /* OK */
      value = FromShortPixels(measure[0].FWx);
      break;
    case MEAS_DB_FLAGS: /* ? */
      value = measure[0].dbFlags;
      break;
    case MEAS_XCCD: /* OK */
/* I need to perform this conversion for ELIXIR and LONEOS formats on load */      
# if 0
      value = measure[0].Xccd;
# else
      ra  = measure[0].R;
      dec = measure[0].D;
      image = MatchImageDVO (measure[0].t, measure[0].photcode, measure[0].imageID);
      if (image == NULL) break;
      RD_to_XY (&x, &y, ra, dec, &image[0].coords);
      value = x;
# endif
      break;
    case MEAS_YCCD: /* OK */
/* I need to perform this conversion for ELIXIR and LONEOS formats on load */      
# if 0
      value = measure[0].Yccd;
# else
      ra  = measure[0].R;
      dec = measure[0].D;
      image = MatchImageDVO (measure[0].t, measure[0].photcode, measure[0].imageID);
      if (image == NULL) break;
      RD_to_XY (&x, &y, ra, dec, &image[0].coords);
      value = y;
# endif
      break;
# if 0
    case MEAS_XMOSAIC: /* OK */
      ra  = measure[0].R;
      dec = measure[0].D;
      mosaic = MatchMosaic (measure[0].t, measure[0].photcode); // XXX not used anymore
      if (mosaic == NULL) break;
      RD_to_XY (&x, &y, ra, dec, mosaic);
      value = x;
      break;
    case MEAS_YMOSAIC: /* OK */
      ra  = measure[0].R;
      dec = measure[0].D;
      mosaic = MatchMosaic (measure[0].t, measure[0].photcode); // XXX not used anymore
      if (mosaic == NULL) break;
      RD_to_XY (&x, &y, ra, dec, mosaic);
      value = y;
      break;
# endif
  }
  return (value);
}
# endif

/** the mosaic entries do not use the registered mosaic found 
    by MatchImage (via FindMosaicForImage).  Rather, they use
    a coordinate frame saved by SetImageSelection 
**/

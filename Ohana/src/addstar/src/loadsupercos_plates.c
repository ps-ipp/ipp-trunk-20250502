# include "addstar.h"
# include "supercos.h"

Image *loadsupercos_plates (Survey *survey, int Nsurvey, char *filename, int *nimage) {

  // these are fields read directly from the file.  some are supplied unchanged to images,
  // some require adjustments.

  double mjd, exptime, sidtime, RAo, DECo, aXmin, aXmax, aYmin, aYmax, alt, az, stepsize;
  int nstar, survey_id, plate_id; 
  char line[3000], emulsion[128], filterID[128];

  FILE *f = fopen (filename, "r");
  if (f == NULL) {
    fprintf (stderr, "failed to open SuperCOSMOS plate table file %s\n", filename);
    exit (1);
  }
  if (scan_line (f, line) == EOF) {
    fprintf (stderr, "error reading header line of %s\n", filename);
    exit (1);
  }
  if (scan_line (f, line) == EOF) {
    fprintf (stderr, "error reading empty plate line of %s\n", filename);
    exit (1);
  }

  int NIMAGE = 1000;
  int Nimage = 0;
  Image *image = NULL;

  ALLOCATE (image, Image, NIMAGE);

  int status = TRUE;
  while (scan_line (f, line) != EOF) {
  
    iparse_csv (&plate_id, 1, line);
    iparse_csv (&survey_id, 2, line);
    if (survey_id >= Nsurvey) {
      fprintf (stderr, "unknown survey %d\n", survey_id);
      abort();
    }

    dparse_csv (&mjd, 28, line); // NOTE dparse is 1 counting (but 0 == 1?)
    iparse_csv (&nstar, 106, line);
    dparse_csv (&aXmin, 39, line);
    dparse_csv (&aXmax, 40, line);
    dparse_csv (&aYmin, 41, line);
    dparse_csv (&aYmax, 42, line);

    dparse_csv (&stepsize, 37, line); // microns / pixel
    dparse_csv (&exptime, 21, line);
    dparse_csv (&sidtime, 20, line);
    loadsupercos_getST (line, &sidtime);

    dparse_csv (&RAo, 14, line);
    dparse_csv (&DECo, 15, line);
      
    loadsupercos_getFilterInfo (line, emulsion, filterID);

    // need to pass sidereal time in degrees
    altaz (&alt, &az, 15.0*sidtime - RAo, DECo, survey[survey_id].latitude);

    float scale = stepsize * survey[survey_id].plateScale / 1000.0 / 3600.0; // scale in degrees / pixel

    image[Nimage].tzero = ohana_mjd_to_sec (mjd);
    image[Nimage].nstar = nstar;
    image[Nimage].secz = 1.0 / cos(RAD_DEG*alt);

    image[Nimage].NX = 5.00 / scale;
    image[Nimage].NY = 4.75 / scale;

    image[Nimage].apmifit = 0.0;
    image[Nimage].dapmifit = 0.0;
    image[Nimage].McalPSF   = 0.0;
    image[Nimage].McalAPER  = 0.0;
    image[Nimage].McalChiSq = 0.0;
    image[Nimage].dMcal     = 0.0;

    char photname[300];
    snprintf (photname, 300, "SCOS.%s.%s", emulsion, filterID);
    image[Nimage].photcode = GetPhotcodeCodebyName (photname);
    if (!image[Nimage].photcode) {
      fprintf (stderr, "unknown photcode %s\n", photname);
      fprintf (stderr, "line %d: %s\n", Nimage, line);
      abort();
    }
    image[Nimage].exptime = exptime * 60.0;
    image[Nimage].sidtime = sidtime; // sidereal time in hours
    image[Nimage].latitude = survey[survey_id].latitude;

    image[Nimage].RAo = RAo;
    image[Nimage].DECo = DECo;
    image[Nimage].Radius = NAN;
    
    snprintf (image[Nimage].name, DVO_IMAGE_NAME_LEN, "SCOS.%02d.%06d", survey_id, plate_id);
    image[Nimage].imageID = Nimage + 1;
    image[Nimage].externID = plate_id;

    image[Nimage].detection_limit = 0;
    image[Nimage].saturation_limit = 0;
    image[Nimage].cerror = 0;
    image[Nimage].fwhm_x = 0;
    image[Nimage].fwhm_y = 0;
    image[Nimage].trate = 0.0;
    image[Nimage].ccdnum = 0;
    image[Nimage].flags = 0;
    image[Nimage].parentID = 0;
    image[Nimage].sourceID = 0;
    image[Nimage].nLinkAstrom = 0;
    image[Nimage].nLinkPhotom = 0;
    image[Nimage].dXpixSys = NAN;
    image[Nimage].dYpixSys = NAN;
    image[Nimage].dMagSys = NAN;
    image[Nimage].nFitAstrom = 0;
    image[Nimage].nFitPhotom = 0;
    image[Nimage].photom_map_id = 0;
    image[Nimage].astrom_map_id = 0;


    // for now, we define a fake coordinate system based on the boresite center
    InitCoords (&image[Nimage].coords, "DEC--TAN");
    
    image[Nimage].coords.crval1 = RAo;
    image[Nimage].coords.crval2 = DECo;
    
    image[Nimage].coords.crpix1 = 0.5*image[Nimage].NX;
    image[Nimage].coords.crpix2 = 0.5*image[Nimage].NY;
    image[Nimage].coords.cdelt1 = image[Nimage].coords.cdelt2 = scale;

    Nimage ++;
    CHECK_REALLOCATE (image, Image, NIMAGE, Nimage, 1000);
  }
  fclose (f);

  if (!status) abort();

  *nimage = Nimage;
  return image;
}

/*** Image fields and mappings from supercosmos ***/

// tzero : mjd [27]
// nstar : objNum [105]
// secz : need to get telescope lattitude & longitude numbers
// NX : from aXmin, aXmax? [38,39]
// NY : from aXmin, aXmax? [40,41]
// apmifit : 0.0
// dapmifit : 0.0
// Mcal : 0.0
// dMcal : 0.0
// Xm : NAN
// photcode : from filterID [11]
// exptime : expLength (min) [20]
// sidtime : lstObs [19]
// latitude : (from survey_id) [01]
// RAo : raPnt [13]
// DECo : decPnt [14]
// Radius : (calculate) 
// DUMMY : 
// name : generate from plateID? [00]
// detection_limit : NAN
// saturation_limit : NAN
// cerror : NAN
// fwhm_x : NAN
// fwhm_y : 
// trate : 
// ccdnum : 
// flags : 
// imageID : 
// parentID : 
// externID : 
// sourceID : 
// nLinkAstrom : 
// nLinkPhotom : 
// dummy3 : 
// dXpixSys : 
// dYpixSys : 
// dMagSys : 
// nFitAstrom : 
// nFitPhotom : 
// photom_map_id : 
// astrom_map_id : 

/*** SuperCOSMOS plate table (downloaded from the SSA).  CSV file contains the following
 * fields:
 ***/

//  1 plateID
//  2 surveyID
//  3 fieldID
//  4 plateNum
//  5 dateMeas
//  6 timeMeas
//  7 instrument
//  8 softVersion
//  9 operator
// 10 scanMaterial
// 11 emulsion
// 12 filterID
// 13 filterComm
// 14 raPnt
// 15 decPnt
// 16 radecSys
// 17 equinox
// 18 eqTsys
// 19 utDateObs
// 20 lstObs
// 21 expLength
// 22 epoch
// 23 domeTemp
// 24 domePressure
// 25 domeHumid
// 26 waveEffect
// 27 tropl
// 28 mjd
// 29 focusNX
// 30 focusNY
// 31 calType
// 32 stepWdg
// 33 nSteps
// 34 orientation
// 35 emulpos
// 36 sosp
// 37 stepsize
// 38 scanlen
// 39 aXmin
// 40 aXmax
// 41 aYmin
// 42 aYmax
// 43 xPnt
// 44 yPnt
// 45 areaCut
// 46 apParam
// 47 dbParam
// 48 dbAmin
// 49 dbAmax
// 50 dbAcut
// 51 dbLevel
// 52 skySquare
// 53 skyDefn
// 54 skyFilter
// 55 fThresh
// 56 fSclen
// 57 pCut
// 58 starCat
// 59 brightLim
// 60 faintLim
// 61 equinRef
// 62 tSysRef
// 63 maxIter
// 64 rCritIni
// 65 rCritAbs
// 66 rCritRel
// 67 rCritFin
// 68 distType
// 69 scdGrid
// 70 raCol
// 71 decCol
// 72 raPMCol
// 73 decPMCol
// 74 plxCol
// 75 rvCol
// 76 magCol
// 77 cEquinox
// 78 cEqTSys
// 79 cEpoch
// 80 cEpTSys
// 81 starsInit
// 82 starsSelec
// 83 starsUsed
// 84 coeffs1
// 85 coeffs2
// 86 coeffs3
// 87 coeffs4
// 88 coeffs5
// 89 coeffs6
// 90 cubicDist
// 91 obsRaPnt
// 92 obsDecPnt
// 93 shiftPnt
// 94 areaMin
// 95 areaMax
// 96 magMin
// 97 magMax
// 98 ellipMin
// 99 ellipMax
//100 ellipMode
//101 orientMode
//102 ellipMed
//103 orientMed
//104 ellipMean
//105 orientMean
//106 objNum
//107 nParents
//108 lane1Count
//109 lane2Count
//110 lane3Count
//111 lane4Count
//112 lane5Count
//113 lane6Count
//114 lane7Count
//115 lane8Count
//116 lane9Count
//117 lane10Count
//118 lane11Count
//119 lane12Count
//120 lane13Count
//121 lane14Count
//122 lane15Count
//123 lane16Count
//124 lane17Count
//125 lane18Count
//126 lane19Count
//127 lane20Count
//128 lane21Count
//129 lane22Count
//130 lane23Count
//131 lane24Count
//132 lane25Count
//133 lane26Count
//134 lane27Count
//135 lane28Count
//136 lane29Count
//137 lane30Count
//138 astResidX
//139 astResidY
//140 nCalCoeffs
//141 CalXK
//142 gradient
//143 getCal1
//144 getCal2
//145 getCal3
//146 getCal4
//147 getCal5
//148 getCal6
//149 ffZp
//150 galZp
//151 galGrad
//152 directory

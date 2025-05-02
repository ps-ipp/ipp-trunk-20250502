# include <dvo.h>

/* convert panstarrs-format measures to internal measures */
Measure *Measure_Panstarrs_DEV_1_ToInternal (Average *ave, Measure_Panstarrs_DEV_1 *in, off_t Nvalues) {

  off_t i;
  Measure *out;

  ALLOCATE_ZERO (out, Measure, Nvalues);

  for (i = 0; i < Nvalues; i++) {
    dvo_measure_init (&out[i]);

    int averef = in[i].averef;
    out[i].R          = ave[averef].R - in[i].dR / 3600.0;
    out[i].D          = ave[averef].D - in[i].dD / 3600.0;
    out[i].M          = in[i].M;
    out[i].dM         = in[i].dM;
    out[i].McalPSF    = in[i].Mcal;
    out[i].McalAPER   = in[i].Mcal;

    out[i].airmass    = in[i].airmass;
    out[i].az         = in[i].az;
    out[i].dt         = in[i].dt;
    out[i].FWx 	      = in[i].FWx;
    out[i].FWy 	      = in[i].FWy;
    out[i].theta      = in[i].theta;
    out[i].photcode   = in[i].photcode;
    out[i].t          = in[i].t;
    out[i].averef     = in[i].averef;
    out[i].Xccd       = in[i].Xccd;
    out[i].Yccd       = in[i].Yccd;
    out[i].dXccd      = in[i].dXccd;
    out[i].dYccd      = in[i].dYccd;
    out[i].Sky        = in[i].Sky;
    out[i].dSky       = in[i].dSky;

    // 2008.02.26 : I've renamed Mgal to Map, and intend it to be used per aperture
    // magnitudes.  Most uses of Mgal in the past were actually aperture (isophotal) mags

    // changed or added for PS1_DEV_1 (2008.02.26)
    out[i].psfQF      = (in[i].psfQF == NAN_S_SHORT) ? NAN : in[i].psfQF;
    out[i].dbFlags    = in[i].flags;
    out[i].detID      = in[i].detID_lo;
    out[i].imageID    = in[i].imageID_lo;

    // changed for PS1_DEV_2
    out[i].Map        = in[i].Mgal;

    // changed for PS1_V1
    out[i].photFlags  = in[i].dophot << 16;
  }
  return (out);
}

/* convert internal measures to panstarrs-format measures */
Measure_Panstarrs_DEV_1 *MeasureInternalTo_Panstarrs_DEV_1 (Average *ave, Measure *in, off_t Nvalues) {

  off_t i;
  Measure_Panstarrs_DEV_1 *out;

  ALLOCATE_ZERO (out, Measure_Panstarrs_DEV_1, Nvalues);

  for (i = 0; i < Nvalues; i++) {
    int averef = in[i].averef;

    out[i].dR         = 3600.0*(ave[averef].R - in[i].R);
    out[i].dD         = 3600.0*(ave[averef].D - in[i].D);
    out[i].M          = in[i].M;
    out[i].dM         = in[i].dM;
    out[i].Mcal       = in[i].McalPSF;
    out[i].airmass    = in[i].airmass;
    out[i].az         = in[i].az;
    out[i].dt         = in[i].dt;
    out[i].FWx 	      = in[i].FWx;
    out[i].FWy 	      = in[i].FWy;
    out[i].theta      = in[i].theta;
    out[i].photcode   = in[i].photcode;
    out[i].t          = in[i].t;
    out[i].averef     = in[i].averef;
    out[i].Xccd       = in[i].Xccd;
    out[i].Yccd       = in[i].Yccd;
    out[i].dXccd      = in[i].dXccd;
    out[i].dYccd      = in[i].dYccd;
    out[i].Sky        = in[i].Sky;
    out[i].dSky       = in[i].dSky;

    // changed or added for PS1_DEV_1 (2008.02.26)
    out[i].flags      = in[i].dbFlags;
    out[i].psfQF      = isnan(in[i].psfQF) ? NAN_S_SHORT : in[i].psfQF;
    out[i].detID_hi   = 0;
    out[i].detID_lo   = in[i].detID;
    out[i].imageID_hi = 0;
    out[i].imageID_lo = in[i].imageID;

    // changed or added for PS1_DEV_2
    out[i].Mgal       = in[i].Map;

    // changed or added for PS1_V1
    // out[i].photFlags  = in[i].photFlags & 0x0000ffff; (only dophot pre PS1_DEV_1)
    out[i].dophot     = in[i].photFlags >> 16;
  }
  return (out);
}

/* convert panstarrs-format averages to internal averages */
// 'primary is needed to conform with the API for Loneos and Elixir, but is not used
Average *Average_Panstarrs_DEV_1_ToInternal (Average_Panstarrs_DEV_1 *in, off_t Nvalues, SecFilt **primary) {
  OHANA_UNUSED_PARAM(primary);

  off_t i;
  Average *out;

  ALLOCATE_ZERO (out, Average, Nvalues);

  for (i = 0; i < Nvalues; i++) {
    dvo_average_init (&out[i]);

    out[i].R        	 = in[i].R;      
    out[i].D        	 = in[i].D;      
    out[i].dR       	 = in[i].dR;
    out[i].dD       	 = in[i].dD;
    out[i].uR       	 = in[i].uR;
    out[i].uD       	 = in[i].uD;
    out[i].duR      	 = in[i].duR;
    out[i].duD      	 = in[i].duD;
    out[i].P        	 = in[i].P;
    out[i].dP       	 = in[i].dP;
    out[i].objID 	 = in[i].objID;
    out[i].catID 	 = in[i].catID;

    // changed for PS1_DEV_2
    out[i].Nmeasure      = in[i].Nm;     
    out[i].Nmissing      = in[i].Nn;     
    out[i].measureOffset = in[i].offset; 
    out[i].missingOffset = in[i].missing;

    // changed for PS1_V1
    out[i].flags    	 = in[i].code;   
  }
  return (out);
}

/* convert internal averages to panstarrs-format averages */
// 'primary is needed to conform with the API for Loneos and Elixir, but is not used
Average_Panstarrs_DEV_1 *AverageInternalTo_Panstarrs_DEV_1 (Average *in, off_t Nvalues, SecFilt *primary) {
  OHANA_UNUSED_PARAM(primary);

  off_t i;
  Average_Panstarrs_DEV_1 *out;

  ALLOCATE_ZERO (out, Average_Panstarrs_DEV_1, Nvalues);

  for (i = 0; i < Nvalues; i++) {
    out[i].R        = in[i].R;      
    out[i].D        = in[i].D;      
    out[i].dR       = in[i].dR;
    out[i].dD       = in[i].dD;
    out[i].uR       = in[i].uR;
    out[i].uD       = in[i].uD;
    out[i].duR      = in[i].duR;
    out[i].duD      = in[i].duD;
    out[i].P        = in[i].P;
    out[i].dP       = in[i].dP;
    out[i].objID    = in[i].objID;
    out[i].catID    = in[i].catID;

    // changed or added for PS1_DEV_2
    out[i].Nm       = in[i].Nmeasure;     
    out[i].Nn       = in[i].Nmissing;     
    out[i].offset   = in[i].measureOffset; 
    out[i].missing  = in[i].missingOffset;

    // changed or added for PS1_V1
    out[i].code          = in[i].flags;   
  }
  return (out);
}

/* convert panstarrs-format secfilts to internal secfilts */
SecFilt *SecFilt_Panstarrs_DEV_1_ToInternal (SecFilt_Panstarrs_DEV_1 *in, off_t Nvalues) {

  off_t i;
  SecFilt *out;

  ALLOCATE_ZERO (out, SecFilt, Nvalues);

  for (i = 0; i < Nvalues; i++) {
    dvo_secfilt_init (&out[i], SECFILT_RESET_ALL);

    out[i].MpsfChp     = in[i].M;      
    out[i].dMpsfChp    = in[i].dM;      

    out[i].Mchisq= pow (10.0, 0.01*in[i].Xm);     
    out[i].Ncode = in[i].Ncode;
    out[i].Nused = in[i].Nused;
 }
  return (out);
}

/* convert internal secfilts to panstarrs-format secfilts */
SecFilt_Panstarrs_DEV_1 *SecFiltInternalTo_Panstarrs_DEV_1 (SecFilt *in, off_t Nvalues) {

  off_t i;
  SecFilt_Panstarrs_DEV_1 *out;

  ALLOCATE_ZERO (out, SecFilt_Panstarrs_DEV_1, Nvalues);

  for (i = 0; i < Nvalues; i++) {
    out[i].M             = in[i].MpsfChp;      
    out[i].dM            = in[i].dMpsfChp;      

    out[i].Xm    = 100.0*log10(in[i].Mchisq);     
    out[i].Ncode = in[i].Ncode;
    out[i].Nused = in[i].Nused;
  }
  return (out);
}

# define RAW_IMAGE_NAME_LEN 64

/* convert panstarrs-format images to internal images */
Image *Image_Panstarrs_DEV_1_ToInternal (Image_Panstarrs_DEV_1 *in, off_t Nvalues, off_t Nalloc) {

  off_t i;
  Image *out;

  char *buffer;
  ALLOCATE_ZERO (buffer, char, Nalloc);
  out = (Image *) buffer;
  // ALLOCATE_ZERO (out, Image, Nvalues);

  for (i = 0; i < Nvalues; i++) {
    // this is only safe because the initial 120 bytes in Coords match CoordsDisk
    memcpy (&out[i].coords, &in[i].coords, sizeof(CoordsDisk));
    out[i].coords.mosaic   = NULL;
    out[i].coords.offsetMap = NULL;

    // RAW_IMAGE_NAME_LEN < DVO_IMAGE_NAME_LEN
    strncpy_nowarn (out[i].name, in[i].name, RAW_IMAGE_NAME_LEN - 1);

    out[i].tzero    	    = in[i].tzero;
    out[i].nstar    	    = in[i].nstar;
    out[i].secz	    	    = in[i].secz;
    out[i].NX	    	    = in[i].NX;
    out[i].NY	    	    = in[i].NY;
    out[i].apmifit  	    = in[i].apmifit;
    out[i].dapmifit 	    = in[i].dapmifit;

    out[i].McalPSF    	    = in[i].Mcal;
    out[i].McalAPER    	    = in[i].Mcal;
    out[i].dMcal    	    = in[i].dMcal;
    out[i].McalChiSq  	    = pow(10.0, 0.01*in[i].Xm);

    out[i].photcode   	    = in[i].photcode;
    out[i].exptime  	    = in[i].exptime;
    out[i].sidtime  	    = in[i].sidtime;
    out[i].latitude  	    = in[i].latitude;
    out[i].detection_limit  = in[i].detection_limit;
    out[i].saturation_limit = in[i].saturation_limit;
    out[i].cerror	    = in[i].cerror;
    out[i].fwhm_x	    = in[i].fwhm_x;
    out[i].fwhm_y	    = in[i].fwhm_y;
    out[i].trate	    = in[i].trate;
    out[i].ccdnum	    = in[i].ccdnum;

    // as of 2011.02.03, the old Mx,My,..., Mxxxx,Myyyy have been deprecated and replaced
    // with the following.  (no real databases used those values -- see
    // libdvo/doc/dvo-images.txt)
    out[i].nLinkAstrom	    = in[i].nLinkAstrom;
    out[i].nLinkPhotom	    = in[i].nLinkPhotom;
    out[i].ubercalDist	    = in[i].dummy3; // new name PS1_V3
    out[i].dXpixSys	    = in[i].dXpixSys;
    out[i].dYpixSys	    = in[i].dYpixSys;
    out[i].dMagSys	    = in[i].dMagSys;
    out[i].nFitAstrom       = in[i].nFitAstrom;
    out[i].nFitPhotom       = in[i].nFitPhotom;
    out[i].photom_map_id    = in[i].photom_map_id;
    out[i].astrom_map_id    = in[i].astrom_map_id;

    // changed or added for PS1_DEV_1
    out[i].imageID	    = in[i].imageID_lo;

    // changed or added for PS1_DEV_2
    out[i].externID	    = 0;
    out[i].sourceID	    = 0;

    // changed or added for PS1_V1
    out[i].flags	    = in[i].code;
    out[i].parentID	    = 0;

    // changed or added for PS1_V2
    out[i].RAo  	    = NAN;
    out[i].DECo  	    = NAN;
    out[i].Radius  	    = NAN;
  }
  return (out);
}

/* convert internal images to panstarrs-format images */
Image_Panstarrs_DEV_1 *ImageInternalTo_Panstarrs_DEV_1 (Image *in, off_t Nvalues) {

  off_t i;
  Image_Panstarrs_DEV_1 *out;

  ALLOCATE_ZERO (out, Image_Panstarrs_DEV_1, Nvalues);

  for (i = 0; i < Nvalues; i++) {
    // this is only safe because the initial 120 bytes in Coords match CoordsDisk
    memcpy (&out[i].coords, &in[i].coords, sizeof(CoordsDisk));

    // RAW_IMAGE_NAME_LEN < DVO_IMAGE_NAME_LEN
    strncpy_nowarn (out[i].name, in[i].name, RAW_IMAGE_NAME_LEN - 1);

    out[i].tzero    	    = in[i].tzero;
    out[i].nstar    	    = in[i].nstar;
    out[i].secz	    	    = in[i].secz;
    out[i].NX	    	    = in[i].NX;
    out[i].NY	    	    = in[i].NY;
    out[i].apmifit  	    = in[i].apmifit;
    out[i].dapmifit 	    = in[i].dapmifit;

    out[i].Mcal	    	    = in[i].McalPSF;
    out[i].dMcal    	    = in[i].dMcal;
    out[i].Xm	    	    = 100.0*log10(in[i].McalChiSq);

    out[i].photcode   	    = in[i].photcode;
    out[i].exptime  	    = in[i].exptime;
    out[i].sidtime  	    = in[i].sidtime;
    out[i].latitude  	    = in[i].latitude;
    out[i].detection_limit  = in[i].detection_limit;
    out[i].saturation_limit = in[i].saturation_limit;
    out[i].cerror	    = in[i].cerror;
    out[i].fwhm_x	    = in[i].fwhm_x;
    out[i].fwhm_y	    = in[i].fwhm_y;
    out[i].trate	    = in[i].trate;
    out[i].ccdnum	    = in[i].ccdnum;

    // as of 2011.02.03, the old Mx,My,..., Mxxxx,Myyyy have been deprecated and replaced
    // with the following.  (no real databases used those values -- see
    // libdvo/doc/dvo-images.txt)
    out[i].nLinkAstrom	    = in[i].nLinkAstrom;
    out[i].nLinkPhotom	    = in[i].nLinkPhotom;
    out[i].dummy3	    = in[i].ubercalDist; // new name PS1_V3
    out[i].dXpixSys	    = in[i].dXpixSys;
    out[i].dYpixSys	    = in[i].dYpixSys;
    out[i].dMagSys	    = in[i].dMagSys;
    out[i].nFitAstrom       = in[i].nFitAstrom;
    out[i].nFitPhotom       = in[i].nFitPhotom;
    out[i].photom_map_id    = in[i].photom_map_id;
    out[i].astrom_map_id    = in[i].astrom_map_id;

    // changed or added for PS1_DEV_1
    out[i].imageID_hi	    = 0;
    out[i].imageID_lo	    = in[i].imageID;

    // changed or added for PS1_V1
    out[i].code		    = in[i].flags;
  }
  return (out);
}

// XXX no photcode conversions?  this may be from before we had a photcode FITS table...


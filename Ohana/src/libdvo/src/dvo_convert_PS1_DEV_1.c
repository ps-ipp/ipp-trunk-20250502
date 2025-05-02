# include <dvo.h>

/* convert PS1_DEV_1 formats to internal formats */

Measure *Measure_PS1_DEV_1_ToInternal (Average *ave, Measure_PS1_DEV_1 *in, off_t Nvalues) {

  off_t i;
  Measure *out;

  ALLOCATE_ZERO (out, Measure, Nvalues);

  for (i = 0; i < Nvalues; i++) {
    dvo_measure_init (&out[i]);

    int averef = in[i].averef;
    out[i].R          = ave[averef].R - in[i].dR / 3600.0;
    out[i].D          = ave[averef].D - in[i].dD / 3600.0;
    out[i].M          = in[i].M;
    out[i].McalPSF    = in[i].Mcal;
    out[i].McalAPER   = in[i].Mcal;
    out[i].dM         = in[i].dM;
    out[i].dt         = in[i].dt;
    out[i].airmass    = in[i].airmass;
    out[i].az         = in[i].az;
    out[i].Xccd       = in[i].Xccd;
    out[i].Yccd       = in[i].Yccd;
    out[i].Sky        = in[i].Sky;
    out[i].dSky       = in[i].dSky;
    out[i].t          = in[i].t;
    out[i].averef     = in[i].averef;
    out[i].detID      = in[i].detID;
    out[i].imageID    = in[i].imageID;
    out[i].psfQF      = in[i].psfQF;
    out[i].psfChisq   = in[i].psfChisq;
    out[i].extNsigma  = in[i].extNsigma;
    out[i].FWx 	      = in[i].FWx;
    out[i].FWy 	      = in[i].FWy;
    out[i].theta      = in[i].theta;
    out[i].photcode   = in[i].photcode;
    out[i].dXccd      = in[i].dXccd;
    out[i].dYccd      = in[i].dYccd;
    out[i].dbFlags    = in[i].dbFlags;
    out[i].photFlags  = in[i].photFlags;

    // changed for PS1_DEV_2
    out[i].Map        = in[i].Mgal;

    // changed for PS1_V1
    out[i].photFlags  = in[i].photFlags | (in[i].dophot << 16);
  }
  return (out);
}

Measure_PS1_DEV_1 *MeasureInternalTo_PS1_DEV_1 (Average *ave, Measure *in, off_t Nvalues) {

  off_t i;
  Measure_PS1_DEV_1 *out;

  ALLOCATE_ZERO (out, Measure_PS1_DEV_1, Nvalues);

  for (i = 0; i < Nvalues; i++) {
    int averef = in[i].averef;

    out[i].dR         = 3600.0*(ave[averef].R - in[i].R);
    out[i].dD         = 3600.0*(ave[averef].D - in[i].D);
    out[i].M          = in[i].M;
    out[i].Mcal       = in[i].McalPSF;
    out[i].dM         = in[i].dM;
    out[i].dt         = in[i].dt;
    out[i].airmass    = in[i].airmass;
    out[i].az         = in[i].az;
    out[i].Xccd       = in[i].Xccd;
    out[i].Yccd       = in[i].Yccd;
    out[i].Sky        = in[i].Sky;
    out[i].dSky       = in[i].dSky;
    out[i].t          = in[i].t;
    out[i].averef     = in[i].averef;
    out[i].detID      = in[i].detID;
    out[i].imageID    = in[i].imageID;
    out[i].psfQF      = in[i].psfQF;
    out[i].psfChisq   = in[i].psfChisq;
    out[i].extNsigma  = in[i].extNsigma;
    out[i].FWx 	      = in[i].FWx;
    out[i].FWy 	      = in[i].FWy;
    out[i].theta      = in[i].theta;
    out[i].photcode   = in[i].photcode;
    out[i].dXccd      = in[i].dXccd;
    out[i].dYccd      = in[i].dYccd;
    out[i].dbFlags    = in[i].dbFlags;

    // changed or added for PS1_DEV_2
    out[i].Mgal       = in[i].Map;

    // changed or added for PS1_V1
    out[i].photFlags  = in[i].photFlags & 0x0000ffff;
    out[i].dophot     = in[i].photFlags >> 16;
  }
  return (out);
}

// 'primary' is needed to conform with the API for Loneos and Elixir, but is not used
Average *Average_PS1_DEV_1_ToInternal (Average_PS1_DEV_1 *in, off_t Nvalues, SecFilt **primary) {
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

    // PS1_V4 : Xp dropped in V4 onward, was not really used anyway
  }
  return (out);
}

// 'primary' is needed to conform with the API for Loneos and Elixir, but is not used
Average_PS1_DEV_1 *AverageInternalTo_PS1_DEV_1 (Average *in, off_t Nvalues, SecFilt *primary) {
  OHANA_UNUSED_PARAM(primary);

  off_t i;
  Average_PS1_DEV_1 *out;

  ALLOCATE_ZERO (out, Average_PS1_DEV_1, Nvalues);

  for (i = 0; i < Nvalues; i++) {
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
    out[i].objID    	 = in[i].objID;
    out[i].catID    	 = in[i].catID;

    // changed or added for PS1_DEV_2
    out[i].Nm       	 = in[i].Nmeasure;     
    out[i].Nn       	 = in[i].Nmissing;     
    out[i].offset   	 = in[i].measureOffset; 
    out[i].missing  	 = in[i].missingOffset;

    // changed or added for PS1_V1
    out[i].code          = in[i].flags;   
  }
  return (out);
}

SecFilt *SecFilt_PS1_DEV_1_ToInternal (SecFilt_PS1_DEV_1 *in, off_t Nvalues) {

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

SecFilt_PS1_DEV_1 *SecFiltInternalTo_PS1_DEV_1 (SecFilt *in, off_t Nvalues) {

  off_t i;
  SecFilt_PS1_DEV_1 *out;

  ALLOCATE_ZERO (out, SecFilt_PS1_DEV_1, Nvalues);

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

Image *Image_PS1_DEV_1_ToInternal (Image_PS1_DEV_1 *in, off_t Nvalues, off_t Nalloc) {

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
    out[i].imageID	    = in[i].imageID;

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
Image_PS1_DEV_1 *ImageInternalTo_PS1_DEV_1 (Image *in, off_t Nvalues) {

  off_t i;
  Image_PS1_DEV_1 *out;

  ALLOCATE_ZERO (out, Image_PS1_DEV_1, Nvalues);

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
    out[i].imageID	    = in[i].imageID;

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

    // changed or added for PS1_V1
    out[i].code		    = in[i].flags;
  }
  return (out);
}

PhotCode *PhotCode_PS1_DEV_1_To_Internal (PhotCode_PS1_DEV_1 *in, off_t Nvalues) {

  off_t i;
  PhotCode *out;

  ALLOCATE_ZERO (out, PhotCode, Nvalues);

  for (i = 0; i < Nvalues; i++) {
    strncpy_nowarn (out[i].name, in[i].name, 31); // out[32], in[32]

    out[i].code  = in[i].code;         
    out[i].type  = in[i].type;         
    out[i].C  	 = in[i].C;            
    out[i].dC 	 = in[i].dC;           
    out[i].dX 	 = in[i].dX;           
    out[i].K  	 = in[i].K;            
    out[i].c1 	 = in[i].c1;           
    out[i].c2 	 = in[i].c2;           
    out[i].equiv = in[i].equiv;        
    out[i].Nc    = in[i].Nc;           
    memcpy (out[i].X, in[i].X, 4*sizeof(float));            

    out[i].astromErrMagScale = in[i].astromErrMagScale;
    out[i].photomErrSys      = in[i].photomErrSys;

    // changed or added for PS1_DEV_2
    out[i].astromErrSys      = 0.0;
    out[i].astromErrScale    = 0.0;

    // changed or added for PS1_V1 (also PS1_DEV_3, deprecated)
    out[i].photomPoorMask      = 0;
    out[i].photomBadMask       = 0;
    out[i].astromPoorMask      = 0;
    out[i].astromBadMask       = 0;
  }
  return (out);
}

PhotCode_PS1_DEV_1 *PhotCode_Internal_To_PS1_DEV_1 (PhotCode *in, off_t Nvalues) {

  off_t i;
  PhotCode_PS1_DEV_1 *out;

  ALLOCATE_ZERO (out, PhotCode_PS1_DEV_1, Nvalues);

  for (i = 0; i < Nvalues; i++) {
    strncpy_nowarn (out[i].name, in[i].name, 31); // out[32], in[32]

    out[i].code = in[i].code;         
    out[i].type = in[i].type;         
    out[i].C  	= in[i].C;            
    out[i].dC 	= in[i].dC;           
    out[i].dX 	= in[i].dX;           
    out[i].K  	= in[i].K;            
    out[i].c1 	= in[i].c1;           
    out[i].c2 	= in[i].c2;           
    out[i].equiv = in[i].equiv;        
    out[i].Nc = in[i].Nc;           
    memcpy (out[i].X, in[i].X, 4*sizeof(float));            

    out[i].astromErrMagScale = out[i].astromErrMagScale;
    out[i].photomErrSys      = out[i].photomErrSys;
  }
  return (out);
}

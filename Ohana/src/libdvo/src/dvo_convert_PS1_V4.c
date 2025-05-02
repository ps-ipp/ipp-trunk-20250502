# include <dvo.h>

/* convert PS1_V4 formats to internal formats */

Measure *Measure_PS1_V4_ToInternal (Average *ave, Measure_PS1_V4 *in, off_t Nvalues) {

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
    out[i].Map        = in[i].Map;
    out[i].Mkron      = in[i].Mkron;
    out[i].dMkron     = in[i].dMkron;
    out[i].dM         = in[i].dM;
    out[i].dMcal      = in[i].dMcal;
    out[i].dt         = in[i].dt;
    out[i].FluxPSF    = in[i].FluxPSF;
    out[i].dFluxPSF   = in[i].dFluxPSF;
    out[i].FluxKron   = in[i].FluxKron;
    out[i].dFluxKron  = in[i].dFluxKron;
    out[i].airmass    = in[i].airmass;
    out[i].az         = in[i].az;
    out[i].Xccd       = in[i].Xccd;
    out[i].Yccd       = in[i].Yccd;
    out[i].Xfix       = in[i].Xfix;
    out[i].Yfix       = in[i].Yfix;
    out[i].Sky        = in[i].Sky;
    out[i].dSky       = in[i].dSky;
    out[i].t          = in[i].t;
    out[i].t_msec     = in[i].t_msec;
    out[i].averef     = in[i].averef;
    out[i].detID      = in[i].detID;
    out[i].imageID    = in[i].imageID;
    out[i].objID      = in[i].objID;
    out[i].catID      = in[i].catID;
    out[i].extID      = in[i].extID;
    out[i].psfQF      = in[i].psfQF;
    out[i].psfQFperf  = in[i].psfQFperf;
    out[i].psfChisq   = in[i].psfChisq;
    out[i].psfNdof    = in[i].psfNdof;
    out[i].psfNpix    = in[i].psfNpix;
    out[i].extNsigma  = in[i].extNsigma;
    out[i].FWx 	      = in[i].FWx;
    out[i].FWy 	      = in[i].FWy;
    out[i].theta      = in[i].theta;
    out[i].Mxx 	      = in[i].Mxx;
    out[i].Mxy 	      = in[i].Mxy;
    out[i].Myy 	      = in[i].Myy;
    out[i].dXccd      = in[i].dXccd;
    out[i].dYccd      = in[i].dYccd;
    out[i].dRsys      = in[i].dRsys;
    out[i].posangle   = in[i].posangle;
    out[i].pltscale   = in[i].pltscale;
    out[i].photcode   = in[i].photcode;
    out[i].dbFlags    = in[i].dbFlags;
    out[i].photFlags  = in[i].photFlags;
  }
  return (out);
}

Measure_PS1_V4 *MeasureInternalTo_PS1_V4 (Average *ave, Measure *in, off_t Nvalues) {

  off_t i;
  Measure_PS1_V4 *out;

  ALLOCATE_ZERO (out, Measure_PS1_V4, Nvalues);

  for (i = 0; i < Nvalues; i++) {
    int averef = in[i].averef;

    out[i].dR         = 3600.0*(ave[averef].R - in[i].R);
    out[i].dD         = 3600.0*(ave[averef].D - in[i].D);
    out[i].M          = in[i].M;
    out[i].Mcal       = in[i].McalPSF;
    out[i].Map        = in[i].Map;
    out[i].Mkron      = in[i].Mkron;
    out[i].dMkron     = in[i].dMkron;
    out[i].dM         = in[i].dM;
    out[i].dMcal      = in[i].dMcal;
    out[i].dt         = in[i].dt;
    out[i].FluxPSF    = in[i].FluxPSF;
    out[i].dFluxPSF   = in[i].dFluxPSF;
    out[i].FluxKron   = in[i].FluxKron;
    out[i].dFluxKron  = in[i].dFluxKron;
    out[i].airmass    = in[i].airmass;
    out[i].az         = in[i].az;
    out[i].Xccd       = in[i].Xccd;
    out[i].Yccd       = in[i].Yccd;
    out[i].Xfix       = in[i].Xfix;
    out[i].Yfix       = in[i].Yfix;
    out[i].Sky        = in[i].Sky;
    out[i].dSky       = in[i].dSky;
    out[i].t          = in[i].t;
    out[i].t_msec     = in[i].t_msec;
    out[i].averef     = in[i].averef;
    out[i].detID      = in[i].detID;
    out[i].imageID    = in[i].imageID;
    out[i].objID      = in[i].objID;
    out[i].catID      = in[i].catID;
    out[i].extID      = in[i].extID;
    out[i].psfQF      = in[i].psfQF;
    out[i].psfQFperf  = in[i].psfQFperf;
    out[i].psfChisq   = in[i].psfChisq;
    out[i].psfNdof    = in[i].psfNdof;
    out[i].psfNpix    = in[i].psfNpix;
    out[i].extNsigma  = in[i].extNsigma;
    out[i].FWx 	      = in[i].FWx;
    out[i].FWy 	      = in[i].FWy;
    out[i].theta      = in[i].theta;
    out[i].Mxx 	      = in[i].Mxx;
    out[i].Mxy 	      = in[i].Mxy;
    out[i].Myy 	      = in[i].Myy;
    out[i].dXccd      = in[i].dXccd;
    out[i].dYccd      = in[i].dYccd;
    out[i].dRsys      = in[i].dRsys;
    out[i].posangle   = in[i].posangle;
    out[i].pltscale   = in[i].pltscale;
    out[i].photcode   = in[i].photcode;
    out[i].dbFlags    = in[i].dbFlags;
    out[i].photFlags  = in[i].photFlags;
  }
  return (out);
}

// 'primary' is needed to conform with the API for Loneos and Elixir, but is not used
Average *Average_PS1_V4_ToInternal (Average_PS1_V4 *in, off_t Nvalues, SecFilt **primary) {
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
    out[i].psfQF         = in[i].psfQF;
    out[i].psfQFperf     = in[i].psfQFperf;
    out[i].stargal     	 = in[i].stargal;     
    out[i].ChiSqAve    	 = in[i].ChiSqAve;     
    out[i].ChiSqPM    	 = in[i].ChiSqPM;     
    out[i].ChiSqPar    	 = in[i].ChiSqPar;     
    out[i].Tmean    	 = in[i].Tmean;     
    out[i].Trange   	 = in[i].Trange;     
    out[i].Npos       	 = in[i].Npos;     
    out[i].Nmeasure      = in[i].Nmeasure;     
    out[i].Nmissing      = in[i].Nmissing;     
    out[i].Ngalphot     = in[i].Ngalphot;     
    out[i].measureOffset = in[i].measureOffset; 
    out[i].missingOffset = in[i].missingOffset;
    out[i].refColorBlue  = in[i].refColor;
    out[i].flags     	 = in[i].flags;   
    out[i].photFlagsUpper = in[i].photFlagsUpper;   
    out[i].photFlagsLower = in[i].photFlagsLower;   
    out[i].objID 	 = in[i].objID;
    out[i].catID 	 = in[i].catID;
    out[i].extID 	 = in[i].extID;
  }
  return (out);
}

// 'primary' is needed to conform with the API for Loneos and Elixir, but is not used
Average_PS1_V4 *AverageInternalTo_PS1_V4 (Average *in, off_t Nvalues, SecFilt *primary) {
  OHANA_UNUSED_PARAM(primary);

  off_t i;
  Average_PS1_V4 *out;

  ALLOCATE_ZERO (out, Average_PS1_V4, Nvalues);

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
    out[i].psfQF         = in[i].psfQF;
    out[i].psfQFperf     = in[i].psfQFperf;
    out[i].stargal     	 = in[i].stargal;     
    out[i].ChiSqAve    	 = in[i].ChiSqAve;     
    out[i].ChiSqPM     	 = in[i].ChiSqPM;     
    out[i].ChiSqPar   	 = in[i].ChiSqPar;     
    out[i].Tmean    	 = in[i].Tmean;     
    out[i].Trange   	 = in[i].Trange;     
    out[i].Npos       	 = in[i].Npos;     
    out[i].Nmeasure      = in[i].Nmeasure;     
    out[i].Nmissing      = in[i].Nmissing;     
    out[i].Ngalphot     = in[i].Ngalphot;     
    out[i].measureOffset = in[i].measureOffset; 
    out[i].missingOffset = in[i].missingOffset;
    out[i].refColor      = in[i].refColorBlue;
    out[i].flags     	 = in[i].flags;   
    out[i].photFlagsUpper = in[i].photFlagsUpper;   
    out[i].photFlagsLower = in[i].photFlagsLower;   
    out[i].objID 	 = in[i].objID;
    out[i].catID 	 = in[i].catID;
    out[i].extID 	 = in[i].extID;
  }
  return (out);
}

SecFilt *SecFilt_PS1_V4_ToInternal (SecFilt_PS1_V4 *in, off_t Nvalues) {

  off_t i;
  SecFilt *out;

  ALLOCATE_ZERO (out, SecFilt, Nvalues);

  for (i = 0; i < Nvalues; i++) {
    dvo_secfilt_init (&out[i], SECFILT_RESET_ALL);

    out[i].MpsfChp       = in[i].M;      
    out[i].sMpsfChp      = in[i].Mstdev;      
    out[i].dMpsfChp      = in[i].dM;      
    out[i].MapChp        = in[i].Map;      
    out[i].MkronChp      = in[i].Mkron;      

    out[i].Mchisq        = in[i].Mchisq;     
    out[i].FpsfStk       = in[i].FluxPSF;
    out[i].dFpsfStk      = in[i].dFluxPSF;
    out[i].FkronStk      = in[i].FluxKron;
    out[i].dFkronStk     = in[i].dFluxKron;
    out[i].flags         = in[i].flags;     
    out[i].Ncode         = in[i].Ncode;
    out[i].Nused         = in[i].Nused;
    out[i].Mmin          = in[i].M_20*0.001;      
    out[i].Mmax          = in[i].M_80*0.001;      
    out[i].ubercalDist   = in[i].ubercalDist;      
    out[i].stackPrmryOff = in[i].stackPrmryOff;      
    out[i].stackBestOff  = in[i].stackBestOff;      
 }
  return (out);
}

SecFilt_PS1_V4 *SecFiltInternalTo_PS1_V4 (SecFilt *in, off_t Nvalues) {

  off_t i;
  SecFilt_PS1_V4 *out;

  ALLOCATE_ZERO (out, SecFilt_PS1_V4, Nvalues);

  for (i = 0; i < Nvalues; i++) {
    out[i].M             = in[i].MpsfChp;      
    out[i].dM            = in[i].dMpsfChp;      
    out[i].Mstdev        = in[i].sMpsfChp;      
    out[i].Map           = in[i].MapChp;      
    out[i].Mkron         = in[i].MkronChp;      

    out[i].Mchisq        = in[i].Mchisq;
    out[i].FluxPSF     	 = in[i].FpsfStk;
    out[i].dFluxPSF    	 = in[i].dFpsfStk;
    out[i].FluxKron    	 = in[i].FkronStk;
    out[i].dFluxKron   	 = in[i].dFkronStk;
    out[i].flags       	 = in[i].flags;     
    out[i].Ncode       	 = in[i].Ncode;
    out[i].Nused       	 = in[i].Nused;
    out[i].M_20        	 = in[i].Mmin*1000.0;      
    out[i].M_80        	 = in[i].Mmax*1000.0;      
    out[i].ubercalDist 	 = in[i].ubercalDist;      
    out[i].stackPrmryOff = in[i].stackPrmryOff;      
    out[i].stackBestOff  = in[i].stackBestOff;      
  }
  return (out);
}

# define RAW_IMAGE_NAME_LEN 121

Image *Image_PS1_V4_ToInternal (Image_PS1_V4 *in, off_t Nvalues, off_t Nalloc) {

  off_t i;
  Image *out;

  char *buffer;
  ALLOCATE_ZERO (buffer, char, Nalloc);
  out = (Image *) buffer;
  // ALLOCATE_ZERO (out, Image, Nvalues);

  for (i = 0; i < Nvalues; i++) {
    // this is only save because the initial 120 bytes in Coords match CoordsDisk
    memcpy (&out[i].coords, &in[i].coords, sizeof(CoordsDisk));
    out[i].coords.mosaic    = NULL;
    out[i].coords.offsetMap = NULL;

    // RAW_IMAGE_NAME_LEN > DVO_IMAGE_NAME_LEN
    strncpy_nowarn (out[i].name, in[i].name, DVO_IMAGE_NAME_LEN - 1);

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

    out[i].RAo  	    = in[i].RAo;
    out[i].DECo  	    = in[i].DECo;
    out[i].Radius  	    = in[i].Radius;
    out[i].refColorBlue	    = in[i].refColor;

    out[i].detection_limit  = in[i].detection_limit;
    out[i].saturation_limit = in[i].saturation_limit;
    out[i].cerror	    = in[i].cerror;
    out[i].fwhm_x	    = in[i].fwhm_x;
    out[i].fwhm_y	    = in[i].fwhm_y;
    out[i].trate	    = in[i].trate;
    out[i].ccdnum	    = in[i].ccdnum;
    out[i].flags	    = in[i].flags;
    out[i].imageID	    = in[i].imageID;
    out[i].parentID	    = in[i].parentID;
    out[i].externID	    = in[i].externID;
    out[i].sourceID	    = in[i].sourceID;

    // as of 2011.02.03, the old Mx,My,..., Mxxxx,Myyyy have been deprecated and replaced
    // with the following.  (no real databases used those values -- see
    // libdvo/doc/dvo-images.txt)
    out[i].nLinkAstrom	    = in[i].nLinkAstrom;
    out[i].nLinkPhotom	    = in[i].nLinkPhotom;
    out[i].ubercalDist	    = in[i].ubercalDist;
    out[i].dXpixSys	    = in[i].dXpixSys;
    out[i].dYpixSys	    = in[i].dYpixSys;
    out[i].dMagSys	    = in[i].dMagSys;
    out[i].nFitAstrom       = in[i].nFitAstrom;
    out[i].nFitPhotom       = in[i].nFitPhotom;
    out[i].photom_map_id    = in[i].photom_map_id;
    out[i].astrom_map_id    = in[i].astrom_map_id;
  }
  return (out);
}

Image_PS1_V4 *ImageInternalTo_PS1_V4 (Image *in, off_t Nvalues) {

  off_t i;
  Image_PS1_V4 *out;

  ALLOCATE_ZERO (out, Image_PS1_V4, Nvalues);

  for (i = 0; i < Nvalues; i++) {
    // this is only save because the initial 120 bytes in Coords match CoordsDisk
    memcpy (&out[i].coords, &in[i].coords, sizeof(CoordsDisk));

    // RAW_IMAGE_NAME_LEN > DVO_IMAGE_NAME_LEN
    strncpy_nowarn (out[i].name, in[i].name, DVO_IMAGE_NAME_LEN - 1);

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

    out[i].RAo  	    = in[i].RAo;
    out[i].DECo  	    = in[i].DECo;
    out[i].Radius  	    = in[i].Radius;
    out[i].refColor  	    = in[i].refColorBlue;

    out[i].detection_limit  = in[i].detection_limit;
    out[i].saturation_limit = in[i].saturation_limit;
    out[i].cerror	    = in[i].cerror;
    out[i].fwhm_x	    = in[i].fwhm_x;
    out[i].fwhm_y	    = in[i].fwhm_y;
    out[i].trate	    = in[i].trate;
    out[i].ccdnum	    = in[i].ccdnum;
    out[i].flags	    = in[i].flags;
    out[i].imageID	    = in[i].imageID;
    out[i].parentID	    = in[i].parentID;
    out[i].externID	    = in[i].externID;
    out[i].sourceID	    = in[i].sourceID;

    // as of 2011.02.03, the old Mx,My,..., Mxxxx,Myyyy have been deprecated and replaced
    // with the following.  (no real databases used those values -- see
    // libdvo/doc/dvo-images.txt)
    out[i].nLinkAstrom	    = in[i].nLinkAstrom;
    out[i].nLinkPhotom	    = in[i].nLinkPhotom;
    out[i].ubercalDist	    = in[i].ubercalDist;
    out[i].dXpixSys	    = in[i].dXpixSys;
    out[i].dYpixSys	    = in[i].dYpixSys;
    out[i].dMagSys	    = in[i].dMagSys;
    out[i].nFitAstrom       = in[i].nFitAstrom;
    out[i].nFitPhotom       = in[i].nFitPhotom;
    out[i].photom_map_id    = in[i].photom_map_id;
    out[i].astrom_map_id    = in[i].astrom_map_id;
  }
  return (out);
}

PhotCode *PhotCode_PS1_V4_To_Internal (PhotCode_PS1_V4 *in, off_t Nvalues) {

  off_t i;
  PhotCode *out;

  ALLOCATE_ZERO (out, PhotCode, Nvalues);

  for (i = 0; i < Nvalues; i++) {
    strncpy_nowarn (out[i].name, in[i].name, 31); // out[32], in[32]

    out[i].code  = in[i].code;         
    out[i].type  = in[i].type;         
    out[i].C     = in[i].C;            
    out[i].dC 	 = in[i].dC;           
    out[i].dX 	 = in[i].dX;           
    out[i].K  	 = in[i].K;            
    out[i].c1 	 = in[i].c1;           
    out[i].c2 	 = in[i].c2;           
    out[i].equiv = in[i].equiv;        
    out[i].Nc    = in[i].Nc;           
    memcpy (out[i].X, in[i].X, 4*sizeof(float));            

    out[i].astromErrSys      = in[i].astromErrSys;
    out[i].astromErrScale    = in[i].astromErrScale;
    out[i].astromErrMagScale = in[i].astromErrMagScale;
    out[i].photomErrSys      = in[i].photomErrSys;

    out[i].photomPoorMask      = in[i].photomPoorMask;
    out[i].photomBadMask       = in[i].photomBadMask;
    out[i].astromPoorMask      = in[i].astromPoorMask;
    out[i].astromBadMask       = in[i].astromBadMask;
  }
  return (out);
}

PhotCode_PS1_V4 *PhotCode_Internal_To_PS1_V4 (PhotCode *in, off_t Nvalues) {

  off_t i;
  PhotCode_PS1_V4 *out;

  ALLOCATE_ZERO (out, PhotCode_PS1_V4, Nvalues);

  for (i = 0; i < Nvalues; i++) {
    strncpy_nowarn (out[i].name, in[i].name, 31); // out[32], in[32]

    out[i].code  = in[i].code;         
    out[i].type  = in[i].type;         
    out[i].C     = in[i].C;            
    out[i].dC 	 = in[i].dC;           
    out[i].dX 	 = in[i].dX;           
    out[i].K  	 = in[i].K;            
    out[i].c1 	 = in[i].c1;           
    out[i].c2 	 = in[i].c2;           
    out[i].equiv = in[i].equiv;        
    out[i].Nc    = in[i].Nc;           
    memcpy (out[i].X, in[i].X, 4*sizeof(float));            

    out[i].astromErrSys      = in[i].astromErrSys;
    out[i].astromErrScale    = in[i].astromErrScale;
    out[i].astromErrMagScale = in[i].astromErrMagScale;
    out[i].photomErrSys      = in[i].photomErrSys;

    out[i].photomPoorMask      = in[i].photomPoorMask;
    out[i].photomBadMask       = in[i].photomBadMask;
    out[i].astromPoorMask      = in[i].astromPoorMask;
    out[i].astromBadMask       = in[i].astromBadMask;
  }
  return (out);
}

/*** there are some mini dvodbs with the wrong PS1_V4 format (missing Xoff,Yoff / Xfix,Yfix) ************/

Measure *Measure_PS1_V4alt_ToInternal (Average *ave, Measure_PS1_V4alt *in, off_t Nvalues) {

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
    out[i].Map        = in[i].Map;
    out[i].Mkron      = in[i].Mkron;
    out[i].dMkron     = in[i].dMkron;
    out[i].dM         = in[i].dM;
    out[i].dMcal      = in[i].dMcal;
    out[i].dt         = in[i].dt;
    out[i].FluxPSF    = in[i].FluxPSF;
    out[i].dFluxPSF   = in[i].dFluxPSF;
    out[i].FluxKron   = in[i].FluxKron;
    out[i].dFluxKron  = in[i].dFluxKron;
    out[i].airmass    = in[i].airmass;
    out[i].az         = in[i].az;
    out[i].Xccd       = in[i].Xccd;
    out[i].Yccd       = in[i].Yccd;
    out[i].Xfix       = in[i].Xccd;
    out[i].Yfix       = in[i].Yccd;
    out[i].Sky        = in[i].Sky;
    out[i].dSky       = in[i].dSky;
    out[i].t          = in[i].t;
    out[i].t_msec     = in[i].t_msec;
    out[i].averef     = in[i].averef;
    out[i].detID      = in[i].detID;
    out[i].imageID    = in[i].imageID;
    out[i].objID      = in[i].objID;
    out[i].catID      = in[i].catID;
    out[i].extID      = in[i].extID;
    out[i].psfQF      = in[i].psfQF;
    out[i].psfQFperf  = in[i].psfQFperf;
    out[i].psfChisq   = in[i].psfChisq;
    out[i].psfNdof    = in[i].psfNdof;
    out[i].psfNpix    = in[i].psfNpix;
    out[i].extNsigma  = in[i].extNsigma;
    out[i].FWx 	      = in[i].FWx;
    out[i].FWy 	      = in[i].FWy;
    out[i].theta      = in[i].theta;
    out[i].Mxx 	      = in[i].Mxx;
    out[i].Mxy 	      = in[i].Mxy;
    out[i].Myy 	      = in[i].Myy;
    out[i].dXccd      = in[i].dXccd;
    out[i].dYccd      = in[i].dYccd;
    out[i].dRsys      = in[i].dRsys;
    out[i].posangle   = in[i].posangle;
    out[i].pltscale   = in[i].pltscale;
    out[i].photcode   = in[i].photcode;
    out[i].dbFlags    = in[i].dbFlags;
    out[i].photFlags  = in[i].photFlags;
  }
  return (out);
}

// XXX note that there are 2 bad versions of PS1_V4 : the other one does have have PSF_QF_PERFECT, but has PAD instead
int gfits_convert_Measure_PS1_V4alt (Measure_PS1_V4alt *data, off_t size, off_t nitems) {

  off_t i;
  unsigned char *byte, tmp;

  if (size != 176) { 
    fprintf (stderr, "WARNING: mismatch in data types Measure_PS1_V4alt: "OFF_T_FMT" vs %d\n",  size,  176);
    return (FALSE);
  }

  /* provide initial values to avoid compiler warnings for non-BYTE_SWAP arch */
  i = tmp = 0;
  byte = NULL;

# ifdef BYTE_SWAP
  byte = (unsigned char *) data;
  for (i = 0; i < nitems; i++, byte += 176) {
    /** BYTE SWAP **/
    SWAP_WORD (0); // D_RA
    SWAP_WORD (4); // D_DEC
    SWAP_WORD (8); // MAG
    SWAP_WORD (12); // M_CAL
    SWAP_WORD (16); // M_APER
    SWAP_WORD (20); // M_KRON
    SWAP_WORD (24); // M_KRON_ERR
    SWAP_WORD (28); // MAG_ERR
    SWAP_WORD (32); // MAG_CAL_ERR
    SWAP_WORD (36); // M_TIME
    SWAP_WORD (40); // FLUX_PSF
    SWAP_WORD (44); // FLUX_PSF_ERR
    SWAP_WORD (48); // FLUX_KRON
    SWAP_WORD (52); // FLUX_KRON_ERR
    SWAP_WORD (56); // AIRMASS
    SWAP_WORD (60); // AZ
    SWAP_WORD (64); // X_CCD
    SWAP_WORD (68); // Y_CCD
    SWAP_WORD (72); // SKY_FLUX	   
    SWAP_WORD (76); // SKY_FLUX_ERR
    SWAP_WORD (80); // TIME
    SWAP_WORD (84); // AVE_REF
    SWAP_WORD (88); // DET_ID
    SWAP_WORD (92); // IMAGE_ID
    SWAP_WORD (96); // OBJ_ID
    SWAP_WORD (100); // CAT_ID
    SWAP_DBLE (104); // EXT_ID
    SWAP_WORD (112); // PSF_QF	      
    SWAP_WORD (116); // PSF_QF_PERFECT 
    SWAP_WORD (120); // PSF_CHISQ     
    SWAP_WORD (124); // PSF_NDOF      
    SWAP_WORD (128); // PSF_NPIX      
    SWAP_WORD (132); // CR_NSIGMA     
    SWAP_WORD (136); // EXT_NSIGMA    
    SWAP_BYTE (140); // FWHM_MAJOR 
    SWAP_BYTE (142); // FWHM_MINOR 
    SWAP_BYTE (144); // PSF_THETA  
    SWAP_BYTE (146); // MXX	   
    SWAP_BYTE (148); // MXY	   
    SWAP_BYTE (150); // MYY	   
    SWAP_BYTE (152); // TIME_MSEC  
    SWAP_BYTE (154); // PHOTCODE   
    SWAP_BYTE (156); // X_CCD_ERR  
    SWAP_BYTE (158); // Y_CCD_ERR  
    SWAP_BYTE (160); // POS_SYS_ERR
    SWAP_BYTE (162); // POSANGLE   
    SWAP_WORD (164); // PLTSCALE  
    SWAP_WORD (168); // DB_FLAGS  
    SWAP_WORD (172); // PHOT_FLAGS
  }
# endif  

  return (TRUE);
} 

/*** add test of EXTNAME and header-defined columns? ***/
/* return internal structure representation */
Measure_PS1_V4alt *gfits_table_get_Measure_PS1_V4alt (FTable *ftable, off_t *Ndata, char *swapped) {

  int Ncols;
  Measure_PS1_V4alt *data;

  Ncols = ftable[0].header[0].Naxis[0];
  if (Ncols != 176) {
    fprintf (stderr, "ERROR: mis-match in table size: width is %d but should be %d bytes\n", Ncols, 176);
    return NULL;
  }

  *Ndata = ftable[0].header[0].Naxis[1];
  data = (Measure_PS1_V4alt *) ftable[0].buffer;
  if ((swapped == NULL) || (*swapped == FALSE)) {
    if (!gfits_convert_Measure_PS1_V4alt (data, sizeof (Measure_PS1_V4alt), *Ndata)) {
      return NULL;
    }
    gfits_table_scale_data (ftable);
    if (swapped != NULL) *swapped = TRUE;
  }
  return (data);
}

// 'primary' is needed to conform with the API for Loneos and Elixir, but is not used
Average *Average_PS1_V4alt_ToInternal (Average_PS1_V4alt *in, off_t Nvalues) {

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
    out[i].ChiSqAve    	 = in[i].ChiSqAve;     
    out[i].ChiSqPM    	 = in[i].ChiSqPM;     
    out[i].ChiSqPar    	 = in[i].ChiSqPar;     
    out[i].Tmean    	 = in[i].Tmean;     
    out[i].Trange   	 = in[i].Trange;     
    out[i].Npos       	 = in[i].Npos;     
    out[i].Nmeasure      = in[i].Nmeasure;     
    out[i].Nmissing      = in[i].Nmissing;     
    out[i].Ngalphot     = in[i].Ngalphot;     
    out[i].measureOffset = in[i].measureOffset; 
    out[i].missingOffset = in[i].missingOffset;
    out[i].refColorBlue  = in[i].refColor;
    out[i].flags     	 = in[i].flags;   
    out[i].photFlagsUpper = in[i].photFlagsUpper;   
    out[i].photFlagsLower = in[i].photFlagsLower;   
    out[i].objID 	 = in[i].objID;
    out[i].catID 	 = in[i].catID;
    out[i].extID 	 = in[i].extID;
  }
  return (out);
}

/******/

int gfits_convert_Average_PS1_V4alt (Average_PS1_V4alt *data, off_t size, off_t nitems) {

  off_t i;
  unsigned char *byte, tmp;

  if (size != 120) { 
    fprintf (stderr, "WARNING: mismatch in data types Average_PS1_V4alt: "OFF_T_FMT" vs %d\n",  size,  120);
    return (FALSE);
  }

  /* provide initial values to avoid compiler warnings for non-BYTE_SWAP arch */
  i = tmp = 0;
  byte = NULL;

# ifdef BYTE_SWAP
  byte = (unsigned char *) data;
  for (i = 0; i < nitems; i++, byte += 120) {
    /** BYTE SWAP **/
    SWAP_DBLE (0); // RA
    SWAP_DBLE (8); // DEC
    SWAP_WORD (16); // RA_ERR
    SWAP_WORD (20); // DEC_ERR
    SWAP_WORD (24); // U_RA
    SWAP_WORD (28); // U_DEC
    SWAP_WORD (32); // V_RA_ERR
    SWAP_WORD (36); // V_DEC_ERR
    SWAP_WORD (40); // PAR
    SWAP_WORD (44); // PAR_ERR
    SWAP_WORD (48); // CHISQ_POS
    SWAP_WORD (52); // CHISQ_PM
    SWAP_WORD (56); // CHISQ_PAP
    SWAP_WORD (60); // MEAN_EPOCH
    SWAP_WORD (64); // TIME_RANGE
    SWAP_WORD (68); // SIGMA
    SWAP_BYTE (72); // NUMBER_POS
    SWAP_BYTE (74); // NMEASURE
    SWAP_BYTE (76); // NMISSING
    SWAP_BYTE (78); // NEXTEND
    SWAP_WORD (80); // OFF_MEASURE
    SWAP_WORD (84); // OFF_MISSING
    SWAP_WORD (88); // OFF_EXTEND
    SWAP_WORD (92); // FLAGS
    SWAP_WORD (96); // PHOTFLAGS_U
    SWAP_WORD (100); // PHOTFLAGS_L
    SWAP_WORD (104); // OBJ_ID
    SWAP_WORD (108); // CAT_ID
    SWAP_DBLE (112); // EXT_ID
  }
# endif  

  return (TRUE);
} 

/*** add test of EXTNAME and header-defined columns? ***/
/* return internal structure representation */
Average_PS1_V4alt *gfits_table_get_Average_PS1_V4alt (FTable *ftable, off_t *Ndata, char *swapped) {

  int Ncols;
  Average_PS1_V4alt *data;

  Ncols = ftable[0].header[0].Naxis[0];
  if (Ncols != 120) {
    fprintf (stderr, "ERROR: mis-match in table size: width is %d but should be %d bytes\n", Ncols, 120);
    return NULL;
  }

  *Ndata = ftable[0].header[0].Naxis[1];
  data = (Average_PS1_V4alt *) ftable[0].buffer;
  if ((swapped == NULL) || (*swapped == FALSE)) {
    if (!gfits_convert_Average_PS1_V4alt (data, sizeof (Average_PS1_V4alt), *Ndata)) {
      return NULL;
    }
    gfits_table_scale_data (ftable);
    if (swapped != NULL) *swapped = TRUE;
  }
  return (data);
}


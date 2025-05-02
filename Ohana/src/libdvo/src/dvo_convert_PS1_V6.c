# include <dvo.h>

/* convert PS1_V6 formats to internal formats */

Measure *Measure_PS1_V6_ToInternal (Average *ave, Measure_PS1_V6 *in, off_t Nvalues) {
  OHANA_UNUSED_PARAM(ave);

  off_t i;
  Measure *out;

  ALLOCATE_ZERO (out, Measure, Nvalues);

  for (i = 0; i < Nvalues; i++) {
    dvo_measure_init (&out[i]);

    out[i].R          = in[i].R;
    out[i].D          = in[i].D;
    out[i].M          = in[i].M;
    out[i].dM         = in[i].dM;
    out[i].Map        = in[i].Map;
    out[i].dMap       = in[i].dMap;
    out[i].Mkron      = in[i].Mkron;
    out[i].dMkron     = in[i].dMkron;
    out[i].McalPSF    = in[i].McalPSF;
    out[i].McalAPER   = in[i].McalAPER;
    out[i].dMcal      = in[i].dMcal;
    out[i].dt         = in[i].dt;
    out[i].FluxPSF    = in[i].FluxPSF;
    out[i].dFluxPSF   = in[i].dFluxPSF;
    out[i].FluxKron   = in[i].FluxKron;
    out[i].dFluxKron  = in[i].dFluxKron;
    out[i].FluxAp     = in[i].FluxAp;
    out[i].dFluxAp    = in[i].dFluxAp;
    out[i].airmass    = in[i].airmass;
    out[i].az         = in[i].az;
    out[i].Xccd       = in[i].Xccd;
    out[i].Yccd       = in[i].Yccd;
    out[i].Xfix       = in[i].Xfix;
    out[i].Yfix       = in[i].Yfix;
    out[i].XoffKH     = in[i].XoffKH;
    out[i].YoffKH     = in[i].YoffKH;
    out[i].XoffDCR    = in[i].XoffDCR;
    out[i].YoffDCR    = in[i].YoffDCR;
    out[i].XoffCAM    = in[i].XoffCAM;
    out[i].YoffCAM    = in[i].YoffCAM;
    out[i].Mflat      = in[i].Mflat;
    out[i].Sky        = in[i].Sky;
    out[i].dSky       = in[i].dSky;
    out[i].t          = in[i].t;
    out[i].averef     = in[i].averef;
    out[i].detID      = in[i].detID;
    out[i].objID      = in[i].objID;
    out[i].catID      = in[i].catID;
    out[i].extID      = in[i].extID;
    out[i].imageID    = in[i].imageID;
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
    out[i].t_msec     = in[i].t_msec;
    out[i].photcode   = in[i].photcode;
    out[i].dXccd      = in[i].dXccd;
    out[i].dYccd      = in[i].dYccd;
    out[i].dRsys      = in[i].dRsys;
    out[i].posangle   = in[i].posangle;
    out[i].pltscale   = in[i].pltscale;
    out[i].dbFlags    = in[i].dbFlags;
    out[i].photFlags  = in[i].photFlags;
    out[i].photFlags2 = in[i].photFlags2;
  }
  return (out);
}

Measure_PS1_V6 *MeasureInternalTo_PS1_V6 (Average *ave, Measure *in, off_t Nvalues) {
  OHANA_UNUSED_PARAM(ave);

  off_t i;
  Measure_PS1_V6 *out;

  ALLOCATE_ZERO (out, Measure_PS1_V6, Nvalues);

  for (i = 0; i < Nvalues; i++) {
    out[i].R          = in[i].R;
    out[i].D          = in[i].D;
    out[i].M          = in[i].M;
    out[i].dM         = in[i].dM;
    out[i].Map        = in[i].Map;
    out[i].dMap       = in[i].dMap;
    out[i].Mkron      = in[i].Mkron;
    out[i].dMkron     = in[i].dMkron;
    out[i].McalPSF    = in[i].McalPSF;
    out[i].McalAPER   = in[i].McalAPER;
    out[i].dMcal      = in[i].dMcal;
    out[i].dt         = in[i].dt;
    out[i].FluxPSF    = in[i].FluxPSF;
    out[i].dFluxPSF   = in[i].dFluxPSF;
    out[i].FluxKron   = in[i].FluxKron;
    out[i].dFluxKron  = in[i].dFluxKron;
    out[i].FluxAp     = in[i].FluxAp;
    out[i].dFluxAp    = in[i].dFluxAp;
    out[i].airmass    = in[i].airmass;
    out[i].az         = in[i].az;
    out[i].Xccd       = in[i].Xccd;
    out[i].Yccd       = in[i].Yccd;
    out[i].Xfix       = in[i].Xfix;
    out[i].Yfix       = in[i].Yfix;
    out[i].XoffKH     = in[i].XoffKH;
    out[i].YoffKH     = in[i].YoffKH;
    out[i].XoffDCR    = in[i].XoffDCR;
    out[i].YoffDCR    = in[i].YoffDCR;
    out[i].XoffCAM    = in[i].XoffCAM;
    out[i].YoffCAM    = in[i].YoffCAM;
    out[i].Mflat      = in[i].Mflat;
    out[i].Sky        = in[i].Sky;
    out[i].dSky       = in[i].dSky;
    out[i].t          = in[i].t;
    out[i].averef     = in[i].averef;
    out[i].detID      = in[i].detID;
    out[i].objID      = in[i].objID;
    out[i].catID      = in[i].catID;
    out[i].extID      = in[i].extID;
    out[i].imageID    = in[i].imageID;
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
    out[i].t_msec     = in[i].t_msec;
    out[i].photcode   = in[i].photcode;
    out[i].dXccd      = in[i].dXccd;
    out[i].dYccd      = in[i].dYccd;
    out[i].dRsys      = in[i].dRsys;
    out[i].posangle   = in[i].posangle;
    out[i].pltscale   = in[i].pltscale;
    out[i].dbFlags    = in[i].dbFlags;
    out[i].photFlags  = in[i].photFlags;
    out[i].photFlags2 = in[i].photFlags2;
  }
  return (out);
}

// 'primary' is needed to conform with the API for Loneos and Elixir, but is not used
Average *Average_PS1_V6_ToInternal (Average_PS1_V6 *in, off_t Nvalues, SecFilt **primary) {
  OHANA_UNUSED_PARAM(primary);

  off_t i;
  Average *out;

  ALLOCATE_ZERO (out, Average, Nvalues);

  for (i = 0; i < Nvalues; i++) {
    dvo_average_init (&out[i]);

    out[i].R        	  = in[i].R;      
    out[i].D        	  = in[i].D;      
    out[i].dR       	  = in[i].dR;
    out[i].dD       	  = in[i].dD;
    out[i].uR       	  = in[i].uR;
    out[i].uD       	  = in[i].uD;
    out[i].duR      	  = in[i].duR;
    out[i].duD      	  = in[i].duD;
    out[i].P        	  = in[i].P;
    out[i].dP       	  = in[i].dP;
			  
    out[i].Rstk        	  = in[i].Rstk;      
    out[i].Dstk        	  = in[i].Dstk;      
    out[i].dRstk       	  = in[i].dRstk;
    out[i].dDstk       	  = in[i].dDstk;
			  
    out[i].ChiSqAve    	  = in[i].ChiSqAve;     
    out[i].ChiSqPM    	  = in[i].ChiSqPM;     
    out[i].ChiSqPar    	  = in[i].ChiSqPar;     
    out[i].Tmean    	  = in[i].Tmean;     
    out[i].Trange   	  = in[i].Trange;     
			  
    out[i].psfQF          = in[i].psfQF;
    out[i].psfQFperf      = in[i].psfQFperf;
    out[i].stargal     	  = in[i].stargal;     
    out[i].Npos       	  = in[i].Npos;     
			  
    out[i].Nmeasure       = in[i].Nmeasure;     
    out[i].Nmissing       = in[i].Nmissing;     
    out[i].Nlensing       = in[i].Nlensing;     
    out[i].Nlensobj       = in[i].Nlensobj;     
    out[i].Nstarpar       = in[i].Nstarpar;     
    out[i].Ngalphot       = in[i].Ngalphot;     
			  
    out[i].measureOffset  = in[i].measureOffset; 
    out[i].missingOffset  = in[i].missingOffset;
    out[i].lensingOffset  = in[i].lensingOffset;
    out[i].lensobjOffset  = in[i].lensobjOffset;
    out[i].starparOffset  = in[i].starparOffset;
    out[i].galphotOffset  = in[i].galphotOffset;

    out[i].refColorBlue   = in[i].refColorBlue;
    out[i].refColorRed    = in[i].refColorRed;
			  
    out[i].tessID         = in[i].tessID;
    out[i].skycellID      = in[i].skycellID;
    out[i].projectionID   = in[i].projectionID;
			  
    out[i].NwarpOK     	  = in[i].NwarpOK;   

    out[i].flags     	  = in[i].flags;   
    out[i].photFlagsUpper = in[i].photFlagsUpper;   
    out[i].photFlagsLower = in[i].photFlagsLower;   
    out[i].objID 	  = in[i].objID;
    out[i].catID 	  = in[i].catID;
    out[i].extID 	  = in[i].extID;
			  
    out[i].uRgal 	  = in[i].uRgal;
    out[i].uDgal 	  = in[i].uDgal;
  }
  return (out);
}

// 'primary' is needed to conform with the API for Loneos and Elixir, but is not used
Average_PS1_V6 *AverageInternalTo_PS1_V6 (Average *in, off_t Nvalues, SecFilt *primary) {
  OHANA_UNUSED_PARAM(primary);

  off_t i;
  Average_PS1_V6 *out;

  ALLOCATE_ZERO (out, Average_PS1_V6, Nvalues);

  for (i = 0; i < Nvalues; i++) {
    out[i].R        	  = in[i].R;      
    out[i].D        	  = in[i].D;      
    out[i].dR       	  = in[i].dR;
    out[i].dD       	  = in[i].dD;
    out[i].uR       	  = in[i].uR;
    out[i].uD       	  = in[i].uD;
    out[i].duR      	  = in[i].duR;
    out[i].duD      	  = in[i].duD;
    out[i].P        	  = in[i].P;
    out[i].dP       	  = in[i].dP;
			  
    out[i].Rstk        	  = in[i].Rstk;      
    out[i].Dstk        	  = in[i].Dstk;      
    out[i].dRstk       	  = in[i].dRstk;
    out[i].dDstk       	  = in[i].dDstk;
			  
    out[i].ChiSqAve    	  = in[i].ChiSqAve;     
    out[i].ChiSqPM     	  = in[i].ChiSqPM;     
    out[i].ChiSqPar   	  = in[i].ChiSqPar;     
    out[i].Tmean    	  = in[i].Tmean;     
    out[i].Trange   	  = in[i].Trange;     
			  
    out[i].psfQF          = in[i].psfQF;
    out[i].psfQFperf      = in[i].psfQFperf;
    out[i].stargal     	  = in[i].stargal;     
    out[i].Npos       	  = in[i].Npos;     
			  
    out[i].Nmeasure       = in[i].Nmeasure;     
    out[i].Nmissing       = in[i].Nmissing;     
    out[i].Nlensing       = in[i].Nlensing;     
    out[i].Nlensobj       = in[i].Nlensobj;     
    out[i].Nstarpar       = in[i].Nstarpar;     
    out[i].Ngalphot        = in[i].Ngalphot;     
			  
    out[i].measureOffset  = in[i].measureOffset; 
    out[i].missingOffset  = in[i].missingOffset;
    out[i].lensingOffset  = in[i].lensingOffset;
    out[i].lensobjOffset  = in[i].lensobjOffset;
    out[i].starparOffset  = in[i].starparOffset;
    out[i].galphotOffset   = in[i].galphotOffset;
			  
    out[i].refColorBlue   = in[i].refColorBlue;
    out[i].refColorRed    = in[i].refColorRed;
			  
    out[i].tessID         = in[i].tessID;
    out[i].skycellID      = in[i].skycellID;
    out[i].projectionID   = in[i].projectionID;
			  
    out[i].NwarpOK     	  = in[i].NwarpOK;   

    out[i].flags     	  = in[i].flags;   
    out[i].photFlagsUpper = in[i].photFlagsUpper;   
    out[i].photFlagsLower = in[i].photFlagsLower;   
    out[i].objID 	  = in[i].objID;
    out[i].catID 	  = in[i].catID;
    out[i].extID 	  = in[i].extID;

    out[i].uRgal 	 = in[i].uRgal;
    out[i].uDgal 	 = in[i].uDgal;
  }
  return (out);
}

SecFilt *SecFilt_PS1_V6_ToInternal (SecFilt_PS1_V6 *in, off_t Nvalues) {

  off_t i;
  SecFilt *out;

  ALLOCATE_ZERO (out, SecFilt, Nvalues);

  for (i = 0; i < Nvalues; i++) {
    dvo_secfilt_init (&out[i], SECFILT_RESET_ALL);

    out[i].MpsfChp       = in[i].M;      
    out[i].sMpsfChp      = in[i].Mstdev;      
    out[i].dMpsfChp      = in[i].dM;      
    out[i].MapChp        = in[i].Map;      
    out[i].dMapChp       = in[i].dMap;      
    out[i].sMapChp       = in[i].sMap;      
    out[i].MkronChp      = in[i].Mkron;      
    out[i].dMkronChp     = in[i].dMkron;      
    out[i].sMkronChp     = in[i].sMkron;      

    out[i].psfQfMax      = in[i].psfQfMax;      
    out[i].psfQfPerfMax  = in[i].psfQfPerfMax;      

    out[i].Mmin          = in[i].Mmin;      
    out[i].Mmax          = in[i].Mmax;      
    out[i].Mchisq        = in[i].Mchisq;     

    out[i].Ncode         = in[i].Ncode;
    out[i].Nused         = in[i].Nused;
    out[i].NusedKron     = in[i].NusedKron;
    out[i].NusedAp       = in[i].NusedAp;

    out[i].flags         = in[i].flags;     

    out[i].MpsfStk       = in[i].MpsfStk;
    out[i].FpsfStk       = in[i].FpsfStk;
    out[i].dFpsfStk      = in[i].dFpsfStk;

    out[i].MkronStk      = in[i].MkronStk;
    out[i].FkronStk      = in[i].FkronStk;
    out[i].dFkronStk     = in[i].dFkronStk;

    out[i].MapStk        = in[i].MapStk;
    out[i].FapStk        = in[i].FapStk;
    out[i].dFapStk       = in[i].dFapStk;

    out[i].Nstack        = in[i].Nstack;      
    out[i].NstackDet     = in[i].NstackDet;      

    out[i].stackPrmryOff = in[i].stackPrmryOff;      
    out[i].stackBestOff  = in[i].stackBestOff;      

    out[i].MpsfWrp       = in[i].MpsfWrp;
    out[i].FpsfWrp       = in[i].FpsfWrp;
    out[i].dFpsfWrp      = in[i].dFpsfWrp;
    out[i].sFpsfWrp      = in[i].sFpsfWrp;

    out[i].MkronWrp      = in[i].MkronWrp;
    out[i].FkronWrp      = in[i].FkronWrp;
    out[i].dFkronWrp     = in[i].dFkronWrp;
    out[i].sFkronWrp     = in[i].sFkronWrp;

    out[i].MapWrp        = in[i].MapWrp;
    out[i].FapWrp        = in[i].FapWrp;
    out[i].dFapWrp       = in[i].dFapWrp;
    out[i].sFapWrp       = in[i].sFapWrp;

    out[i].NusedWrp      = in[i].NusedWrp;
    out[i].NusedKronWrp  = in[i].NusedKronWrp;
    out[i].NusedApWrp    = in[i].NusedApWrp;

    out[i].Nwarp         = in[i].Nwarp;      
    out[i].NwarpGood     = in[i].NwarpGood;      

    out[i].ubercalDist   = in[i].ubercalDist;      
  }
  return (out);
}

SecFilt_PS1_V6 *SecFiltInternalTo_PS1_V6 (SecFilt *in, off_t Nvalues) {

  off_t i;
  SecFilt_PS1_V6 *out;

  ALLOCATE_ZERO (out, SecFilt_PS1_V6, Nvalues);

  for (i = 0; i < Nvalues; i++) {

    out[i].M             = in[i].MpsfChp;      
    out[i].dM            = in[i].dMpsfChp;      
    out[i].Mstdev        = in[i].sMpsfChp;      
    out[i].Map           = in[i].MapChp;      
    out[i].dMap          = in[i].dMapChp;      
    out[i].sMap          = in[i].sMapChp;      
    out[i].Mkron         = in[i].MkronChp;      
    out[i].dMkron        = in[i].dMkronChp;      
    out[i].sMkron        = in[i].sMkronChp;      

    out[i].psfQfMax      = in[i].psfQfMax;      
    out[i].psfQfPerfMax  = in[i].psfQfPerfMax;      

    out[i].Mmin          = in[i].Mmin;      
    out[i].Mmax          = in[i].Mmax;      
    out[i].Mchisq        = in[i].Mchisq;     

    out[i].Ncode         = in[i].Ncode;
    out[i].Nused         = in[i].Nused;
    out[i].NusedKron     = in[i].NusedKron;
    out[i].NusedAp       = in[i].NusedAp;

    out[i].flags         = in[i].flags;     

    out[i].MpsfStk       = in[i].MpsfStk;
    out[i].FpsfStk       = in[i].FpsfStk;
    out[i].dFpsfStk      = in[i].dFpsfStk;

    out[i].MkronStk      = in[i].MkronStk;
    out[i].FkronStk      = in[i].FkronStk;
    out[i].dFkronStk     = in[i].dFkronStk;

    out[i].MapStk        = in[i].MapStk;
    out[i].FapStk        = in[i].FapStk;
    out[i].dFapStk       = in[i].dFapStk;

    out[i].Nstack        = in[i].Nstack;      
    out[i].NstackDet     = in[i].NstackDet;      

    out[i].stackPrmryOff = in[i].stackPrmryOff;      
    out[i].stackBestOff  = in[i].stackBestOff;      

    out[i].MpsfWrp       = in[i].MpsfWrp;
    out[i].FpsfWrp       = in[i].FpsfWrp;
    out[i].dFpsfWrp      = in[i].dFpsfWrp;
    out[i].sFpsfWrp      = in[i].sFpsfWrp;

    out[i].MkronWrp      = in[i].MkronWrp;
    out[i].FkronWrp      = in[i].FkronWrp;
    out[i].dFkronWrp     = in[i].dFkronWrp;
    out[i].sFkronWrp     = in[i].sFkronWrp;

    out[i].MapWrp        = in[i].MapWrp;
    out[i].FapWrp        = in[i].FapWrp;
    out[i].dFapWrp       = in[i].dFapWrp;
    out[i].sFapWrp       = in[i].sFapWrp;

    out[i].NusedWrp      = in[i].NusedWrp;
    out[i].NusedKronWrp  = in[i].NusedKronWrp;
    out[i].NusedApWrp    = in[i].NusedApWrp;

    out[i].Nwarp         = in[i].Nwarp;      
    out[i].NwarpGood     = in[i].NwarpGood;      

    out[i].ubercalDist   = in[i].ubercalDist;      
  }
  return (out);
}

Lensing *Lensing_PS1_V6_ToInternal (Lensing_PS1_V6 *in, off_t Nvalues) {

  off_t i;
  Lensing *out;

  ALLOCATE_ZERO (out, Lensing, Nvalues);

  for (i = 0; i < Nvalues; i++) {
    dvo_lensing_init (&out[i]);

    out[i].X11_sm_obj  = in[i].X11_sm_obj;      
    out[i].X12_sm_obj  = in[i].X12_sm_obj;      
    out[i].X22_sm_obj  = in[i].X22_sm_obj;      
    out[i].E1_sm_obj   = in[i].E1_sm_obj;      
    out[i].E2_sm_obj   = in[i].E2_sm_obj;      
	             	                 
    out[i].X11_sh_obj  = in[i].X11_sh_obj;      
    out[i].X12_sh_obj  = in[i].X12_sh_obj;      
    out[i].X22_sh_obj  = in[i].X22_sh_obj;      
    out[i].E1_sh_obj   = in[i].E1_sh_obj;      
    out[i].E2_sh_obj   = in[i].E2_sh_obj;     
	             	                 
    out[i].X11_sm_psf  = in[i].X11_sm_psf;
    out[i].X12_sm_psf  = in[i].X12_sm_psf;
    out[i].X22_sm_psf  = in[i].X22_sm_psf;
    out[i].E1_sm_psf   = in[i].E1_sm_psf;
    out[i].E2_sm_psf   = in[i].E2_sm_psf;     
	             	                 
    out[i].X11_sh_psf  = in[i].X11_sh_psf;
    out[i].X12_sh_psf  = in[i].X12_sh_psf;
    out[i].X22_sh_psf  = in[i].X22_sh_psf;
    out[i].E1_sh_psf   = in[i].E1_sh_psf;
    out[i].E2_sh_psf   = in[i].E2_sh_psf;

    out[i].E1_psf      = in[i].E1_psf;
    out[i].E2_psf      = in[i].E2_psf;

    out[i].F_ApR5      = in[i].F_ApR5;
    out[i].dF_ApR5     = in[i].dF_ApR5;
    out[i].sF_ApR5     = in[i].sF_ApR5;
    out[i].fF_ApR5     = in[i].fF_ApR5;
	               	            
    out[i].F_ApR6      = in[i].F_ApR6;
    out[i].dF_ApR6     = in[i].dF_ApR6;
    out[i].sF_ApR6     = in[i].sF_ApR6;
    out[i].fF_ApR6     = in[i].fF_ApR6;
	          	            
    out[i].F_ApR7      = in[i].F_ApR7;
    out[i].dF_ApR7     = in[i].dF_ApR7;
    out[i].sF_ApR7     = in[i].sF_ApR7;
    out[i].fF_ApR7     = in[i].fF_ApR7;
	          	            
    out[i].detID       = in[i].detID;
    out[i].objID       = in[i].objID;
    out[i].catID       = in[i].catID;
    out[i].averef      = in[i].averef;

    out[i].imageID     = in[i].imageID;
    out[i].oldImID     = in[i].oldImID;
  }
  return (out);
}

Lensing_PS1_V6 *LensingInternalTo_PS1_V6 (Lensing *in, off_t Nvalues) {

  off_t i;
  Lensing_PS1_V6 *out;

  ALLOCATE_ZERO (out, Lensing_PS1_V6, Nvalues);

  for (i = 0; i < Nvalues; i++) {

    out[i].X11_sm_obj  = in[i].X11_sm_obj;      
    out[i].X12_sm_obj  = in[i].X12_sm_obj;      
    out[i].X22_sm_obj  = in[i].X22_sm_obj;      
    out[i].E1_sm_obj   = in[i].E1_sm_obj;      
    out[i].E2_sm_obj   = in[i].E2_sm_obj;      
	             	                 
    out[i].X11_sh_obj  = in[i].X11_sh_obj;      
    out[i].X12_sh_obj  = in[i].X12_sh_obj;      
    out[i].X22_sh_obj  = in[i].X22_sh_obj;      
    out[i].E1_sh_obj   = in[i].E1_sh_obj;      
    out[i].E2_sh_obj   = in[i].E2_sh_obj;     
	             	                 
    out[i].X11_sm_psf  = in[i].X11_sm_psf;
    out[i].X12_sm_psf  = in[i].X12_sm_psf;
    out[i].X22_sm_psf  = in[i].X22_sm_psf;
    out[i].E1_sm_psf   = in[i].E1_sm_psf;
    out[i].E2_sm_psf   = in[i].E2_sm_psf;     
	             	                 
    out[i].X11_sh_psf  = in[i].X11_sh_psf;
    out[i].X12_sh_psf  = in[i].X12_sh_psf;
    out[i].X22_sh_psf  = in[i].X22_sh_psf;
    out[i].E1_sh_psf   = in[i].E1_sh_psf;
    out[i].E2_sh_psf   = in[i].E2_sh_psf;

    out[i].E1_psf      = in[i].E1_psf;
    out[i].E2_psf      = in[i].E2_psf;

    out[i].F_ApR5      = in[i].F_ApR5;
    out[i].dF_ApR5     = in[i].dF_ApR5;
    out[i].sF_ApR5     = in[i].sF_ApR5;
    out[i].fF_ApR5     = in[i].fF_ApR5;
	               	            
    out[i].F_ApR6      = in[i].F_ApR6;
    out[i].dF_ApR6     = in[i].dF_ApR6;
    out[i].sF_ApR6     = in[i].sF_ApR6;
    out[i].fF_ApR6     = in[i].fF_ApR6;
	          	            
    out[i].F_ApR7      = in[i].F_ApR7;
    out[i].dF_ApR7     = in[i].dF_ApR7;
    out[i].sF_ApR7     = in[i].sF_ApR7;
    out[i].fF_ApR7     = in[i].fF_ApR7;
	          	            
    out[i].detID       = in[i].detID;
    out[i].objID       = in[i].objID;
    out[i].catID       = in[i].catID;
    out[i].averef      = in[i].averef;

    out[i].imageID     = in[i].imageID;
    out[i].oldImID     = in[i].oldImID;
  }
  return (out);
}

Lensobj *Lensobj_PS1_V6_ToInternal (Lensobj_PS1_V6 *in, off_t Nvalues) {

  off_t i;
  Lensobj *out;

  ALLOCATE_ZERO (out, Lensobj, Nvalues);

  for (i = 0; i < Nvalues; i++) {
    dvo_lensobj_init (&out[i], FALSE);

    out[i].X11_sm_obj  = in[i].X11_sm_obj;      
    out[i].X12_sm_obj  = in[i].X12_sm_obj;      
    out[i].X22_sm_obj  = in[i].X22_sm_obj;      
    out[i].E1_sm_obj   = in[i].E1_sm_obj;      
    out[i].E2_sm_obj   = in[i].E2_sm_obj;      
	             	                 
    out[i].X11_sh_obj  = in[i].X11_sh_obj;      
    out[i].X12_sh_obj  = in[i].X12_sh_obj;      
    out[i].X22_sh_obj  = in[i].X22_sh_obj;      
    out[i].E1_sh_obj   = in[i].E1_sh_obj;      
    out[i].E2_sh_obj   = in[i].E2_sh_obj;     
	             	                 
    out[i].X11_sm_psf  = in[i].X11_sm_psf;
    out[i].X12_sm_psf  = in[i].X12_sm_psf;
    out[i].X22_sm_psf  = in[i].X22_sm_psf;
    out[i].E1_sm_psf   = in[i].E1_sm_psf;
    out[i].E2_sm_psf   = in[i].E2_sm_psf;     
	             	                 
    out[i].X11_sh_psf  = in[i].X11_sh_psf;
    out[i].X12_sh_psf  = in[i].X12_sh_psf;
    out[i].X22_sh_psf  = in[i].X22_sh_psf;
    out[i].E1_sh_psf   = in[i].E1_sh_psf;
    out[i].E2_sh_psf   = in[i].E2_sh_psf;

    out[i].F_ApR5      = in[i].F_ApR5;
    out[i].dF_ApR5     = in[i].dF_ApR5;
    out[i].sF_ApR5     = in[i].sF_ApR5;
    out[i].fF_ApR5     = in[i].fF_ApR5;
	               	            
    out[i].F_ApR6      = in[i].F_ApR6;
    out[i].dF_ApR6     = in[i].dF_ApR6;
    out[i].sF_ApR6     = in[i].sF_ApR6;
    out[i].fF_ApR6     = in[i].fF_ApR6;
	          	            
    out[i].F_ApR7      = in[i].F_ApR7;
    out[i].dF_ApR7     = in[i].dF_ApR7;
    out[i].sF_ApR7     = in[i].sF_ApR7;
    out[i].fF_ApR7     = in[i].fF_ApR7;
	          	            
    out[i].gamma       = in[i].gamma;
    out[i].E1          = in[i].E1;
    out[i].E2          = in[i].E2;

    out[i].objID       = in[i].objID;
    out[i].catID       = in[i].catID;

    out[i].photcode    = in[i].photcode;
    out[i].Nmeas       = in[i].Nmeas;
  }
  return (out);
}

Lensobj_PS1_V6 *LensobjInternalTo_PS1_V6 (Lensobj *in, off_t Nvalues) {

  off_t i;
  Lensobj_PS1_V6 *out;

  ALLOCATE_ZERO (out, Lensobj_PS1_V6, Nvalues);

  for (i = 0; i < Nvalues; i++) {

    out[i].X11_sm_obj  = in[i].X11_sm_obj;      
    out[i].X12_sm_obj  = in[i].X12_sm_obj;      
    out[i].X22_sm_obj  = in[i].X22_sm_obj;      
    out[i].E1_sm_obj   = in[i].E1_sm_obj;      
    out[i].E2_sm_obj   = in[i].E2_sm_obj;      
	             	                 
    out[i].X11_sh_obj  = in[i].X11_sh_obj;      
    out[i].X12_sh_obj  = in[i].X12_sh_obj;      
    out[i].X22_sh_obj  = in[i].X22_sh_obj;      
    out[i].E1_sh_obj   = in[i].E1_sh_obj;      
    out[i].E2_sh_obj   = in[i].E2_sh_obj;     
	             	                 
    out[i].X11_sm_psf  = in[i].X11_sm_psf;
    out[i].X12_sm_psf  = in[i].X12_sm_psf;
    out[i].X22_sm_psf  = in[i].X22_sm_psf;
    out[i].E1_sm_psf   = in[i].E1_sm_psf;
    out[i].E2_sm_psf   = in[i].E2_sm_psf;     
	             	                 
    out[i].X11_sh_psf  = in[i].X11_sh_psf;
    out[i].X12_sh_psf  = in[i].X12_sh_psf;
    out[i].X22_sh_psf  = in[i].X22_sh_psf;
    out[i].E1_sh_psf   = in[i].E1_sh_psf;
    out[i].E2_sh_psf   = in[i].E2_sh_psf;

    out[i].F_ApR5      = in[i].F_ApR5;
    out[i].dF_ApR5     = in[i].dF_ApR5;
    out[i].sF_ApR5     = in[i].sF_ApR5;
    out[i].fF_ApR5     = in[i].fF_ApR5;
	               	            
    out[i].F_ApR6      = in[i].F_ApR6;
    out[i].dF_ApR6     = in[i].dF_ApR6;
    out[i].sF_ApR6     = in[i].sF_ApR6;
    out[i].fF_ApR6     = in[i].fF_ApR6;
	          	            
    out[i].F_ApR7      = in[i].F_ApR7;
    out[i].dF_ApR7     = in[i].dF_ApR7;
    out[i].sF_ApR7     = in[i].sF_ApR7;
    out[i].fF_ApR7     = in[i].fF_ApR7;
	          	            
    out[i].gamma       = in[i].gamma;
    out[i].E1          = in[i].E1;
    out[i].E2          = in[i].E2;

    out[i].objID       = in[i].objID;
    out[i].catID       = in[i].catID;

    out[i].photcode    = in[i].photcode;
    out[i].Nmeas       = in[i].Nmeas;
  }
  return (out);
}

StarPar *StarPar_PS1_V6_ToInternal (StarPar_PS1_V6 *in, off_t Nvalues) {

  off_t i;
  StarPar *out;

  ALLOCATE_ZERO (out, StarPar, Nvalues);

  for (i = 0; i < Nvalues; i++) {
    dvo_starpar_init (&out[i]);

    out[i].R  	     = in[i].R;      
    out[i].D  	     = in[i].D;      
    out[i].galLat    = in[i].galLat;      
    out[i].galLon    = in[i].galLon;      

    out[i].Ebv       = in[i].Ebv     ;      
    out[i].dEbv      = in[i].dEbv    ;      
    out[i].DistMag   = in[i].DistMag ;      
    out[i].dDistMag  = in[i].dDistMag;      
    out[i].M_r       = in[i].M_r     ;      
    out[i].dM_r      = in[i].dM_r    ;      
    out[i].FeH       = in[i].FeH     ;      
    out[i].dFeH      = in[i].dFeH    ;      
    out[i].uRA       = in[i].uRA     ;      
    out[i].uDEC      = in[i].uDEC    ;      

    out[i].averef  = in[i].averef;
    out[i].objID   = in[i].objID ;
    out[i].catID   = in[i].catID ;
  }
  return (out);
}

StarPar_PS1_V6 *StarParInternalTo_PS1_V6 (StarPar *in, off_t Nvalues) {

  off_t i;
  StarPar_PS1_V6 *out;

  ALLOCATE_ZERO (out, StarPar_PS1_V6, Nvalues);

  for (i = 0; i < Nvalues; i++) {

    out[i].R  	     = in[i].R;      
    out[i].D  	     = in[i].D;      
    out[i].galLat    = in[i].galLat;      
    out[i].galLon    = in[i].galLon;      

    out[i].Ebv       = in[i].Ebv     ;      
    out[i].dEbv      = in[i].dEbv    ;      
    out[i].DistMag   = in[i].DistMag ;      
    out[i].dDistMag  = in[i].dDistMag;      
    out[i].M_r       = in[i].M_r     ;      
    out[i].dM_r      = in[i].dM_r    ;      
    out[i].FeH       = in[i].FeH     ;      
    out[i].dFeH      = in[i].dFeH    ;      
    out[i].uRA       = in[i].uRA     ;      
    out[i].uDEC      = in[i].uDEC    ;      

    out[i].averef  = in[i].averef;
    out[i].objID   = in[i].objID ;
    out[i].catID   = in[i].catID ;
  }
  return (out);
}

GalPhot *GalPhot_PS1_V6_ToInternal (GalPhot_PS1_V6 *in, off_t Nvalues) {

  off_t i;
  GalPhot *out;

  ALLOCATE_ZERO (out, GalPhot, Nvalues);

  for (i = 0; i < Nvalues; i++) {
    dvo_galphot_init (&out[i]);

    out[i].Xfit       	 = in[i].Xfit;
    out[i].Yfit      	 = in[i].Yfit;
    out[i].mag       	 = in[i].mag;
    out[i].magErr    	 = in[i].magErr;
    out[i].majorAxis 	 = in[i].majorAxis;
    out[i].minorAxis 	 = in[i].minorAxis;
    out[i].majorAxisErr  = in[i].majorAxisErr;
    out[i].minorAxisErr  = in[i].minorAxisErr;
    out[i].theta         = in[i].theta;
    out[i].thetaErr      = in[i].thetaErr;
    out[i].index         = in[i].index;
    out[i].chisq	 = in[i].chisq;
    out[i].Npix 	 = in[i].Npix;
    out[i].objID	 = in[i].objID;
    out[i].catID         = in[i].catID;
    out[i].detID	 = in[i].detID;
    out[i].imageID	 = in[i].imageID;
    out[i].averef	 = in[i].averef;
    out[i].flags	 = in[i].flags;
    out[i].photcode	 = in[i].photcode;
    out[i].modelType	 = in[i].modelType;
  }
  return (out);
}

GalPhot_PS1_V6 *GalPhotInternalTo_PS1_V6 (GalPhot *in, off_t Nvalues) {

  off_t i;
  GalPhot_PS1_V6 *out;

  ALLOCATE_ZERO (out, GalPhot_PS1_V6, Nvalues);

  for (i = 0; i < Nvalues; i++) {
    out[i].Xfit       	 = in[i].Xfit;
    out[i].Yfit      	 = in[i].Yfit;
    out[i].mag       	 = in[i].mag;
    out[i].magErr    	 = in[i].magErr;
    out[i].majorAxis 	 = in[i].majorAxis;
    out[i].minorAxis 	 = in[i].minorAxis;
    out[i].majorAxisErr  = in[i].majorAxisErr;
    out[i].minorAxisErr  = in[i].minorAxisErr;
    out[i].theta         = in[i].theta;
    out[i].thetaErr      = in[i].thetaErr;
    out[i].index         = in[i].index;
    out[i].chisq	 = in[i].chisq;
    out[i].Npix 	 = in[i].Npix;
    out[i].objID	 = in[i].objID;
    out[i].catID         = in[i].catID;
    out[i].detID	 = in[i].detID;
    out[i].imageID	 = in[i].imageID;
    out[i].averef	 = in[i].averef;
    out[i].flags	 = in[i].flags;
    out[i].photcode	 = in[i].photcode;
    out[i].modelType	 = in[i].modelType;
  }
  return (out);
}

# define RAW_IMAGE_NAME_LEN 117

Image *Image_PS1_V6_ToInternal (Image_PS1_V6 *in, off_t Nvalues, off_t Nalloc) {

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

    // RAW_IMAGE_NAME_LEN == DVO_IMAGE_NAME_LEN
    strncpy_nowarn (out[i].name, in[i].name, RAW_IMAGE_NAME_LEN - 1);

    out[i].tzero    	    = in[i].tzero;
    out[i].nstar    	    = in[i].nstar;
    out[i].secz	    	    = in[i].secz;
    out[i].NX	    	    = in[i].NX;
    out[i].NY	    	    = in[i].NY;
    out[i].apmifit  	    = in[i].apmifit;
    out[i].dapmifit 	    = in[i].dapmifit;
    out[i].McalPSF   	    = in[i].McalPSF;
    out[i].McalAPER    	    = in[i].McalAPER;
    out[i].dMcal    	    = in[i].dMcal;
    out[i].McalChiSq   	    = in[i].McalChiSq;
    out[i].photcode   	    = in[i].photcode;
    out[i].exptime  	    = in[i].exptime;
    out[i].sidtime  	    = in[i].sidtime;
    out[i].latitude  	    = in[i].latitude;

    out[i].RAo  	    = in[i].RAo;
    out[i].DECo  	    = in[i].DECo;
    out[i].Radius  	    = in[i].Radius;
    out[i].refColorBlue	    = in[i].refColorBlue;
    out[i].refColorRed 	    = in[i].refColorRed;

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

Image_PS1_V6 *ImageInternalTo_PS1_V6 (Image *in, off_t Nvalues) {

  off_t i;
  Image_PS1_V6 *out;

  ALLOCATE_ZERO (out, Image_PS1_V6, Nvalues);

  for (i = 0; i < Nvalues; i++) {
    // this is only save because the initial 120 bytes in Coords match CoordsDisk
    memcpy (&out[i].coords, &in[i].coords, sizeof(CoordsDisk));

    // RAW_IMAGE_NAME_LEN == DVO_IMAGE_NAME_LEN
    strncpy_nowarn (out[i].name, in[i].name, RAW_IMAGE_NAME_LEN - 1);

    out[i].tzero    	    = in[i].tzero;
    out[i].nstar    	    = in[i].nstar;
    out[i].secz	    	    = in[i].secz;
    out[i].NX	    	    = in[i].NX;
    out[i].NY	    	    = in[i].NY;
    out[i].apmifit  	    = in[i].apmifit;
    out[i].dapmifit 	    = in[i].dapmifit;
    out[i].McalPSF   	    = in[i].McalPSF;
    out[i].McalAPER    	    = in[i].McalAPER;
    out[i].dMcal    	    = in[i].dMcal;
    out[i].McalChiSq   	    = in[i].McalChiSq;
    out[i].photcode   	    = in[i].photcode;
    out[i].exptime  	    = in[i].exptime;
    out[i].sidtime  	    = in[i].sidtime;
    out[i].latitude  	    = in[i].latitude;

    out[i].RAo  	    = in[i].RAo;
    out[i].DECo  	    = in[i].DECo;
    out[i].Radius  	    = in[i].Radius;
    out[i].refColorBlue	    = in[i].refColorBlue;
    out[i].refColorRed 	    = in[i].refColorRed;

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

PhotCode *PhotCode_PS1_V6_To_Internal (PhotCode_PS1_V6 *in, off_t Nvalues) {

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

PhotCode_PS1_V6 *PhotCode_Internal_To_PS1_V6 (PhotCode *in, off_t Nvalues) {

  off_t i;
  PhotCode_PS1_V6 *out;

  ALLOCATE_ZERO (out, PhotCode_PS1_V6, Nvalues);

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

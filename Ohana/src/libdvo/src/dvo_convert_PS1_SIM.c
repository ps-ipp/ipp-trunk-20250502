# include <dvo.h>

/* convert PS1_SIM formats to internal formats */

Measure *Measure_PS1_SIM_ToInternal (Average *ave, Measure_PS1_SIM *in, off_t Nvalues) {
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
    out[i].McalPSF    = in[i].Mcal;
    out[i].McalAPER   = in[i].Mcal;
    out[i].dt         = in[i].dt;
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
    out[i].Mflat      = in[i].Mflat;
    out[i].t          = in[i].t;
    out[i].averef     = in[i].averef;
    out[i].detID      = in[i].detID;
    out[i].objID      = in[i].objID;
    out[i].catID      = in[i].catID;
    out[i].imageID    = in[i].imageID;
    out[i].psfQF      = in[i].psfQF;
    out[i].psfQFperf  = in[i].psfQFperf;
    out[i].photcode   = in[i].photcode;
    out[i].dXccd      = in[i].dXccd;
    out[i].dYccd      = in[i].dYccd;
    out[i].dRsys      = in[i].dRsys;
    out[i].posangle   = in[i].posangle;
    out[i].pltscale   = in[i].pltscale;
    out[i].dbFlags    = in[i].dbFlags;
    out[i].photFlags  = in[i].photFlags;
  }
  return (out);
}

Measure_PS1_SIM *MeasureInternalTo_PS1_SIM (Average *ave, Measure *in, off_t Nvalues) {
  OHANA_UNUSED_PARAM(ave);

  off_t i;
  Measure_PS1_SIM *out;

  ALLOCATE_ZERO (out, Measure_PS1_SIM, Nvalues);

  for (i = 0; i < Nvalues; i++) {
    out[i].R          = in[i].R;
    out[i].D          = in[i].D;
    out[i].M          = in[i].M;
    out[i].dM         = in[i].dM;
    out[i].Mcal       = in[i].McalPSF;
    out[i].dt         = in[i].dt;
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
    out[i].Mflat      = in[i].Mflat;
    out[i].t          = in[i].t;
    out[i].averef     = in[i].averef;
    out[i].detID      = in[i].detID;
    out[i].objID      = in[i].objID;
    out[i].catID      = in[i].catID;
    out[i].imageID    = in[i].imageID;
    out[i].psfQF      = in[i].psfQF;
    out[i].psfQFperf  = in[i].psfQFperf;
    out[i].photcode   = in[i].photcode;
    out[i].dXccd      = in[i].dXccd;
    out[i].dYccd      = in[i].dYccd;
    out[i].dRsys      = in[i].dRsys;
    out[i].posangle   = in[i].posangle;
    out[i].pltscale   = in[i].pltscale;
    out[i].dbFlags    = in[i].dbFlags;
    out[i].photFlags  = in[i].photFlags;
  }
  return (out);
}

// 'primary' is needed to conform with the API for Loneos and Elixir, but is not used
Average *Average_PS1_SIM_ToInternal (Average_PS1_SIM *in, off_t Nvalues, SecFilt **primary) {
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

    out[i].ChiSqAve    	 = in[i].ChiSqAve;     
    out[i].ChiSqPM    	 = in[i].ChiSqPM;     
    out[i].ChiSqPar    	 = in[i].ChiSqPar;     
    out[i].Tmean    	 = in[i].Tmean;     
    out[i].Trange   	 = in[i].Trange;     

    out[i].Npos       	 = in[i].Npos;     

    out[i].Nmeasure      = in[i].Nmeasure;     
    out[i].Nstarpar      = in[i].Nstarpar;     

    out[i].measureOffset = in[i].measureOffset; 
    out[i].starparOffset = in[i].starparOffset;

    out[i].refColorBlue  = in[i].refColorBlue;
    out[i].refColorRed   = in[i].refColorRed;

    out[i].flags     	 = in[i].flags;   
    out[i].objID 	 = in[i].objID;
    out[i].catID 	 = in[i].catID;
  }
  return (out);
}

// 'primary' is needed to conform with the API for Loneos and Elixir, but is not used
Average_PS1_SIM *AverageInternalTo_PS1_SIM (Average *in, off_t Nvalues, SecFilt *primary) {
  OHANA_UNUSED_PARAM(primary);

  off_t i;
  Average_PS1_SIM *out;

  ALLOCATE_ZERO (out, Average_PS1_SIM, Nvalues);

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

    out[i].ChiSqAve    	 = in[i].ChiSqAve;     
    out[i].ChiSqPM    	 = in[i].ChiSqPM;     
    out[i].ChiSqPar    	 = in[i].ChiSqPar;     
    out[i].Tmean    	 = in[i].Tmean;     
    out[i].Trange   	 = in[i].Trange;     

    out[i].Npos       	 = in[i].Npos;     

    out[i].Nmeasure      = in[i].Nmeasure;     
    out[i].Nstarpar      = in[i].Nstarpar;     

    out[i].measureOffset = in[i].measureOffset; 
    out[i].starparOffset = in[i].starparOffset;

    out[i].refColorBlue  = in[i].refColorBlue;
    out[i].refColorRed   = in[i].refColorRed;

    out[i].flags     	 = in[i].flags;   
    out[i].objID 	 = in[i].objID;
    out[i].catID 	 = in[i].catID;
  }
  return (out);
}

SecFilt *SecFilt_PS1_SIM_ToInternal (SecFilt_PS1_SIM *in, off_t Nvalues) {

  off_t i;
  SecFilt *out;

  ALLOCATE_ZERO (out, SecFilt, Nvalues);

  for (i = 0; i < Nvalues; i++) {
    dvo_secfilt_init (&out[i], SECFILT_RESET_ALL);

    out[i].MpsfChp     = in[i].M;      
    out[i].dMpsfChp    = in[i].dM;      

    out[i].Ncode         = in[i].Ncode;
    out[i].Nused         = in[i].Nused;
    out[i].flags         = in[i].flags;     
  }
  return (out);
}

SecFilt_PS1_SIM *SecFiltInternalTo_PS1_SIM (SecFilt *in, off_t Nvalues) {

  off_t i;
  SecFilt_PS1_SIM *out;

  ALLOCATE_ZERO (out, SecFilt_PS1_SIM, Nvalues);

  for (i = 0; i < Nvalues; i++) {

    out[i].M             = in[i].MpsfChp;      
    out[i].dM            = in[i].dMpsfChp;      

    out[i].Ncode         = in[i].Ncode;
    out[i].Nused         = in[i].Nused;
    out[i].flags         = in[i].flags;     
  }
  return (out);
}

StarPar *StarPar_PS1_SIM_ToInternal (StarPar_PS1_SIM *in, off_t Nvalues) {

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

StarPar_PS1_SIM *StarParInternalTo_PS1_SIM (StarPar *in, off_t Nvalues) {

  off_t i;
  StarPar_PS1_SIM *out;

  ALLOCATE_ZERO (out, StarPar_PS1_SIM, Nvalues);

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

# define RAW_IMAGE_NAME_LEN 117

Image *Image_PS1_SIM_ToInternal (Image_PS1_SIM *in, off_t Nvalues, off_t Nalloc) {

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

Image_PS1_SIM *ImageInternalTo_PS1_SIM (Image *in, off_t Nvalues) {

  off_t i;
  Image_PS1_SIM *out;

  ALLOCATE_ZERO (out, Image_PS1_SIM, Nvalues);

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


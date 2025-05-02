# include <dvo.h>

/* convert elixir-format measures to internal measures */
Measure *Measure_Elixir_ToInternal (Average *ave, Measure_Elixir *in, off_t Nvalues) {

  off_t i;
  Measure *out;

  ALLOCATE_ZERO (out, Measure, Nvalues);

  for (i = 0; i < Nvalues; i++) {
    dvo_measure_init (&out[i]);
    out[i].FWx 	    = in[i].FWx;
    out[i].t        = in[i].t;
    out[i].averef   = in[i].averef;

    // changed for PS1_V5
    int averef = out[i].averef;
    out[i].R          = (in[i].dR == NAN_S_SHORT) ? NAN : ave[averef].R - 0.01 * in[i].dR / 3600.0;
    out[i].D          = (in[i].dD == NAN_S_SHORT) ? NAN : ave[averef].D - 0.01 * in[i].dD / 3600.0;

    // changed for PANSTARRS_DEV_0
    out[i].M        = (in[i].M       == NAN_S_SHORT) ? NAN : in[i].M      * 0.001;
    out[i].dM       = (in[i].dM      == NAN_U_CHAR)  ? NAN : in[i].dM     * 0.001;
    out[i].dt       = (in[i].dt      == NAN_S_SHORT) ? NAN : in[i].dt     * 0.001;
    out[i].McalPSF  = (in[i].Mcal    == NAN_S_SHORT) ? NAN : in[i].Mcal   * 0.001;
    out[i].McalAPER = (in[i].Mcal    == NAN_S_SHORT) ? NAN : in[i].Mcal   * 0.001;
    out[i].Map      = (in[i].Mgal    == NAN_S_SHORT) ? NAN : in[i].Mgal   * 0.001;
    out[i].airmass  = (in[i].airmass == NAN_S_SHORT) ? NAN : in[i].airmass* 0.001;
    out[i].FWy 	    = in[i].fwy * in[i].FWx * 0.01;
    out[i].theta    = in[i].theta*(0x10000 / 0x100);
    out[i].photcode = in[i].source;
    
    // added for PANSTARRS_DEV_0
    // out[i].Xccd      = 0;  // determine on-the-fly
    // out[i].Yccd      = 0;  // determine on-the-fly
    // out[i].detID     = 0;  // determine on-the-fly
    // out[i].imageID   = 0;  // determine on-the-fly

    // changed for PS1_DEV_1 (2008.02.26)
    out[i].dbFlags    = in[i].flags;

    // changed for PS1_DEV_2
    out[i].Map        = in[i].Mgal;

    // changed for PS1_V1
    out[i].photFlags  = in[i].dophot << 16;
  }
  return (out);
}

/* convert internal measures to elixir-format measures */
Measure_Elixir *MeasureInternalTo_Elixir (Average *ave, Measure *in, off_t Nvalues) {

  off_t i;
  Measure_Elixir *out;

  ALLOCATE_ZERO (out, Measure_Elixir, Nvalues);

  for (i = 0; i < Nvalues; i++) {
    out[i].FWx 	   = in[i].FWx;
    out[i].t       = in[i].t;
    out[i].averef  = in[i].averef;

    // changed for PS1_V5
    int averef = in[i].averef;
    int isBad = isnan(in[i].R) || isnan(in[i].D);
    out[i].dR     = isBad ? NAN_S_SHORT : 100.0*3600.0*(ave[averef].R - in[i].R);
    out[i].dD     = isBad ? NAN_S_SHORT : 100.0*3600.0*(ave[averef].D - in[i].D);

    // changed for PANSTARRS_DEV_0
    out[i].M        = isnan(in[i].M      ) ? NAN_S_SHORT : in[i].M       * 1000.0;
    out[i].dM       = isnan(in[i].dM     ) ? NAN_U_CHAR  : in[i].dM      * 1000.0;
    out[i].dt       = isnan(in[i].dt     ) ? NAN_S_SHORT : in[i].dt      * 1000.0;
    out[i].Mcal     = isnan(in[i].McalPSF) ? NAN_S_SHORT : in[i].McalPSF * 1000.0;
    out[i].Mgal     = isnan(in[i].Map    ) ? NAN_S_SHORT : in[i].Map     * 1000.0;
    out[i].airmass  = isnan(in[i].airmass) ? NAN_S_SHORT : in[i].airmass * 1000.0;
    out[i].fwy 	    = in[i].FWy * 100.0 / in[i].FWx;
    out[i].theta    = in[i].theta*(0x100/ 0x10000);
    out[i].source   = in[i].photcode;

    if (out[i].M < 0) {
	fprintf (stderr, ".");
    }

    // changed or added for PS1_DEV_1 (2008.02.26)
    out[i].flags   = in[i].dbFlags;

    // changed or added for PS1_V1
    out[i].dophot  = in[i].photFlags >> 16;
  }
  return (out);
}

/* convert elixir-format averages to internal averages */
Average *Average_Elixir_ToInternal (Average_Elixir *in, off_t Nvalues, SecFilt **primary) {

  off_t i;
  Average *out;

  ALLOCATE_ZERO (out, Average, Nvalues);
  ALLOCATE_ZERO (*primary, SecFilt, Nvalues);

  for (i = 0; i < Nvalues; i++) {
    dvo_average_init (&out[i]);
    dvo_secfilt_init (&primary[0][i], SECFILT_RESET_ALL);

    out[i].R       	 = in[i].R;      
    out[i].D       	 = in[i].D;      

    // changed for PANSTARRS_DEV_0 (moved from Average to Measure)
    primary[0][i].MpsfChp  = (in[i].M  == NAN_S_SHORT) ? NAN : in[i].M  * 0.001;      
    primary[0][i].dMpsfChp = (in[i].dM == NAN_S_SHORT) ? NAN : in[i].dM * 0.001;      
    primary[0][i].Mchisq= pow (10.0, 0.01*in[i].Xm);     

    // added for PANSTARRS_DEV_0
    // out[i].objID   = 0; // determine on-the-fly
    // out[i].catID   = 0; // determine on-the-fly

    // changed or added for PS1_DEV_2
    out[i].Nmeasure 	 = in[i].Nm;     
    out[i].Nmissing 	 = in[i].Nn;     
    out[i].measureOffset = in[i].offset; 
    out[i].missingOffset = in[i].missing;

    // changed or added for PS1_V1
    out[i].flags    	 = in[i].code;   
  }
  return (out);
}

/* convert internal averages to elixir-format averages */
Average_Elixir *AverageInternalTo_Elixir (Average *in, off_t Nvalues, SecFilt *primary) {

  off_t i;
  Average_Elixir *out;

  ALLOCATE_ZERO (out, Average_Elixir, Nvalues);

  for (i = 0; i < Nvalues; i++) {
    out[i].R       = in[i].R;      
    out[i].D       = in[i].D;      

    // changed for PANSTARRS_DEV_0 (moved from Average to Measure)
    out[i].M       = isnan(primary[i].MpsfChp)  ? NAN_S_SHORT : primary[i].MpsfChp   * 1000.0;
    out[i].dM      = isnan(primary[i].dMpsfChp) ? NAN_S_SHORT : primary[i].dMpsfChp  * 1000.0;
    out[i].Xm      = 100.0*log10(primary[i].Mchisq);     

    // changed or added for PS1_DEV_2
    out[i].Nm      = in[i].Nmeasure;     
    out[i].Nn      = in[i].Nmissing;     
    out[i].offset  = in[i].measureOffset; 
    out[i].missing = in[i].missingOffset;

    // changed or added for PS1_V1
    out[i].code    = in[i].flags;   
  }
  return (out);
}

/* convert elixir-format secfilts to internal secfilts */
SecFilt *SecFilt_Elixir_ToInternal (SecFilt_Elixir *in, off_t Nvalues) {

  off_t i;
  SecFilt *out;

  ALLOCATE_ZERO (out, SecFilt, Nvalues);

  for (i = 0; i < Nvalues; i++) {
    dvo_secfilt_init (&out[i], SECFILT_RESET_ALL);

    out[i].Mchisq = pow (10.0, 0.01*in[i].Xm);     

    // added or changed for PANSTARRS_DEV_0
    out[i].MpsfChp     = (in[i].M  == NAN_S_SHORT) ? NAN : in[i].M   * 0.001;
    out[i].dMpsfChp    = (in[i].dM == NAN_S_SHORT) ? NAN : in[i].dM  * 0.001;
  }
  return (out);
}

/* convert internal secfilts to elixir-format secfilts */
SecFilt_Elixir *SecFiltInternalTo_Elixir (SecFilt *in, off_t Nvalues) {

  off_t i;
  SecFilt_Elixir *out;

  ALLOCATE_ZERO (out, SecFilt_Elixir, Nvalues);

  for (i = 0; i < Nvalues; i++) {
    out[i].Xm    = 100.0*log10(in[i].Mchisq);     

    // added or changed for PANSTARRS_DEV_0
    out[i].M     = isnan(in[i].MpsfChp)  ? NAN_S_SHORT : in[i].MpsfChp   * 1000.0;
    out[i].dM    = isnan(in[i].dMpsfChp) ? NAN_S_SHORT : in[i].dMpsfChp  * 1000.0;
  }
  return (out);
}

# define RAW_IMAGE_NAME_LEN 32

/* convert elixir-format images to internal images */
Image *Image_Elixir_ToInternal (Image_Elixir *in, off_t Nvalues, off_t Nalloc) {

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
    out[i].NX	    	    = in[i].NX;
    out[i].NY	    	    = in[i].NY;
    out[i].photcode   	    = in[i].source;
    out[i].exptime  	    = in[i].exptime;
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

    // added or changed for PANSTARRS_DEV_0
    out[i].secz    	    = (in[i].secz     == NAN_S_SHORT) ? NAN : in[i].secz     * 0.001;
    out[i].apmifit  	    = (in[i].apmifit  == NAN_S_SHORT) ? NAN : in[i].apmifit  * 0.001;
    out[i].dapmifit  	    = (in[i].dapmifit == NAN_S_SHORT) ? NAN : in[i].dapmifit * 0.001;

    out[i].McalPSF   	    = (in[i].Mcal     == NAN_S_SHORT) ? NAN : in[i].Mcal     * 0.001;
    out[i].McalAPER   	    = (in[i].Mcal     == NAN_S_SHORT) ? NAN : in[i].Mcal     * 0.001;
    out[i].dMcal   	    = (in[i].dMcal    == NAN_S_SHORT) ? NAN : in[i].dMcal    * 0.001;
    out[i].McalChiSq  	    = (in[i].dMcal    == NAN_S_SHORT) ? NAN : pow(10.0, 0.01*in[i].Xm);

    out[i].sidtime  	    = NAN;
    out[i].latitude  	    = NAN;
    out[i].imageID          = 0; // determine on-the-fly

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

/* convert internal images to elixir-format images */
Image_Elixir *ImageInternalTo_Elixir (Image *in, off_t Nvalues) {

  off_t i;
  Image_Elixir *out;

  ALLOCATE_ZERO (out, Image_Elixir, Nvalues);

  for (i = 0; i < Nvalues; i++) {
    // this is only safe because the initial 120 bytes in Coords match CoordsDisk
    memcpy (&out[i].coords, &in[i].coords, sizeof(CoordsDisk));

    // RAW_IMAGE_NAME_LEN < DVO_IMAGE_NAME_LEN
    strncpy_nowarn (out[i].name, in[i].name, RAW_IMAGE_NAME_LEN - 1);

    out[i].tzero    	    = in[i].tzero;
    out[i].nstar    	    = in[i].nstar;
    out[i].NX	    	    = in[i].NX;
    out[i].NY	    	    = in[i].NY;

    out[i].source   	    = in[i].photcode;
    out[i].exptime  	    = in[i].exptime;
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

    // added or changed for PANSTARRS_DEV_0
    out[i].secz    	    = isnan(in[i].secz    ) ? NAN_S_SHORT : in[i].secz     * 1000.0;
    out[i].apmifit    	    = isnan(in[i].apmifit ) ? NAN_S_SHORT : in[i].apmifit  * 1000.0;
    out[i].dapmifit    	    = isnan(in[i].dapmifit) ? NAN_S_SHORT : in[i].dapmifit * 1000.0;

    out[i].Mcal    	    = isnan(in[i].McalPSF ) ? NAN_S_SHORT : in[i].McalPSF  * 1000.0;
    out[i].dMcal    	    = isnan(in[i].dMcal   ) ? NAN_S_SHORT : in[i].dMcal    * 1000.0;
    out[i].Xm    	    = isnan(in[i].dMcal   ) ? NAN_S_SHORT : 100.0*log10(in[i].McalChiSq);

    // changed or added for PS1_V1
    out[i].code		    = in[i].flags;
  }
  return (out);
}

PhotCode *PhotCode_Elixir_To_Internal (PhotCode_Elixir *in, off_t Nvalues) {

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

    // changed or added for PS1_DEV_1
    out[i].astromErrMagScale = 0.0;
    out[i].photomErrSys      = 0.0;

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

PhotCode_Elixir *PhotCode_Internal_To_Elixir (PhotCode *in, off_t Nvalues) {

  off_t i;
  PhotCode_Elixir *out;

  ALLOCATE_ZERO (out, PhotCode_Elixir, Nvalues);

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
  }
  return (out);
}

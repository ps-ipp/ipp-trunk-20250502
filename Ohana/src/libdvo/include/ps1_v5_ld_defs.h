Image 		       	*Image_PS1_V5_LOAD_ToInternal (Image_PS1_V5_LOAD *in, off_t Nvalues, off_t Nalloc);
Image_PS1_V5_LOAD    	*ImageInternalTo_PS1_V5_LOAD (Image *in, off_t Nvalues);

Average 	       	*Average_PS1_V5_LOAD_ToInternal (Average_PS1_V5_LOAD *in, off_t Nvalues, SecFilt **primary);
Average_PS1_V5_LOAD     *AverageInternalTo_PS1_V5_LOAD (Average *in, off_t Nvalues, SecFilt *primary);

Measure 	       	*Measure_PS1_V5_LOAD_ToInternal (Average *ave, Measure_PS1_V5_LOAD *in, off_t Nvalues);
Measure_PS1_V5_LOAD     *MeasureInternalTo_PS1_V5_LOAD (Average *ave, Measure *in, off_t Nvalues);

SecFilt 	       	*SecFilt_PS1_V5_LOAD_ToInternal (SecFilt_PS1_V5_LOAD *in, off_t Nvalues);
SecFilt_PS1_V5_LOAD     *SecFiltInternalTo_PS1_V5_LOAD (SecFilt *in, off_t Nvalues);

Lensobj 	       	*Lensobj_PS1_V5_LOAD_ToInternal (Lensobj_PS1_V5_LOAD *in, off_t Nvalues);
Lensobj_PS1_V5_LOAD     *LensobjInternalTo_PS1_V5_LOAD (Lensobj *in, off_t Nvalues);

Lensing 	       	*Lensing_PS1_V5_LOAD_ToInternal (Lensing_PS1_V5_LOAD *in, off_t Nvalues);
Lensing_PS1_V5_LOAD     *LensingInternalTo_PS1_V5_LOAD (Lensing *in, off_t Nvalues);

StarPar 	       	*StarPar_PS1_V5_LOAD_ToInternal (StarPar_PS1_V5_LOAD *in, off_t Nvalues);
StarPar_PS1_V5_LOAD     *StarParInternalTo_PS1_V5_LOAD (StarPar *in, off_t Nvalues);

GalPhot 	       	*GalPhot_PS1_V5_LOAD_ToInternal (GalPhot_PS1_V5_LOAD *in, off_t Nvalues);
GalPhot_PS1_V5_LOAD     *GalPhotInternalTo_PS1_V5_LOAD (GalPhot *in, off_t Nvalues);

PhotCode                *PhotCode_PS1_V5_LOAD_To_Internal (PhotCode_PS1_V5_LOAD *in, off_t Nvalues);
PhotCode_PS1_V5_LOAD    *PhotCode_Internal_To_PS1_V5_LOAD (PhotCode *in, off_t Nvalues);

Image 		       	*Image_PS1_V6_ToInternal (Image_PS1_V6 *in, off_t Nvalues, off_t Nalloc);
Image_PS1_V6    	*ImageInternalTo_PS1_V6 (Image *in, off_t Nvalues);

Average 	       	*Average_PS1_V6_ToInternal (Average_PS1_V6 *in, off_t Nvalues, SecFilt **primary);
Average_PS1_V6          *AverageInternalTo_PS1_V6 (Average *in, off_t Nvalues, SecFilt *primary);

Measure 	       	*Measure_PS1_V6_ToInternal (Average *ave, Measure_PS1_V6 *in, off_t Nvalues);
Measure_PS1_V6          *MeasureInternalTo_PS1_V6 (Average *ave, Measure *in, off_t Nvalues);

SecFilt 	       	*SecFilt_PS1_V6_ToInternal (SecFilt_PS1_V6 *in, off_t Nvalues);
SecFilt_PS1_V6          *SecFiltInternalTo_PS1_V6 (SecFilt *in, off_t Nvalues);

Lensobj 	       	*Lensobj_PS1_V6_ToInternal (Lensobj_PS1_V6 *in, off_t Nvalues);
Lensobj_PS1_V6          *LensobjInternalTo_PS1_V6 (Lensobj *in, off_t Nvalues);

Lensing 	       	*Lensing_PS1_V6_ToInternal (Lensing_PS1_V6 *in, off_t Nvalues);
Lensing_PS1_V6          *LensingInternalTo_PS1_V6 (Lensing *in, off_t Nvalues);

StarPar 	       	*StarPar_PS1_V6_ToInternal (StarPar_PS1_V6 *in, off_t Nvalues);
StarPar_PS1_V6          *StarParInternalTo_PS1_V6 (StarPar *in, off_t Nvalues);

GalPhot 	       	*GalPhot_PS1_V6_ToInternal (GalPhot_PS1_V6 *in, off_t Nvalues);
GalPhot_PS1_V6          *GalPhotInternalTo_PS1_V6 (GalPhot *in, off_t Nvalues);

PhotCode                *PhotCode_PS1_V6_To_Internal (PhotCode_PS1_V6 *in, off_t Nvalues);
PhotCode_PS1_V6         *PhotCode_Internal_To_PS1_V6 (PhotCode *in, off_t Nvalues);

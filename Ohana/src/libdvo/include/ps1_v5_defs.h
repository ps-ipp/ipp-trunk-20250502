Image 		       	*Image_PS1_V5_ToInternal (Image_PS1_V5 *in, off_t Nvalues, off_t Nalloc);
Image_PS1_V5    	*ImageInternalTo_PS1_V5 (Image *in, off_t Nvalues);
Average 	       	*Average_PS1_V5_ToInternal (Average_PS1_V5 *in, off_t Nvalues, SecFilt **primary);
Average_PS1_V5          *AverageInternalTo_PS1_V5 (Average *in, off_t Nvalues, SecFilt *primary);
Measure 	       	*Measure_PS1_V5_ToInternal (Average *ave, Measure_PS1_V5 *in, off_t Nvalues);
Measure_PS1_V5          *MeasureInternalTo_PS1_V5 (Average *ave, Measure *in, off_t Nvalues);
SecFilt 	       	*SecFilt_PS1_V5_ToInternal (SecFilt_PS1_V5 *in, off_t Nvalues);
SecFilt_PS1_V5          *SecFiltInternalTo_PS1_V5 (SecFilt *in, off_t Nvalues);

Lensobj 	       	*Lensobj_PS1_V5_R0_ToInternal (Lensobj_PS1_V5_R0 *in, off_t Nvalues);
Lensobj 	       	*Lensobj_PS1_V5_R1_ToInternal (Lensobj_PS1_V5_R1 *in, off_t Nvalues);

Lensobj_PS1_V5_R1       *LensobjInternalTo_PS1_V5_R1 (Lensobj *in, off_t Nvalues);

Lensing 	       	*Lensing_PS1_V5_R0_ToInternal (Lensing_PS1_V5_R0 *in, off_t Nvalues);
Lensing 	       	*Lensing_PS1_V5_R1_ToInternal (Lensing_PS1_V5_R1 *in, off_t Nvalues);
Lensing 	       	*Lensing_PS1_V5_R2_ToInternal (Lensing_PS1_V5_R2 *in, off_t Nvalues);
Lensing 	       	*Lensing_PS1_V5_R3_ToInternal (Lensing_PS1_V5_R3 *in, off_t Nvalues);

Lensing_PS1_V5_R3       *LensingInternalTo_PS1_V5_R3 (Lensing *in, off_t Nvalues);

StarPar 	       	*StarPar_PS1_V5_ToInternal (StarPar_PS1_V5 *in, off_t Nvalues);
StarPar_PS1_V5          *StarParInternalTo_PS1_V5 (StarPar *in, off_t Nvalues);

GalPhot 	       	*GalPhot_PS1_V5_R1_ToInternal (GalPhot_PS1_V5_R1 *in, off_t Nvalues);
GalPhot                 *GalPhot_PS1_V5_R0_ToInternal (GalPhot_PS1_V5_R0 *in, off_t Nvalues);

GalPhot_PS1_V5_R1       *GalPhotInternalTo_PS1_V5_R1 (GalPhot *in, off_t Nvalues);

PhotCode                *PhotCode_PS1_V5_To_Internal (PhotCode_PS1_V5 *in, off_t Nvalues);
PhotCode_PS1_V5         *PhotCode_Internal_To_PS1_V5 (PhotCode *in, off_t Nvalues);

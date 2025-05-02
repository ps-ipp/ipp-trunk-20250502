Image 		       	*Image_PS1_REF_V3_ToInternal (Image_PS1_REF_V3 *in, off_t Nvalues, off_t Nalloc);
Image_PS1_REF_V3    	*ImageInternalTo_PS1_REF_V3 (Image *in, off_t Nvalues);
Average 	       	*Average_PS1_REF_V3_ToInternal (Average_PS1_REF_V3 *in, off_t Nvalues, SecFilt **primary);
Average_PS1_REF_V3      *AverageInternalTo_PS1_REF_V3 (Average *in, off_t Nvalues, SecFilt *primary);
Measure 	       	*Measure_PS1_REF_V3_ToInternal (Average *ave, Measure_PS1_REF_V3 *in, off_t Nvalues);
Measure_PS1_REF_V3      *MeasureInternalTo_PS1_REF_V3 (Average *ave, Measure *in, off_t Nvalues);
SecFilt 	       	*SecFilt_PS1_REF_V3_ToInternal (SecFilt_PS1_REF_V3 *in, off_t Nvalues);
SecFilt_PS1_REF_V3      *SecFiltInternalTo_PS1_REF_V3 (SecFilt *in, off_t Nvalues);

PhotCode                *PhotCode_PS1_REF_V3_To_Internal (PhotCode_PS1_REF_V3 *in, off_t Nvalues);
PhotCode_PS1_REF_V3     *PhotCode_Internal_To_PS1_REF_V3 (PhotCode *in, off_t Nvalues);

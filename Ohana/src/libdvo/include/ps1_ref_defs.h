Image 		       	*Image_PS1_REF_ToInternal (Image_PS1_REF *in, off_t Nvalues, off_t Nalloc);
Image_PS1_REF    	*ImageInternalTo_PS1_REF (Image *in, off_t Nvalues);
Average 	       	*Average_PS1_REF_ToInternal (Average_PS1_REF *in, off_t Nvalues, SecFilt **primary);
Average_PS1_REF          *AverageInternalTo_PS1_REF (Average *in, off_t Nvalues, SecFilt *primary);
Measure 	       	*Measure_PS1_REF_ToInternal (Average *ave, Measure_PS1_REF *in, off_t Nvalues);
Measure_PS1_REF          *MeasureInternalTo_PS1_REF (Average *ave, Measure *in, off_t Nvalues);
SecFilt 	       	*SecFilt_PS1_REF_ToInternal (SecFilt_PS1_REF *in, off_t Nvalues);
SecFilt_PS1_REF          *SecFiltInternalTo_PS1_REF (SecFilt *in, off_t Nvalues);

PhotCode                *PhotCode_PS1_REF_To_Internal (PhotCode_PS1_REF *in, off_t Nvalues);
PhotCode_PS1_REF         *PhotCode_Internal_To_PS1_REF (PhotCode *in, off_t Nvalues);

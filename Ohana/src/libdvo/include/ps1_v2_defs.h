Image 		       	*Image_PS1_V2_ToInternal (Image_PS1_V2 *in, off_t Nvalues, off_t Nalloc);
Image_PS1_V2    	*ImageInternalTo_PS1_V2 (Image *in, off_t Nvalues);
Average 	       	*Average_PS1_V2_ToInternal (Average_PS1_V2 *in, off_t Nvalues, SecFilt **primary);
Average_PS1_V2          *AverageInternalTo_PS1_V2 (Average *in, off_t Nvalues, SecFilt *primary);
Measure 	       	*Measure_PS1_V2_ToInternal (Average *ave, Measure_PS1_V2 *in, off_t Nvalues);
Measure_PS1_V2          *MeasureInternalTo_PS1_V2 (Average *ave, Measure *in, off_t Nvalues);
SecFilt 	       	*SecFilt_PS1_V2_ToInternal (SecFilt_PS1_V2 *in, off_t Nvalues);
SecFilt_PS1_V2          *SecFiltInternalTo_PS1_V2 (SecFilt *in, off_t Nvalues);

PhotCode                *PhotCode_PS1_V2_To_Internal (PhotCode_PS1_V2 *in, off_t Nvalues);
PhotCode_PS1_V2         *PhotCode_Internal_To_PS1_V2 (PhotCode *in, off_t Nvalues);

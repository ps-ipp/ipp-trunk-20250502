Image 		       	*Image_PS1_V4_ToInternal (Image_PS1_V4 *in, off_t Nvalues, off_t Nalloc);
Image_PS1_V4    	*ImageInternalTo_PS1_V4 (Image *in, off_t Nvalues);
Average 	       	*Average_PS1_V4_ToInternal (Average_PS1_V4 *in, off_t Nvalues, SecFilt **primary);
Average_PS1_V4          *AverageInternalTo_PS1_V4 (Average *in, off_t Nvalues, SecFilt *primary);
Measure 	       	*Measure_PS1_V4_ToInternal (Average *ave, Measure_PS1_V4 *in, off_t Nvalues);
Measure_PS1_V4          *MeasureInternalTo_PS1_V4 (Average *ave, Measure *in, off_t Nvalues);
SecFilt 	       	*SecFilt_PS1_V4_ToInternal (SecFilt_PS1_V4 *in, off_t Nvalues);
SecFilt_PS1_V4          *SecFiltInternalTo_PS1_V4 (SecFilt *in, off_t Nvalues);

PhotCode                *PhotCode_PS1_V4_To_Internal (PhotCode_PS1_V4 *in, off_t Nvalues);
PhotCode_PS1_V4         *PhotCode_Internal_To_PS1_V4 (PhotCode *in, off_t Nvalues);

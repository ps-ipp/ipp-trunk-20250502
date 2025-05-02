Image 		       	*Image_PS1_DEV_1_ToInternal (Image_PS1_DEV_1 *in, off_t Nvalues, off_t Nalloc);
Image_PS1_DEV_1  	*ImageInternalTo_PS1_DEV_1 (Image *in, off_t Nvalues);
Average 	       	*Average_PS1_DEV_1_ToInternal (Average_PS1_DEV_1 *in, off_t Nvalues, SecFilt **primary);
Average_PS1_DEV_1       *AverageInternalTo_PS1_DEV_1 (Average *in, off_t Nvalues, SecFilt *primary);
Measure 	       	*Measure_PS1_DEV_1_ToInternal (Average *ave, Measure_PS1_DEV_1 *in, off_t Nvalues);
Measure_PS1_DEV_1       *MeasureInternalTo_PS1_DEV_1 (Average *ave, Measure *in, off_t Nvalues);
SecFilt 	       	*SecFilt_PS1_DEV_1_ToInternal (SecFilt_PS1_DEV_1 *in, off_t Nvalues);
SecFilt_PS1_DEV_1       *SecFiltInternalTo_PS1_DEV_1 (SecFilt *in, off_t Nvalues);

PhotCode *PhotCode_PS1_DEV_1_To_Internal (PhotCode_PS1_DEV_1 *in, off_t Nvalues);
PhotCode_PS1_DEV_1 *PhotCode_Internal_To_PS1_DEV_1 (PhotCode *in, off_t Nvalues);

Image 		       	*Image_Loneos_ToInternal (Image_Loneos *in, off_t Nvalues, off_t Nalloc);
Image_Loneos 	       	*ImageInternalTo_Loneos (Image *in, off_t Nvalues);
Average                	*Average_Loneos_ToInternal (Average_Loneos *in, off_t Nvalues, SecFilt **primary);
Average_Loneos 	        *AverageInternalTo_Loneos (Average *in, off_t Nvalues, SecFilt *primary);
Measure                	*Measure_Loneos_ToInternal (Average *ave, Measure_Loneos *in, off_t Nvalues);
Measure_Loneos 	        *MeasureInternalTo_Loneos (Average *ave, Measure *in, off_t Nvalues);
SecFilt 	       	*SecFilt_Loneos_ToInternal (SecFilt_Loneos *in, off_t Nvalues);
SecFilt_Loneos 	       	*SecFiltInternalTo_Loneos (SecFilt *in, off_t Nvalues);

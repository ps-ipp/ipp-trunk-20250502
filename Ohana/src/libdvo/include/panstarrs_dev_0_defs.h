Image 		       	*Image_Panstarrs_DEV_0_ToInternal (Image_Panstarrs_DEV_0 *in, off_t Nvalues, off_t Nalloc);
Image_Panstarrs_DEV_0  	*ImageInternalTo_Panstarrs_DEV_0 (Image *in, off_t Nvalues);
Average 	       	*Average_Panstarrs_DEV_0_ToInternal (Average_Panstarrs_DEV_0 *in, off_t Nvalues, SecFilt **primary);
Average_Panstarrs_DEV_0 *AverageInternalTo_Panstarrs_DEV_0 (Average *in, off_t Nvalues, SecFilt *primary);
Measure 	       	*Measure_Panstarrs_DEV_0_ToInternal (Average *ave, Measure_Panstarrs_DEV_0 *in, off_t Nvalues);
Measure_Panstarrs_DEV_0 *MeasureInternalTo_Panstarrs_DEV_0 (Average *ave, Measure *in, off_t Nvalues);
SecFilt 	       	*SecFilt_Panstarrs_DEV_0_ToInternal (SecFilt_Panstarrs_DEV_0 *in, off_t Nvalues);
SecFilt_Panstarrs_DEV_0 *SecFiltInternalTo_Panstarrs_DEV_0 (SecFilt *in, off_t Nvalues);

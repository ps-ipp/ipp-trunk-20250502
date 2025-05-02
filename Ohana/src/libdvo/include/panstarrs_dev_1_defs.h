Image 		       	*Image_Panstarrs_DEV_1_ToInternal (Image_Panstarrs_DEV_1 *in, off_t Nvalues, off_t Nalloc);
Image_Panstarrs_DEV_1  	*ImageInternalTo_Panstarrs_DEV_1 (Image *in, off_t Nvalues);
Average 	       	*Average_Panstarrs_DEV_1_ToInternal (Average_Panstarrs_DEV_1 *in, off_t Nvalues, SecFilt **primary);
Average_Panstarrs_DEV_1 *AverageInternalTo_Panstarrs_DEV_1 (Average *in, off_t Nvalues, SecFilt *primary);
Measure 	       	*Measure_Panstarrs_DEV_1_ToInternal (Average *ave, Measure_Panstarrs_DEV_1 *in, off_t Nvalues);
Measure_Panstarrs_DEV_1 *MeasureInternalTo_Panstarrs_DEV_1 (Average *ave, Measure *in, off_t Nvalues);
SecFilt 	       	*SecFilt_Panstarrs_DEV_1_ToInternal (SecFilt_Panstarrs_DEV_1 *in, off_t Nvalues);
SecFilt_Panstarrs_DEV_1 *SecFiltInternalTo_Panstarrs_DEV_1 (SecFilt *in, off_t Nvalues);

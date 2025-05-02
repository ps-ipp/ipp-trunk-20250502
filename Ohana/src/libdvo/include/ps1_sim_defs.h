Image 		       	*Image_PS1_SIM_ToInternal (Image_PS1_SIM *in, off_t Nvalues, off_t Nalloc);
Image_PS1_SIM    	*ImageInternalTo_PS1_SIM (Image *in, off_t Nvalues);

Average 	       	*Average_PS1_SIM_ToInternal (Average_PS1_SIM *in, off_t Nvalues, SecFilt **primary);
Average_PS1_SIM         *AverageInternalTo_PS1_SIM (Average *in, off_t Nvalues, SecFilt *primary);

Measure 	       	*Measure_PS1_SIM_ToInternal (Average *ave, Measure_PS1_SIM *in, off_t Nvalues);
Measure_PS1_SIM         *MeasureInternalTo_PS1_SIM (Average *ave, Measure *in, off_t Nvalues);

SecFilt 	       	*SecFilt_PS1_SIM_ToInternal (SecFilt_PS1_SIM *in, off_t Nvalues);
SecFilt_PS1_SIM         *SecFiltInternalTo_PS1_SIM (SecFilt *in, off_t Nvalues);

StarPar 	       	*StarPar_PS1_SIM_ToInternal (StarPar_PS1_SIM *in, off_t Nvalues);
StarPar_PS1_SIM         *StarParInternalTo_PS1_SIM (StarPar *in, off_t Nvalues);

Image 		       	*Image_Elixir_ToInternal (Image_Elixir *in, off_t Nvalues, off_t Nalloc);
Image_Elixir 	       	*ImageInternalTo_Elixir (Image *in, off_t Nvalues);
Average 	       	*Average_Elixir_ToInternal (Average_Elixir *in, off_t Nvalues, SecFilt **primary);
Average_Elixir 	        *AverageInternalTo_Elixir (Average *in, off_t Nvalues, SecFilt *primary);
Measure 	       	*Measure_Elixir_ToInternal (Average *ave, Measure_Elixir *in, off_t Nvalues);
Measure_Elixir 	        *MeasureInternalTo_Elixir (Average *ave, Measure *in, off_t Nvalues);
SecFilt 	       	*SecFilt_Elixir_ToInternal (SecFilt_Elixir *in, off_t Nvalues);
SecFilt_Elixir 	       	*SecFiltInternalTo_Elixir (SecFilt *in, off_t Nvalues);

PhotCode *PhotCode_Elixir_To_Internal (PhotCode_Elixir *in, off_t Nvalues);
PhotCode_Elixir *PhotCode_Internal_To_Elixir (PhotCode *in, off_t Nvalues);

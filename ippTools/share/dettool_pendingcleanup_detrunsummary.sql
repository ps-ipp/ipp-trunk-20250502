SELECT DISTINCT detRunSummary.*
FROM detRunSummary
     JOIN detProcessedImfile USING (det_id) 
     JOIN detProcessedExp USING (det_id) 
     JOIN detStackedImfile USING (det_id,iteration)
     JOIN detResidExp USING (det_id,iteration) 
     JOIN detResidImfile USING (det_id,iteration) 
     JOIN detNormalizedStatImfile USING (det_id,iteration) 
     JOIN detNormalizedImfile USING (det_id,iteration) 
     JOIN detNormalizedExp USING (det_id,iteration) 
WHERE (!(
       (
        detRunSummary.data_state = 'full' OR
	detRunSummary.data_state = 'cleaned'
       ) OR
       (
        !((detProcessedImfile.data_state = 'cleaned' OR
           detProcessedImfile.data_state = 'scrubbed' OR
           detProcessedImfile.data_state = 'purged') AND
          (detProcessedExp.data_state = 'cleaned' OR
           detProcessedExp.data_state = 'scrubbed' OR
           detProcessedExp.data_state = 'purged') AND
          (detStackedImfile.data_state = 'cleaned' OR
           detStackedImfile.data_state = 'scrubbed' OR
           detStackedImfile.data_state = 'purged') AND
          (detResidExp.data_state = 'cleaned' OR
           detResidExp.data_state = 'scrubbed' OR
           detResidExp.data_state = 'purged') AND
          (detResidImfile.data_state = 'cleaned' OR
           detResidImfile.data_state = 'scrubbed' OR
           detResidImfile.data_state = 'purged') AND
          (detNormalizedStatImfile.data_state = 'cleaned' OR
           detNormalizedStatImfile.data_state = 'scrubbed' OR
           detNormalizedStatImfile.data_state = 'purged') AND
          (detNormalizedImfile.data_state = 'cleaned' OR
           detNormalizedImfile.data_state = 'scrubbed' OR
           detNormalizedImfile.data_state = 'purged') AND
          (detNormalizedExp.data_state = 'cleaned' OR
           detNormalizedExp.data_state = 'scrubbed' OR
           detNormalizedExp.data_state = 'purged')
	 )
       )))

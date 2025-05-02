SELECT
    detStackedImfile.*,
    detRunSummary.data_state
FROM detRunSummary
JOIN detStackedImfile
     USING(det_id,iteration)
WHERE
     (detStackedImfile.data_state = 'cleaned'
      OR detStackedImfile.data_state = 'scrubbed'
      OR detStackedImfile.data_state = 'purged')

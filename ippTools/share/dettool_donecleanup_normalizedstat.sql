SELECT
    detNormalizedStatImfile.*,
    detRunSummary.data_state
FROM detRunSummary
JOIN detNormalizedStatImfile
    USING(det_id,iteration)
WHERE
    (detNormalizedStatImfile.data_state = 'cleaned'
     OR detNormalizedStatImfile.data_state = 'scrubbed'
     OR detNormalizedStatImfile.data_state = 'purged')


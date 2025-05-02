SELECT
    detNormalizedImfile.*,
    detRunSummary.data_state
FROM detRunSummary
JOIN detNormalizedImfile
    USING(det_id,iteration)
WHERE
    (detNormalizedImfile.data_state = 'cleaned'
     OR detNormalizedImfile.data_state = 'scrubbed'
     OR detNormalizedImfile.data_state = 'purged')


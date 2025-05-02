SELECT
    detNormalizedExp.*,
    detRunSummary.data_state
FROM detRunSummary
JOIN detNormalizedExp
    USING(det_id,iteration)
WHERE
    (detNormalizedExp.data_state = 'cleaned'
     OR detNormalizedExp.data_state = 'scrubbed'
     OR detNormalizedExp.data_state = 'purged')


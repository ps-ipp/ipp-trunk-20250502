SELECT
    detResidExp.*,
    detRunSummary.data_state
FROM detRunSummary
JOIN detResidExp
    USING(det_id,iteration)
WHERE
    (detResidExp.data_state = 'cleaned'
     OR detResidExp.data_state = 'scrubbed'
     OR detResidExp.data_state = 'purged')
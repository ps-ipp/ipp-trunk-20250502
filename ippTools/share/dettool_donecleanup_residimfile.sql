SELECT
    detResidImfile.*,
    detRunSummary.data_state
FROM detRunSummary
JOIN detResidImfile
    USING(det_id,iteration)
WHERE
    (detResidImfile.data_state = 'cleaned'
     OR detResidImfile.data_state = 'scrubbed'
     OR detResidImfile.data_state = 'purged')



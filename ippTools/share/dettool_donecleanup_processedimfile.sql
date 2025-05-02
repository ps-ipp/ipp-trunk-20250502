-- need to restrict to a single detRunSummary (require all to say 'cleaned'?)
SELECT
    detProcessedImfile.*,
    detRunSummary.data_state
FROM detRunSummary
JOIN detProcessedImfile
    USING(det_id)
WHERE
    (detProcessedImfile.data_state = 'cleaned'
     OR detProcessedImfile.data_state = 'scrubbed'
     OR detProcessedImfile.data_state = 'purged')

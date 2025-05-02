-- need to restrict to a single detRunSummary (require all to say 'cleaned'?)
SELECT
    detProcessedExp.*,
    detRunSummary.data_state
FROM detRunSummary
JOIN detProcessedExp
    USING(det_id)
WHERE
    (detProcessedExp.data_state = 'cleaned'
     OR detProcessedExp.data_state = 'scrubbed'
     OR detProcessedExp.data_state = 'purged')

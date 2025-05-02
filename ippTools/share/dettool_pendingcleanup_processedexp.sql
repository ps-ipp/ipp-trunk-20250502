-- need to restrict to a single detRunSummary (require all to say 'cleaned'?)
SELECT DISTINCT 
    detProcessedExp.*,
    rawExp.camera
FROM detProcessedExp
JOIN detInputExp
     USING(det_id,exp_id)
JOIN rawExp
     USING(exp_id)
WHERE
    (detProcessedExp.data_state = 'goto_cleaned'
     OR detProcessedExp.data_state = 'goto_scrubbed'
     OR detProcessedExp.data_state = 'goto_purged')

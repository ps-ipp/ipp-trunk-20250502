SELECT DISTINCT
    detProcessedImfile.*,
    rawExp.camera
    FROM detProcessedImfile
JOIN detInputExp
     USING(det_id,exp_id)
JOIN rawExp
     USING(exp_id)
WHERE
     (detProcessedImfile.data_state = 'goto_cleaned'
     OR detProcessedImfile.data_state = 'goto_scrubbed'
     OR detProcessedImfile.data_state = 'goto_purged')


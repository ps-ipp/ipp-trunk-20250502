SELECT DISTINCT
    detNormalizedImfile.*,
    rawExp.camera
FROM detNormalizedImfile
JOIN detInputExp
     USING(det_id,exp_id)
JOIN rawExp
     USING(exp_id)
WHERE
    (detNormalizedImfile.data_state = 'goto_cleaned'
     OR detNormalizedImfile.data_state = 'goto_scrubbed'
     OR detNormalizedImfile.data_state = 'goto_purged')


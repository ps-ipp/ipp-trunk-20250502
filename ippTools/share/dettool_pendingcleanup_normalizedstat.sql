SELECT DISTINCT
    detNormalizedStatImfile.*,
    rawExp.camera
FROM detNormalizedStatImfile
JOIN detInputExp
     USING(det_id,exp_id)
JOIN rawExp
     USING(exp_id)
WHERE
    (detNormalizedStatImfile.data_state = 'goto_cleaned'
     OR detNormalizedStatImfile.data_state = 'goto_scrubbed'
     OR detNormalizedStatImfile.data_state = 'goto_purged')


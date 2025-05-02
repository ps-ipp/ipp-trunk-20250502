ySELECT DISTINCT
    detStackedImfile.*,
    rawExp.camera
FROM detStackedImfile
JOIN detInputExp
     USING(det_id,exp_id)
JOIN rawExp
     USING(exp_id)
WHERE
    (detStackedImfile.data_state = 'goto_cleaned'
     OR detStackedImfile.data_state = 'goto_scrubbed'
     OR detStackedImfile.data_state = 'goto_purged')

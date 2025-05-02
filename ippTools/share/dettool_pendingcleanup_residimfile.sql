SELECT DISTINCT
    detResidImfile.*,
    rawExp.camera
FROM detResidImfile
JOIN detInputExp
    USING(det_id,exp_id)
JOIN rawExp
    USING(exp_id)
WHERE
    (detResidImfile.data_state = 'goto_cleaned'
     OR detResidImfile.data_state = 'goto_scrubbed'
     OR detResidImfile.data_state = 'goto_purged')



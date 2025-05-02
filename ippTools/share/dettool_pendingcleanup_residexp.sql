SELECT DISTINCT 
    detResidExp.*,
    rawExp.camera
FROM detResidExp
JOIN detInputExp
     USING(det_id,exp_id)
JOIN rawExp
    USING(exp_id)
WHERE
    (detResidExp.data_state = 'goto_cleaned'
     OR detResidExp.data_state = 'goto_scrubbed'
     OR detResidExp.data_state = 'goto_purged')

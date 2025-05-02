SELECT DISTINCT 
    detNormalizedExp.*,
    rawExp.camera    
FROM detNormalizedExp
JOIN detInputExp
     USING(det_id,exp_id)
JOIN rawExp
     USING(exp_id)
WHERE
    (detNormalizedExp.data_state = 'goto_cleaned'
     OR detNormalizedExp.data_state = 'goto_scrubbed'
     OR detNormalizedExp.data_state = 'goto_purged')


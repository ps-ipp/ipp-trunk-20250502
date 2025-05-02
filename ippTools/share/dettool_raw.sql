SELECT
    detInputExp.det_id,
    detRun.det_type,
    rawImfile.*
FROM detRun
JOIN detInputExp
    USING(det_id) 
JOIN rawImfile
    ON detInputExp.exp_id = rawImfile.exp_id
WHERE
    detRun.state = 'run'
    AND detRun.mode = 'master'

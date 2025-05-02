SELECT
    detRun.det_id,
    detRun.det_type,
    detRun.workdir,
    detRun.reduction,
    rawImfile.*,
    rawExp.exp_tag
FROM detRun
JOIN detInputExp
    USING(det_id, iteration)
JOIN rawExp
    ON detInputExp.exp_id = rawExp.exp_id
JOIN rawImfile
    ON detInputExp.exp_id = rawImfile.exp_id
LEFT JOIN detProcessedImfile
    ON detInputExp.det_id = detProcessedImfile.det_id
    AND rawImfile.exp_id = detProcessedImfile.exp_id
    AND rawImfile.class_id = detProcessedImfile.class_id
WHERE
    detRun.state = 'run'
    AND (detRun.mode = 'master' or detRun.mode = 'verify')
    AND detProcessedImfile.det_id IS NULL
    AND detProcessedImfile.exp_id IS NULL
    AND detProcessedImfile.class_id IS NULL

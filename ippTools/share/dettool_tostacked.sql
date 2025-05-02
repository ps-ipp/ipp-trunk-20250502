-- select detRun.iteration
-- select detProcessedImfile.det_id
-- select detProcessedImfile.class_id
-- by:
-- find the current iteration bassed on det_id
-- find all exp_ids in the current det_id/iteration from detInputExp
-- find all rawImfiles in the current exp_ids
-- compare to detProcessedImfiles by det_id/exp_id
-- found how many imfile there are in each class_id
-- and:
-- det_id is not in detStackedImfile;
-- iteration is not in detStackedImfile;
-- class_id is not in detStackedImfile;

SELECT
    detProcessedImfile.det_id,
    detRun.iteration,
    detRun.det_type,
    detRun.workdir,
    detRun.reduction,
    detProcessedImfile.class_id,
    rawExp.camera,
    rawExp.exp_tag
FROM detRun
JOIN detInputExp
    ON detRun.det_id = detInputExp.det_id
    AND detRun.iteration = detInputExp.iteration
JOIN rawExp
    ON detInputExp.exp_id = rawExp.exp_id
JOIN rawImfile
    ON detInputExp.exp_id = rawImfile.exp_id
LEFT JOIN detProcessedImfile
    ON detInputExp.det_id = detProcessedImfile.det_id
    AND detInputExp.exp_id = detProcessedImfile.exp_id
    AND rawImfile.class_id = detProcessedImfile.class_id
LEFT JOIN detStackedImfile
    ON detInputExp.det_id = detStackedImfile.det_id
    AND detInputExp.iteration = detStackedImfile.iteration
    AND rawImfile.class_id = detStackedImfile.class_id
WHERE
    detRun.state = 'run'
    AND detRun.mode = 'master'
    AND detStackedImfile.det_id IS NULL
    AND detStackedImfile.iteration IS NULL
    AND detStackedImfile.class_id IS NULL
    AND detInputExp.include = 1
GROUP BY
    rawImfile.class_id,
    detRun.det_id
HAVING
    COUNT(detProcessedImfile.class_id) = COUNT(rawImfile.class_id)
    AND SUM(detProcessedImfile.fault) = 0

-- select detProcessedImfile.det_id
-- select detRun.iteration
-- select detRun.det_type
-- select detProcessedImfile.exp_id
-- by:
-- find the current iteration bassed on det_id
-- find all exp_ids in the current det_id/iteration from detInputExp
-- find all rawImfiles in the current exp_ids
-- compare to detProcessedImfiles by det_id/exp_id
-- found how many imfile there are in each class_id
-- and:
-- det_id is not in detProcessedExp;
-- iteration is not in detProcessedExp;

SELECT DISTINCT
    det_id,
    iteration,
    det_type,
    exp_id,
    camera,
    workdir,
    exp_tag
FROM
    (SELECT DISTINCT
        detRun.det_id,
        detRun.iteration,
        detRun.det_type,
        detRun.workdir,
        detProcessedImfile.exp_id,
        detProcessedImfile.class_id,
        rawImfile.class_id as rawimfile_class_id,
        rawExp.camera,
        rawExp.exp_tag
    FROM detRun
    JOIN detInputExp
        USING(det_id, iteration)
    JOIN rawExp
        ON detInputExp.exp_id = rawExp.exp_id
    JOIN rawImfile
        ON rawExp.exp_id = rawImfile.exp_id
    LEFT JOIN detProcessedImfile
        ON detRun.det_id = detProcessedImfile.det_id
        AND detInputExp.exp_id = detProcessedImfile.exp_id
        AND rawImfile.class_id = detProcessedImfile.class_id
    LEFT JOIN detProcessedExp
        ON detProcessedImfile.det_id = detProcessedExp.det_id
        AND detProcessedImfile.exp_id = detProcessedExp.exp_id
    WHERE
        detRun.state = 'run'
        AND (detRun.mode = 'master' or detRun.mode = 'verify')
        AND detProcessedExp.det_id IS NULL
        AND detProcessedExp.exp_id IS NULL
        AND detInputExp.include = 1
    GROUP BY
        rawExp.exp_id,
        detRun.det_id
    HAVING
        COUNT(detProcessedImfile.class_id) = COUNT(rawImfile.class_id)
        AND SUM(detProcessedImfile.fault) = 0
    ) AS detProcessedExp


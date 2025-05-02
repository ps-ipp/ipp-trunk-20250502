-- select detRun.det_id
-- select detRun.iteration
-- select detRun.det_type
-- by:
-- find the current iteration bassed on det_id
-- find all exp_ids in the current det_id/iteration from detInputExp
-- find all exp_ids in the current det_id/iteration from detResidExp
-- compare the counts of exp_ids

SELECT DISTINCT
    det_id,
    iteration,
    det_type,
    mode,
    workdir,
    camera
FROM
    (SELECT DISTINCT
        detRun.det_id,
        detRun.iteration,
        detRun.det_type,
        detRun.mode,
        detRun.workdir,
        detInputExp.exp_id,
        rawExp.camera
    FROM detRun
    JOIN detInputExp
        USING(det_id, iteration)
    JOIN rawExp
        ON detInputExp.exp_id = rawExp.exp_id
    LEFT JOIN detResidExp
        ON detRun.det_id = detResidExp.det_id
        AND detRun.iteration = detResidExp.iteration
        AND detInputExp.exp_id = detResidExp.exp_id
    LEFT JOIN detRunSummary
        ON detRun.det_id = detRunSummary.det_id
        AND detRun.iteration = detRunSummary.iteration
    WHERE
        detRun.state = 'run'
        AND detRunSummary.det_id IS NULL
        AND detRunSummary.iteration IS NULL
	AND detResidExp.fault = 0
    GROUP BY
        detRun.det_id,
        detRun.iteration
    HAVING
        COUNT(detResidExp.exp_id) = COUNT(detInputExp.exp_id)
    ) AS Foo

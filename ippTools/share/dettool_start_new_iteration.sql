-- select detRun.iteration
-- select detInputExp.exp_id
-- select detResidExp.accept
-- by:
-- find the current iteration bassed on det_id
-- find all exp_ids in the current det_id/iteration from detInputExp
-- find all exp_ids in the current det_id/iteration from detResidExp
-- compare the counts of exp_ids

SELECT * FROM
    (SELECT DISTINCT
        detRun.det_id AS det_id,
        detRun.iteration,
        detInputExp.exp_id,
        detResidExp.accept
    FROM detRun
    JOIN detInputExp
        ON detRun.det_id = detInputExp.det_id
        AND detRun.iteration = detInputExp.iteration
    JOIN detResidExp
        ON detRun.det_id = detResidExp.det_id
        AND detRun.iteration = detResidExp.iteration
        AND detInputExp.exp_id = detResidExp.exp_id
    WHERE
        detRun.state = 'run'
        AND detRun.mode = 'master'
    GROUP BY
        detRun.det_id,
        detRun.iteration,
        detInputExp.exp_id
    HAVING
        COUNT(detResidExp.exp_id) = COUNT(detInputExp.exp_id)
    ) as Foo

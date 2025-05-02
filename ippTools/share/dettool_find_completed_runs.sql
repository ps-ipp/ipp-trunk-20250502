SELECT DISTINCT
   det_id,
   iteration
FROM
    (SELECT DISTINCT
        detRun.det_id,
        detRun.iteration,
        detInputExp.exp_id
    FROM detRun
        LEFT JOIN detInputExp
        ON detRun.det_id = detInputExp.det_id
        AND detRun.iteration = detInputExp.iteration
    LEFT JOIN rawExp
        ON detInputExp.exp_id = rawExp.exp_id
    LEFT JOIN detResidExp
        ON detRun.det_id = detResidExp.det_id
        AND detRun.iteration = detResidExp.iteration
        AND detInputExp.exp_id = detResidExp.exp_id
   WHERE
        detRun.state = 'run'
   GROUP BY
        detRun.det_id,
        detRun.iteration
    HAVING
        COUNT(detResidExp.exp_id) = COUNT(detInputExp.exp_id)
    ) AS residdetrun

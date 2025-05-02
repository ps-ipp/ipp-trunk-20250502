-- this is used by both 'toresidexp' and 'addresidexp'

-- which returns a list of exposures for which all component class ids have had
-- residuals created.  This list includes the detrend id, iteration, exposure
-- id, detrend type, and whether the exposure was included in the stack for
-- this iteration.

-- require the corresponding detNormalizedExp to complete before starting

-- select detRun.det_id
-- select detRun.iteration
-- select detRun.det_type
-- select detInputExp.exp_id
-- select detInputExp.include
-- by:
-- find the current iteration bassed on det_id
-- find all exp_ids in the current det_id/iteration from detInputExp
-- compare to detInputExp.imfiles to derResidImfile by class_id
-- and:
-- detResidImfile.{det_id, iteration, exp_id} is not in detResidExp

SELECT
    det_id,
    iteration,
    det_type,
    mode,
    exp_id,
    include,
    camera,
    workdir,
    exp_tag
FROM 
(   SELECT
        detRun.det_id AS det_id,
        detRun.iteration,
        detRun.det_type,
        detRun.mode,
        detInputExp.exp_id,
        detInputExp.include,
        rawExp.camera,
        detRun.workdir,
        rawExp.exp_tag
    FROM detRun
    JOIN detNormalizedExp
    	USING(det_id, iteration)
    JOIN detInputExp
        USING(det_id, iteration)
    JOIN rawExp
        ON detInputExp.exp_id = rawExp.exp_id
    JOIN rawImfile
        on rawExp.exp_id = rawImfile.exp_id
    LEFT JOIN detResidImfile
        ON detRun.det_id = detResidImfile.det_id
        AND detRun.iteration = detResidImfile.iteration
        AND detInputExp.exp_id = detResidImfile.exp_id
        AND rawImfile.class_id = detResidImfile.class_id
    LEFT JOIN detResidExp
        ON detResidImfile.det_id = detResidExp.det_id
        AND detResidImfile.iteration = detResidExp.iteration
        AND detResidImfile.exp_id = detResidExp.exp_id
    WHERE
        detRun.state = 'run'
        AND detRun.mode = 'master'
        AND detResidExp.det_id IS NULL
        AND detResidExp.iteration IS NULL
        AND detResidExp.exp_id IS NULL
    GROUP BY
        detInputExp.exp_id,
        detRun.iteration,
        detRun.det_id
    HAVING
        COUNT(rawImfile.class_id) = COUNT(detResidImfile.class_id)
        AND SUM(detResidImfile.fault) = 0
    UNION
    SELECT
        detRun.det_id AS det_id,
        detRun.iteration,
        detRun.det_type,
        detRun.mode,
        detInputExp.exp_id,
        detInputExp.include,
        rawExp.camera,
        detRun.workdir,
        rawExp.exp_tag
    FROM detRun
    JOIN detInputExp
        USING(det_id, iteration)
    JOIN rawExp
        ON detInputExp.exp_id = rawExp.exp_id
    JOIN rawImfile
        on rawExp.exp_id = rawImfile.exp_id
    LEFT JOIN detResidImfile
        ON detRun.det_id = detResidImfile.det_id
        AND detRun.iteration = detResidImfile.iteration
        AND detInputExp.exp_id = detResidImfile.exp_id
        AND rawImfile.class_id = detResidImfile.class_id
    LEFT JOIN detResidExp
        ON detResidImfile.det_id = detResidExp.det_id
        AND detResidImfile.iteration = detResidExp.iteration
        AND detResidImfile.exp_id = detResidExp.exp_id
    WHERE
        detRun.state = 'run'
        AND detRun.mode = 'verify'
        AND detResidExp.det_id IS NULL
        AND detResidExp.iteration IS NULL
        AND detResidExp.exp_id IS NULL
    GROUP BY
        detInputExp.exp_id,
        detRun.iteration,
        detRun.det_id
    HAVING
        COUNT(rawImfile.class_id) = COUNT(detResidImfile.class_id)
        AND SUM(detResidImfile.fault) = 0
) as temp

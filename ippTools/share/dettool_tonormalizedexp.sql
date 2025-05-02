-- this is used by both 'tonormalizedexp' and 'addnormalizedexp'

SELECT
    *
FROM (SELECT 
      *
    FROM (
        SELECT DISTINCT
            detRun.det_id,
            detRun.iteration,
            detRun.det_type,
            detRun.workdir,
            rawExp.camera,
            rawExp.telescope,
            rawExp.exp_type,
            detNormalizedImfile.class_id as norm_class_id,
            detNormalizedImfile.fault,
            detStackedImfile.class_id as stack_class_id
        FROM detRun
        JOIN detInputExp
            ON detRun.det_id = detInputExp.det_id
            AND detRun.iteration = detInputExp.iteration
        JOIN rawExp
            ON detInputExp.exp_id = rawExp.exp_id
        JOIN detStackedImfile
            ON detStackedImfile.det_id = detRun.det_id
            AND detStackedImfile.iteration = detRun.iteration
        LEFT JOIN detNormalizedImfile
            ON detStackedImfile.det_id = detNormalizedImfile.det_id
            AND detStackedImfile.iteration = detNormalizedImfile.iteration
            AND detStackedImfile.class_id = detNormalizedImfile.class_id
        LEFT JOIN detNormalizedExp
            ON detInputExp.det_id = detNormalizedExp.det_id
            AND detInputExp.iteration = detNormalizedExp.iteration
        WHERE
            detRun.state = 'run' AND
            detRun.mode = 'master' AND
            detNormalizedExp.det_id IS NULL AND
            detNormalizedExp.iteration IS NULL AND
            detInputExp.include = 1
    ) as Foo
    GROUP BY
      det_id,
      iteration
    HAVING
      COUNT(norm_class_id) = COUNT(stack_class_id)
      AND SUM(fault) = 0
) as Bar

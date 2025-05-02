SELECT DISTINCT
    *
FROM
    (SELECT DISTINCT
	detRun.state,
        detNormalizedImfile.*
    FROM detNormalizedImfile
    JOIN detRun
        USING(det_id)
    JOIN detRunSummary
        ON detNormalizedImfile.det_id = detRunSummary.det_id
        AND detNormalizedImfile.iteration = detRunSummary.iteration
    WHERE
        detRun.mode  = 'master'
        AND detRunSummary.accept = 1
    UNION
    SELECT DISTINCT
	detRun.state,
        detRegisteredImfile.*
    FROM detRegisteredImfile
    JOIN detRun
        USING(det_id)
    WHERE
        (detRun.mode  = 'register' OR detRun.mode = 'correction')
    ) as Foo
WHERE
    (state = 'stop' OR state = 'register')

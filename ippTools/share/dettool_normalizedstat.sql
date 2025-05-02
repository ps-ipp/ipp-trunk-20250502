SELECT
    *
FROM
    (SELECT
        detNormalizedStatImfile.*
    FROM detNormalizedStatImfile
    JOIN detRun
    USING(det_id, iteration)
    WHERE
        detRun.state = 'run'
        AND detRun.mode = 'master'
    ) as Foo

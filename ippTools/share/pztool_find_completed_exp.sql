SELECT DISTINCT
    summit_id,
    exp_name,
    camera,
    telescope,
    dateobs,
    state
FROM (
    SELECT
        pzDownloadImfile.*,
        pzDownloadExp.state,
        summitExp.imfiles,
        summitExp.dateobs
    FROM pzDownloadExp
    JOIN summitExp
        USING(summit_id)
    LEFT JOIN pzDownloadImfile
        USING(summit_id)
    LEFT JOIN newExp
        USING(summit_id)
    WHERE
        pzDownloadExp.state = 'run'
        AND newExp.summit_id IS NULL
        AND newExp.tmp_exp_name IS NULL
        AND newExp.tmp_camera IS NULL
        AND newExp.tmp_telescope IS NULL
    GROUP BY
        pzDownloadExp.summit_id
    -- it doesn't matter which field in pzDownloadImfile we count as we've
    -- already Download a group by
    HAVING
        COUNT(pzDownloadImfile.exp_name) = summitExp.imfiles
        AND SUM(pzDownloadImfile.fault) = 0
    ) as Foo

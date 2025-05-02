-- the output of this query must match the format of fakeRun row
SELECT DISTINCT
    fake_id,
    cam_id,
    state,
    workdir,
    label,
    data_group,
    dist_group,
    reduction,
    expgroup,
    dvodb,
    tess_id,
    end_stage,
    epoch,
    note
FROM
    (SELECT
        fakeRun.*,
        rawImfile.class_id as rawimfile_class_id,
        fakeProcessedImfile.class_id
    FROM fakeRun
    JOIN camRun
        USING(cam_id)
    JOIN chipRun
        USING(chip_id)
    JOIN rawExp
        USING(exp_id)
    JOIN rawImfile
        ON rawImfile.exp_id = rawExp.exp_id
        AND rawImfile.ignored = 0
    LEFT JOIN fakeProcessedImfile
        ON fakeRun.fake_id = fakeProcessedImfile.fake_id
        AND rawImfile.exp_id = fakeProcessedImfile.exp_id
        AND rawImfile.class_id = fakeProcessedImfile.class_id
    WHERE
        fakeRun.state = 'new'
    GROUP BY
        fakeRun.fake_id,
        rawExp.exp_id
    HAVING
        COUNT(rawImfile.class_id) = COUNT(fakeProcessedImfile.class_id)
        AND SUM(fakeProcessedImfile.fault) = 0
    ) as Foo

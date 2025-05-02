-- this query is used by both camtool -pendingexp & camtool -addprocessedexp it
-- does a little more work then is necessary for -addprocessed but it seems
-- "cleaner" to use the same query for both cases
SELECT * FROM
    (SELECT
        fakeRun.*,
        rawExp.exp_tag,
        rawExp.exp_id,
        rawExp.exp_name,
        rawExp.camera,
        rawExp.telescope,
        rawExp.filelevel
    FROM fakeRun
    JOIN camRun
        USING(cam_id)
    JOIN chipRun
        USING(chip_id)
    JOIN chipProcessedImfile
        USING(chip_id)
    JOIN rawExp
        ON chipRun.exp_id = rawExp.exp_id
    LEFT JOIN fakeProcessedImfile
        USING(fake_id)
    LEFT JOIN fakeMask
        ON fakeRun.label = fakeMask.label
    WHERE
        fakeRun.state = 'new'
        AND camRun.state = 'full'
        AND chipRun.state = 'full'
        AND fakeMask.label IS NULL
        AND fakeProcessedImfile.fake_id IS NOT NULL
    GROUP BY
        fakeRun.fake_id
    ) as Foo

SELECT * FROM
    (SELECT
        -- warp input
        diffRun.diff_id,
        diffInputSkyfile.skycell_id,
        diffInputSkyfile.tess_id,
        0 as stack_id,
        warpSkyfile.warp_id,
        warpSkyfile.uri,
        warpSkyfile.path_base,
        warpSkyfile.magicked as magicked,
        0 as template,
        rawExp.camera
    FROM diffRun
    JOIN diffInputSkyfile
        USING(diff_id)
    JOIN warpSkyfile
        ON  diffInputSkyfile.warp1      = warpSkyfile.warp_id
        AND diffInputSkyfile.skycell_id = warpSkyfile.skycell_id
        AND diffInputSkyfile.tess_id    = warpSkyfile.tess_id
    JOIN warpRun
        ON diffInputSkyfile.warp1 = warpRun.warp_id
    JOIN fakeRun
        USING(fake_id)
    JOIN camRun
        USING(cam_id)
    JOIN chipRun
        USING(chip_id)
    JOIN rawExp
        USING(exp_id)
        -- where hook %s
    UNION
    SELECT
        -- warp template
        diffRun.diff_id,
        diffInputSkyfile.skycell_id,
        diffInputSkyfile.tess_id,
        0 as stack_id,
        warpSkyfile.warp_id,
        warpSkyfile.uri,
        warpSkyfile.path_base,
        warpSkyfile.magicked as magicked,
        1 as template,
        rawExp.camera
    FROM diffRun
    JOIN diffInputSkyfile
        USING(diff_id)
    JOIN warpSkyfile
        ON  diffInputSkyfile.warp2      = warpSkyfile.warp_id
        AND diffInputSkyfile.skycell_id = warpSkyfile.skycell_id
        AND diffInputSkyfile.tess_id    = warpSkyfile.tess_id
    JOIN warpRun
        ON diffInputSkyfile.warp2 = warpRun.warp_id
    JOIN fakeRun
        USING(fake_id)
    JOIN camRun
        USING(cam_id)
    JOIN chipRun
        USING(chip_id)
    JOIN rawExp
        USING(exp_id)
        -- where hook %s
    UNION
    SELECT 
        -- stack input
        diffRun.diff_id,
        diffInputSkyfile.skycell_id,
        diffInputSkyfile.tess_id,
        stackSumSkyfile.stack_id,
        0 as warp_id,
        stackSumSkyfile.uri,
        stackSumSkyfile.path_base,
        0 as magicked,
        0 as template,
        rawExp.camera
    FROM diffRun
    JOIN diffInputSkyfile
        USING(diff_id)
    JOIN stackSumSkyfile
        ON  diffInputSkyfile.stack1 = stackSumSkyfile.stack_id
    JOIN stackInputSkyfile
        ON diffInputSkyfile.stack1 = stackInputSkyfile.stack_id
    JOIN warpRun
        ON stackInputSkyfile.warp_id = warpRun.warp_id
    JOIN fakeRun
        USING(fake_id)
    JOIN camRun
        USING(cam_id)
    JOIN chipRun
        USING(chip_id)
    JOIN rawExp
        USING(exp_id)
        -- where hook %s
    UNION
    SELECT 
        -- stack template
        diffRun.diff_id,
        diffInputSkyfile.skycell_id,
        diffInputSkyfile.tess_id,
        stackSumSkyfile.stack_id,
        0 as warp_id,
        stackSumSkyfile.uri,
        stackSumSkyfile.path_base,
        0 as magicked,
        1 as template,
        rawExp.camera
    FROM diffRun
    JOIN diffInputSkyfile
        USING(diff_id)
    JOIN stackSumSkyfile
        ON  diffInputSkyfile.stack2 = stackSumSkyfile.stack_id
    JOIN stackInputSkyfile
        ON diffInputSkyfile.stack2 = stackInputSkyfile.stack_id
    JOIN warpRun
        ON stackInputSkyfile.warp_id = warpRun.warp_id
    JOIN fakeRun
        USING(fake_id)
    JOIN camRun
        USING(cam_id)
    JOIN chipRun
        USING(chip_id)
    JOIN rawExp
        USING(exp_id)
        -- where hook %s
    ) as Foo
-- template where hook %s

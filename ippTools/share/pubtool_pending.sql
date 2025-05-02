SELECT
    publishToDo.*
FROM (
    -- Get the diffs
    -- The following is only appropriate for a diff where warp1 is set; otherwise it's more difficult to get the camera name
    SELECT DISTINCT
        publishRun.pub_id,
        publishClient.product,
        publishClient.stage,
        publishClient.workdir,
	publishClient.output_format,
        publishClient.magicked AS need_magic,
        diffRun.diff_id AS stage_id,
        rawExp.camera,
        rawExp.exp_id
    FROM publishRun
    JOIN publishClient USING(client_id)
    JOIN diffRun
        ON diffRun.diff_id = publishRun.stage_id
    JOIN diffInputSkyfile USING(diff_id)
    -- Need to do something fancy here to get the camera name for a stack
    LEFT JOIN warpRun ON warpRun.warp_id = diffInputSkyfile.warp1
    JOIN fakeRun USING(fake_id)
    JOIN camRun USING(cam_id)
    JOIN chipRun USING(chip_id)
    JOIN rawExp USING(exp_id)
    WHERE publishClient.stage = 'diff'
        AND publishClient.active = 1
        AND publishRun.state = 'new'
        AND diffRun.state IN ('full', 'cleaned', 'goto_cleaned')
        AND (diffRun.magicked != 0 OR diffRun.diff_mode = 4 OR publishClient.magicked = 0)
        -- WHERE hook %s
    UNION
    SELECT
        publishRun.pub_id,
        publishClient.product,
        publishClient.stage,
        publishClient.workdir,
	publishClient.output_format,
        publishClient.magicked AS need_magic,
        camRun.cam_id AS stage_id,
        rawExp.camera,
        rawExp.exp_id
    FROM publishRun
    JOIN publishClient USING(client_id)
    JOIN camRun
        ON camRun.cam_id = publishRun.stage_id
    JOIN chipRun USING(chip_id)
    JOIN rawExp USING(exp_id)
    WHERE publishClient.stage = 'camera'
        AND publishClient.active = 1
        AND publishRun.state ='new'
        AND camRun.state IN ('full', 'cleaned', 'goto_cleaned')
        AND (camRun.magicked != 0 OR publishClient.magicked = 0)
        -- WHERE hook %s
    UNION
    SELECT DISTINCT
        publishRun.pub_id,
        publishClient.product,
        publishClient.stage,
        publishClient.workdir,
	publishClient.output_format,
        publishClient.magicked AS need_magic,
        diffPhotRun.diff_phot_id AS stage_id,
        rawExp.camera,
        rawExp.exp_id
    FROM publishRun
    JOIN publishClient USING(client_id)
    JOIN diffPhotRun
        ON diffPhotRun.diff_phot_id = publishRun.stage_id
    JOIN diffRun USING(diff_id)
    JOIN diffInputSkyfile USING(diff_id)
    -- Need to do something fancy here to get the camera name for a stack
    LEFT JOIN warpRun ON warpRun.warp_id = diffInputSkyfile.warp1
    JOIN fakeRun USING(fake_id)
    JOIN camRun USING(cam_id)
    JOIN chipRun USING(chip_id)
    JOIN rawExp USING(exp_id)
    WHERE publishClient.stage = 'diffphot'
        AND publishClient.active = 1
        AND publishRun.state = 'new'
        AND diffPhotRun.state IN ('full', 'cleaned', 'goto_cleaned')
        AND (diffPhotRun.magicked != 0 OR diffRun.diff_mode = 4 OR publishClient.magicked = 0)
        -- WHERE hook %s
) AS publishToDo
LEFT JOIN publishDone USING(pub_id)
WHERE publishDone.pub_id IS NULL

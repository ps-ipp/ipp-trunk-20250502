-- Get runs to publish 
SELECT DISTINCT
    client_id,
    stage_id,
    src_label,
    output_format
FROM (
    -- Get diffs to publish
    SELECT DISTINCT
        client_id,
        diff_id AS stage_id,
        diffRun.label AS src_label,
	output_format
    FROM publishClient
    JOIN diffRun
    JOIN diffInputSkyfile USING(diff_id)
    JOIN diffSkyfile using(diff_id)
    JOIN warpRun ON warpRun.warp_id = diffInputSkyfile.warp1 -- Only JOINing input, not reference!
    JOIN fakeRun USING(fake_id)
    JOIN camRun USING(cam_id)
    JOIN chipRun USING(chip_id)
    JOIN rawExp USING(exp_id)
    LEFT JOIN magicDSRun ON magicDSRun.stage = 'diff' AND magicDSRun.stage_id = diff_id
    WHERE publishClient.stage = 'diff'
        AND publishClient.active = 1
        AND diffRun.state IN ('full', 'cleaned', 'goto_cleaned')
        AND ( diffRun.diff_mode = 4
           OR (publishClient.magicked AND diffRun.magicked != 0)
           OR (publishClient.magicked = 0 AND 
                (diffRun.magicked = 0 OR (diffRun.magicked != 0 AND magic_ds_id IS NOT NULL))
              )
           )
        AND diffSkyfile.quality = 0
    -- WHERE hook %s
    UNION
    -- Get cameras to publish
    SELECT
        client_id,
        cam_id AS stage_id,
        camRun.label AS src_label,
	output_format
    FROM publishClient
    JOIN camRun
    JOIN chipRun USING(chip_id)
    JOIN rawExp USING(exp_id)
    WHERE publishClient.stage = 'camera'
        AND publishClient.active = 1
        AND camRun.state IN ('full', 'cleaned', 'goto_cleaned')
        AND (camRun.magicked != 0 OR publishClient.magicked = 0)
    -- WHERE hook %s
    UNION
    -- Get diffphots to publish
    SELECT DISTINCT
        client_id,
        diff_phot_id AS stage_id,
        diffPhotRun.label AS src_label,
	output_format
    FROM publishClient
    JOIN diffPhotRun
    JOIN diffRun USING(diff_id)
    JOIN diffInputSkyfile USING(diff_id)
    JOIN warpRun ON warpRun.warp_id = diffInputSkyfile.warp1 -- Only JOINing input, not reference!
    JOIN fakeRun USING(fake_id)
    JOIN camRun USING(cam_id)
    JOIN chipRun USING(chip_id)
    JOIN rawExp USING(exp_id)
    WHERE publishClient.stage = 'diffphot'
        AND publishClient.active = 1
        AND diffPhotRun.state IN ('full', 'cleaned', 'goto_cleaned')
        AND (diffPhotRun.magicked != 0 OR diffRun.diff_mode = 4 OR publishClient.magicked = 0)
    -- WHERE hook %s
    ) AS publishToDo
-- Only get stuff that hasn't been published
LEFT JOIN publishRun USING(client_id, stage_id)

SELECT DISTINCT       -- DISTINCT because we are going though diffInputSkyfile
    magicRun.magic_id,
    chipRun.exp_id,
    'chip' AS stage,
    chipRun.chip_id AS stage_id,
    chipRun.data_group,
    camRun.cam_id,
    chipRun.label,
    CAST(NULL AS SIGNED) AS inv_magic_id
FROM magicRun
    JOIN magicMask USING(magic_id)
    JOIN diffRun USING(diff_id)
    JOIN diffInputSkyfile USING(diff_id)
    JOIN warpRun
        ON (!magicRun.inverse AND warp1 = warp_id)
        OR ( magicRun.inverse AND warp2 = warp_id)
    JOIN fakeRun USING(fake_id)
    JOIN camRun AS oldCamRun USING(cam_id)
    -- This ends the usual JOIN down to the raw level
    JOIN chipRun
        ON chipRun.exp_id = magicRun.exp_id
        AND chipRun.chip_id != oldCamRun.chip_id -- Not allowed to copy yourself
    JOIN camRun ON
        camRun.chip_id = chipRun.chip_id
    JOIN rawExp
        ON rawExp.exp_id = chipRun.exp_id
    LEFT JOIN magicDSRun
        ON magicRun.magic_id = magicDSRun.magic_id
        AND magicDSRun.stage  = 'chip'
        AND magicDSRun.label = '%s'
WHERE magicRun.state = 'full'
    -- HOOK: NEWLINE HERE TO ACTIVATE %s AND magicDSRun.magic_ds_id IS NULL
    AND chipRun.magicked  = 0
    AND chipRun.state = 'full'
    AND camRun.state = 'full'

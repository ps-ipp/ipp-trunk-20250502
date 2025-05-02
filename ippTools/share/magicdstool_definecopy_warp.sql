SELECT DISTINCT       -- DISTINCT because we are going though diffInputSkyfile
    magicRun.magic_id,
    chipRun.exp_id,
    'warp' AS stage,
    warpRun.warp_id AS stage_id,
    warpRun.data_group,
    camRun.cam_id,
    warpRun.label,
    CAST(NULL AS SIGNED) AS inv_magic_id
FROM magicRun
    JOIN magicMask USING(magic_id)
    JOIN diffRun USING(diff_id)
    JOIN diffInputSkyfile USING(diff_id)
    JOIN warpRun AS oldWarpRun
        ON (!magicRun.inverse AND diffInputSkyfile.warp1 = oldWarpRun.warp_id)
        OR ( magicRun.inverse AND diffInputSkyfile.warp2 = oldWarpRun.warp_id)
    -- This ends the usual JOIN down to the raw level
    JOIN rawExp
        ON rawExp.exp_id = magicRun.exp_id
    JOIN chipRun
        ON chipRun.exp_id = rawExp.exp_id
    JOIN camRun
        ON camRun.chip_id = chipRun.chip_id
    JOIN fakeRun
        ON fakeRun.cam_id = camRun.cam_id
    JOIN warpRun
        ON warpRun.fake_id = fakeRun.fake_id
        AND warpRun.warp_id != oldWarpRun.warp_id -- Not allowed to copy yourself
    LEFT JOIN magicDSRun
        ON magicRun.magic_id = magicDSRun.magic_id
        AND magicDSRun.stage  = 'warp'
        AND magicDSRun.label = '%s'
WHERE magicRun.state = 'full'
    -- HOOK: NEWLINE HERE TO ACTIVATE %s AND magicDSRun.magic_ds_id IS NULL
    AND warpRun.magicked  = 0
    AND warpRun.state = 'full'

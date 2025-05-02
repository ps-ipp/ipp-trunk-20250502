SELECT DISTINCT       -- DISTINCT because we are going though diffInputSkyfile
    magicRun.magic_id,
    exp_id,
    'camera' AS stage,
    camRun.cam_id AS stage_id,
    camRun.data_group,
    camRun.cam_id,
    magicRun.label,
    magicRun.workdir,
    CAST(NULL AS SIGNED) AS inv_magic_id,
    CAST(NULL AS SIGNED) AS inv_exp_id
FROM magicRun
    JOIN magicMask USING(magic_id)
    JOIN diffRun USING(diff_id)
    JOIN diffInputSkyfile USING(diff_id)
    JOIN warpRun ON (!magicRun.inverse AND warp1 = warp_id) 
                 OR ( magicRun.inverse AND warp2 = warp_id)
    JOIN fakeRun USING(fake_id)
    JOIN camRun USING(cam_id)
    JOIN chipRun USING(chip_id, exp_id)
    JOIN rawExp USING(exp_id)
    LEFT JOIN magicDSRun ON magicRun.magic_id = magicDSRun.magic_id
                         AND magicDSRun.stage  = 'camera'
WHERE magicRun.state = 'full'
    AND ( -- rerun HOOK magicdstool sends "\n 1 " if rerun else "\n 0 " %s
        OR magicDSRun.magic_ds_id IS NULL)
    AND camRun.magicked  = 0
    AND chipRun.magicked
    AND chipRun.state = 'full'
    AND camRun.state = 'full'


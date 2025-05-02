SELECT DISTINCT       -- DISTINCT because we are going though diffInputSkyfile
    magicRun.magic_id,
    exp_id,
    'chip' AS stage,
    chip_id AS stage_id,
    chipRun.data_group,
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
                         AND magicDSRun.stage  = 'chip'
WHERE magicRun.state = 'full'
    AND ( -- rerun HOOK magicdstool sends "\n1 " if rerun else "\n0 " %s
        OR magicDSRun.magic_ds_id IS NULL)
    AND chipRun.magicked  = 0
    AND chipRun.state = 'full'
    AND camRun.state = 'full'


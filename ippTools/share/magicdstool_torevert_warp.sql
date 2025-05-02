SELECT
    magic_ds_id,
    magicDSRun.state,
    exp_id,
    re_place,
    camera,
    stage,
    stage_id,
    component,
    outroot,
    warpSkyfile.path_base,
    "NULL" AS cam_path_base,
    recovery_path_base,
    0 AS bothways,
    0 AS bytes,
    0 AS md5sum,
    warpRun.magicked
FROM magicDSRun
    JOIN magicDSFile USING(magic_ds_id)
    JOIN warpSkyfile ON (stage_id = warp_id AND component = skycell_id)
    JOIN warpRun USING(warp_id)
    JOIN fakeRun USING(fake_id)
    JOIN camRun ON fakeRun.cam_id = camRun.cam_id
    JOIN chipRun USING(chip_id)
    JOIN rawExp USING(exp_id)
WHERE magicDSRun.stage = 'warp'
    AND ((magicDSRun.state = 'new' AND magicDSFile.fault > 0)
         OR ((magicDSRun.state = 'goto_censored' OR magicDSRun.state = 'goto_restored')
              AND ((backup_path_base IS NOT NULL) OR (recovery_path_base IS NOT NULL))
            )
        )

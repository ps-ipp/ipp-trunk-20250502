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
    camProcessedExp.path_base,
    camProcessedExp.path_base AS cam_path_base,
    recovery_path_base,
    0 AS bothways,
    0 AS bytes,
    0 AS md5sum,
    camRun.magicked
FROM magicDSRun
    JOIN magicDSFile using(magic_ds_id)
    JOIN camProcessedExp ON stage_id = camProcessedExp.cam_id
    JOIN camRun ON camRun.cam_id = camProcessedExp.cam_id
    JOIN chipRun USING(chip_id)
    JOIN rawExp using(exp_id)
WHERE magicDSRun.stage = 'camera'
    AND camProcessedExp.fault = 0
    AND camRun.state = 'full'
    AND ((magicDSRun.state = 'new' AND magicDSFile.fault > 0)
         OR ((magicDSRun.state = 'goto_censored' OR magicDSRun.state = 'goto_restored')
              AND (backup_path_base IS NOT NULL)
            )
        )

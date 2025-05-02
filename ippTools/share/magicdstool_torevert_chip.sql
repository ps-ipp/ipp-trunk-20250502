SELECT
    magic_ds_id,
    magicDSRun.state,
    rawExp.exp_id,
    re_place,
    camera,
    stage,
    stage_id,
    component,
    outroot,
    chipProcessedImfile.path_base,
    camProcessedExp.path_base AS cam_path_base,
    recovery_path_base,
    0 AS bothways,
    0 AS bytes,
    0 AS md5sum,
    chipRun.magicked
FROM magicDSRun
    JOIN magicDSFile using(magic_ds_id)
    JOIN camProcessedExp using(cam_id)
    JOIN chipRun ON (stage_id = chipRun.chip_id)
    JOIN chipProcessedImfile ON (stage_id = chipProcessedImfile.chip_id AND component = class_id)
    JOIN rawExp ON chipRun.exp_id = rawExp.exp_id
WHERE magicDSRun.stage = 'chip'
    AND (((magicDSRun.state = 'new' OR (magicDSRun.state = 'update'))
            AND magicDSFile.fault > 0)
            -- why don't we require a fault for these states?
         OR ((magicDSRun.state = 'goto_censored' OR magicDSRun.state = 'goto_restored')
              AND ((backup_path_base IS NOT NULL) OR (recovery_path_base IS NOT NULL))
            )
        )

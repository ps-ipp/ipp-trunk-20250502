SELECT
    magic_ds_id,
    magicDSRun.state,
    exp_id,
    re_place,
    rawImfile.camera,
    stage,
    stage_id,
    component,
    outroot,
    rawImfile.uri AS path_base,
    CAST(NULL AS CHAR(255)) AS cam_path_base, 
    recovery_path_base,
    0 AS bothways,
    bytes,
    md5sum,
    0 AS magicked
FROM magicDSRun
    JOIN magicDSFile using(magic_ds_id)
    JOIN rawImfile ON (stage_id = exp_id AND component = class_id)
WHERE magicDSRun.stage = 'raw'
    AND ((magicDSRun.state = 'new' AND magicDSFile.fault > 0)
         OR ((magicDSRun.state = 'goto_censored' OR magicDSRun.state = 'goto_restored')
              AND ((backup_path_base IS NOT NULL) OR (recovery_path_base IS NOT NULL))
            )
        )

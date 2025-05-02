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
    diffSkyfile.path_base,
    magicDSFile.backup_path_base,
    magicDSFile.recovery_path_base,
    "NULL" AS cam_path_base,
    CAST(diffRun.bothways AS SIGNED) AS bothways,
    0 AS bytes,
    0 AS md5sum,
    diffRun.magicked
FROM magicDSRun
    JOIN magicRun using(magic_id)
    JOIN magicDSFile USING(magic_ds_id)
    JOIN diffSkyfile ON (stage_id = diffSkyfile.diff_id AND component = skycell_id)
    JOIN diffRun ON diffSkyfile.diff_id = diffRun.diff_id
    JOIN rawExp USING(exp_id)
WHERE magicDSRun.stage = 'diff'
    AND ((magicDSRun.state = 'new' AND magicDSFile.fault > 0)
         OR ((magicDSRun.state = 'goto_censored' OR magicDSRun.state = 'goto_restored')
              AND ((backup_path_base IS NOT NULL) OR (recovery_path_base IS NOT NULL))
            )
        )

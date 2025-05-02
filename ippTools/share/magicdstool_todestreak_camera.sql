SELECT DISTINCT
    magicDSRun.magic_ds_id,
    magicDSRun.magic_id,
    magicDSRun.state,
    chipRun.exp_id,
    camRun.magicked,
    magicDSRun.label,
    camera,
    magicMask.uri AS streaks_uri,
    magicMask.path_base AS streaks_path_base,
    CAST(NULL AS CHAR(255)) AS inv_streaks_uri,
    CAST(NULL AS CHAR(255)) AS inv_streaks_path_base,
    stage,
    stage_id,
    'exposure' AS component,
    0 AS mismatched_tess,
    CAST(NULL AS CHAR(255)) AS uri,
    camProcessedExp.path_base,
    magicRun.inverse,
    camProcessedExp.path_base AS cam_path_base,
    camRun.reduction AS cam_reduction,
    outroot,
    recoveryroot,
    re_place,
    remove,
    IFNULL(Label.priority, 10000) AS priority
FROM magicDSRun
JOIN magicMask USING (magic_id)
JOIN magicRun USING(magic_id)
JOIN camRun ON magicDSRun.stage_id = camRun.cam_id
JOIN camProcessedExp ON camRun.cam_id = camProcessedExp.cam_id
JOIN chipRun USING(chip_id)
JOIN rawExp ON chipRun.exp_id = rawExp.exp_id
LEFT JOIN magicDSFile
    ON magicDSRun.magic_ds_id = magicDSFile.magic_ds_id
LEFT JOIN Label ON magicDSRun.label = Label.label
WHERE
    magicDSRun.state = 'new'
    AND magicDSRun.stage = 'camera'
    AND camRun.state = 'full'
    AND ((chipRun.state = 'full' AND chipRun.magicked > 0) 
         OR (chipRun.state = 'cleaned' AND chipRun.magicked < 0)) 
    AND camProcessedExp.fault = 0
    AND camProcessedExp.quality = 0
    AND magicDSFile.component IS NULL
    AND (Label.active OR Label.active IS NULL)

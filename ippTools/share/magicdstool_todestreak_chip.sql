SELECT DISTINCT
    magicDSRun.magic_ds_id,
    magicDSRun.magic_id,
    magicDSRun.state,
    chipRun.exp_id,
    chipRun.magicked,
    magicDSRun.label,
    camera,
    magicMask.uri AS streaks_uri,
    magicMask.path_base AS streaks_path_base,
    CAST(NULL AS CHAR(255)) AS inv_streaks_uri,
    CAST(NULL AS CHAR(255)) AS inv_streaks_path_base,
    stage,
    stage_id,
    class_id AS component,
    0 AS mismatched_tess,
    chipProcessedImfile.uri,
    chipProcessedImfile.path_base,
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
JOIN camRun USING(cam_id)
JOIN camProcessedExp USING(cam_id)
JOIN chipRun ON chipRun.chip_id = stage_id
JOIN chipProcessedImfile ON chipRun.chip_id = chipProcessedImfile.chip_id
JOIN rawExp ON chipRun.exp_id = rawExp.exp_id
LEFT JOIN magicDSFile
    ON magicDSRun.magic_ds_id = magicDSFile.magic_ds_id
    AND magicDSFile.component = chipProcessedImfile.class_id
LEFT JOIN Label
    ON magicDSRun.label = Label.label
WHERE
    ((magicDSRun.state = 'new' AND magicDSFile.component IS NULL)
     OR (magicDSRun.state = 'update' AND magicDSFile.data_state = 'update'
         AND magicDSFile.fault = 0))
    AND magicDSRun.stage = 'chip'
    AND (chipRun.state = 'full' OR (chipRun.state = 'update' and chipProcessedImfile.data_state = 'full'))
    AND chipProcessedImfile.fault = 0
    AND chipProcessedImfile.quality = 0
    AND (Label.active OR Label.active IS NULL)

SELECT DISTINCT
    magicDSRun.magic_ds_id,
    magicRun.magic_id,
    magicDSRun.state,
    magicRun.exp_id,
    magicDSRun.label,
    rawExp.camera,
    magicMask.uri as streaks_uri,
    magicMask.path_base as streaks_path_base,
    CAST(NULL AS CHAR(255)) AS inv_streaks_uri,
    CAST(NULL AS CHAR(255)) AS inv_streaks_path_base,
    stage,
    stage_id,
    rawExp.magicked,
    class_id as component,
    0 AS mismatched_tess,
    rawImfile.uri AS uri,
    -- XXX: replace this with rawImfile.path_base once it exists
    TRIM(TRAILING '.fits' FROM rawImfile.uri) AS path_base,
    magicRun.inverse,
    camProcessedExp.path_base as cam_path_base,
    camRun.reduction AS cam_reduction,
    outroot,
    recoveryroot,
    re_place,
    remove,
    IFNULL(Label.priority, 10000) AS priority
FROM magicDSRun
JOIN magicMask USING (magic_id)
JOIN magicRun USING (magic_id)
JOIN camRun USING(cam_id)
JOIN camProcessedExp USING(cam_id)
JOIN rawExp ON magicRun.exp_id = rawExp.exp_id
JOIN rawImfile ON rawExp.exp_id = rawImfile.exp_id
LEFT JOIN magicDSFile
    ON magicDSRun.magic_ds_id = magicDSFile.magic_ds_id
    AND magicDSFile.component = rawImfile.class_id
LEFT JOIN Label ON magicDSRun.label = Label.label
WHERE
    magicDSRun.state = 'new'
    AND magicDSRun.stage = 'raw'
    AND magicDSFile.component IS NULL
    AND (Label.active OR Label.active IS NULL)


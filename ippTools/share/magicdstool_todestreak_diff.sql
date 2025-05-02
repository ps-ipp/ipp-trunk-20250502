SELECT * FROM (
SELECT DISTINCT
    magicDSRun.magic_ds_id,
    magicRun.magic_id,
    magicDSRun.state,
    magicRun.exp_id,
    magicDSRun.label,
    rawExp.camera,
    magicMask.uri AS streaks_uri,
    magicMask.path_base AS streaks_path_base,
    CAST(NULL AS CHAR(255)) AS inv_streaks_uri,
    CAST(NULL AS CHAR(255)) AS inv_streaks_path_base,
    stage,
    magicRun.diff_id AS stage_id,
    diffRun.magicked,
    diffSkyfile.skycell_id AS component,
    0 AS mismatched_tess,
    CAST(NULL AS CHAR(255)) AS uri,
    diffSkyfile.path_base,
    magicRun.inverse,
    CAST(NULL AS CHAR(255)) AS cam_path_base,
    CAST(NULL AS CHAR(255)) AS cam_reduction,
    outroot,
    recoveryroot,
    re_place,
    remove,
    IFNULL(Label.priority, 10000) AS priority
FROM rawExp
JOIN magicRun USING (exp_id)
JOIN magicMask USING (magic_id)
JOIN magicDSRun USING(magic_id)
JOIN magicInputSkyfile USING(magic_id)
JOIN diffRun USING(diff_id)
JOIN diffSkyfile
    ON  magicRun.diff_id = diffSkyfile.diff_id
    AND magicInputSkyfile.node = diffSkyfile.skycell_id
LEFT JOIN magicDSFile
    ON magicDSRun.magic_ds_id = magicDSFile.magic_ds_id
    AND magicDSFile.component = diffSkyfile.skycell_id
LEFT JOIN Label ON magicDSRun.label = Label.label
WHERE
    magicDSRun.state = 'new'
    AND magicDSRun.stage = 'diff'
    AND diffRun.bothways = 0
    AND diffSkyfile.fault = 0
    AND diffSkyfile.quality = 0
    AND magicDSFile.component IS NULL
    AND (Label.active OR Label.active IS NULL)
-- bothways diffSkyfiles
UNION
SELECT DISTINCT
    magicDSRun.magic_ds_id,
    magicRun.magic_id,
    magicDSRun.state,
    magicRun.exp_id,
    magicDSRun.label,
    rawExp.camera,
    magicMask.uri AS streaks_uri,
    magicMask.path_base AS streaks_path_base,
    (SELECT uri from magicMask where magic_id = inv_magic_id) AS inv_streaks_uri,
    (SELECT path_base from magicMask where magic_id = inv_magic_id) AS inv_streaks_path_base,
    stage,
    magicRun.diff_id AS stage_id,
    diffRun.magicked,
    diffSkyfile.skycell_id AS component,
    0 AS mismatched_tess,
    CAST(NULL AS CHAR(255)) AS uri,
    diffSkyfile.path_base,
    magicRun.inverse,
    CAST(NULL AS CHAR(255)) AS cam_path_base,
    CAST(NULL AS CHAR(255)) AS cam_reduction,
    outroot,
    recoveryroot,
    re_place,
    remove,
    IFNULL(Label.priority, 10000) AS priority
FROM rawExp
JOIN magicRun USING (exp_id)
JOIN magicMask USING (magic_id)
JOIN magicDSRun USING(magic_id)
JOIN magicInputSkyfile USING(magic_id)
JOIN diffRun USING(diff_id)
JOIN diffSkyfile
    ON  magicRun.diff_id = diffSkyfile.diff_id
    AND magicInputSkyfile.node = diffSkyfile.skycell_id
LEFT JOIN magicDSFile
    ON magicDSRun.magic_ds_id = magicDSFile.magic_ds_id
    AND magicDSFile.component = diffSkyfile.skycell_id
LEFT JOIN Label ON magicDSRun.label = Label.label
WHERE
    magicDSRun.state = 'new'
    AND magicDSRun.stage = 'diff'
    AND diffRun.bothways
    AND diffSkyfile.fault = 0
    AND diffSkyfile.quality = 0
    AND magicDSFile.component IS NULL
    AND (Label.active OR Label.active IS NULL)
) AS magicDSRun
-- we need the following so this query is compatible with the other stages
WHERE magic_ds_id IS NOT NULL

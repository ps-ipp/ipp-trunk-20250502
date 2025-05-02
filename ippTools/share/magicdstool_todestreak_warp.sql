SELECT DISTINCT
    magicDSRun.magic_ds_id,
    magicRun.magic_id,
    magicDSRun.state,
    magicRun.exp_id,
    magicDSRun.label,
    camera,
    magicMask.uri as streaks_uri,
    magicMask.path_base as streaks_path_base,
    CAST(NULL AS CHAR(255)) AS inv_streaks_uri,
    CAST(NULL AS CHAR(255)) AS inv_streaks_path_base,
    stage,
    stage_id,
    warpRun.magicked,
    warpSkyfile.skycell_id as component,
    diffRun.tess_id != warpRun.tess_id AS mismatched_tess,
    diffRun.tess_id as diff_tess_id,
    warpSkyfile.uri,
    warpSkyfile.path_base,
    magicRun.inverse,
    CAST(NULL AS CHAR(255)) as cam_path_base,
    CAST(NULL AS CHAR(255)) as cam_reduction,
    outroot,
    recoveryroot,
    re_place,
    remove,
    IFNULL(Label.priority, 10000) AS priority
FROM magicDSRun
JOIN magicMask USING (magic_id)
JOIN magicRun USING (magic_id)
JOIN diffRun USING(diff_id)
JOIN warpRun ON warp_id = stage_id
JOIN warpSkyfile USING(warp_id)
JOIN rawExp ON magicRun.exp_id = rawExp.exp_id
LEFT JOIN magicDSFile
    ON magicDSRun.magic_ds_id = magicDSFile.magic_ds_id
    AND magicDSFile.component = warpSkyfile.skycell_id
LEFT JOIN Label ON magicDSRun.label = Label.label
WHERE
    magicDSRun.state = 'new'
    AND magicDSRun.stage = 'warp'
    AND warpRun.state = 'full'
    AND warpSkyfile.fault = 0
    AND warpSkyfile.quality = 0
    AND magicDSFile.component IS NULL
    AND (Label.active OR Label.active IS NULL)

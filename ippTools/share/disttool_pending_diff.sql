SELECT
    distRun.dist_id,
    distRun.label,
    distTarget.dist_group,
    stage,
    stage_id,
    diffSkyfile.skycell_id AS component,
    exp_type,
    clean,
    rawExp.camera,
    CONCAT_WS('.', outroot, CONVERT(distRun.dist_id, CHAR)) as outdir,
    diffSkyfile.path_base,
    CAST(NULL AS CHAR(255)) as chip_path_base,
    diffRun.state,
    -- data_state doesn't exist yet
    -- diffSkyfile.data_state,
    'full' AS data_state,
    diffSkyfile.quality,
    distRun.no_magic,
    diffSkyfile.magicked,
    IFNULL(Label.priority, 10000) AS priority
FROM distRun
JOIN distTarget USING(target_id, stage, clean)
JOIN diffRun ON stage_id = diff_id
JOIN diffSkyfile using(diff_id)
JOIN diffInputSkyfile USING(diff_id, skycell_id)
JOIN warpRun
    ON warpRun.warp_id = diffInputSkyfile.warp1
JOIN fakeRun USING(fake_id)
JOIN camRun USING(cam_id)
JOIN chipRun USING(chip_id)
JOIN rawExp USING(exp_id)
LEFT JOIN distComponent 
    ON distRun.dist_id = distComponent.dist_id 
    AND diffSkyfile.skycell_id = distComponent.component
LEFT JOIN Label ON distRun.label = Label.label
WHERE
    distRun.state = 'new'
    AND distRun.stage = 'diff'
    AND distComponent.dist_id IS NULL
    AND ((diffRun.magicked > 0) OR distRun.no_magic)
    AND (diffRun.state = 'full' OR (distRun.clean AND diffRun.state = 'cleaned'))
    AND (Label.active OR Label.active IS NULL)

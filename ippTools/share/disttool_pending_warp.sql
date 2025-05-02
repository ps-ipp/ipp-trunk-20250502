SELECT
    distRun.dist_id,
    distRun.label,
    distTarget.dist_group,
    stage,
    stage_id,
    warpSkyfile.skycell_id AS component,
    exp_type,
    clean,
    rawExp.camera,
    CONCAT_WS('.', outroot, CONVERT(distRun.dist_id, CHAR)) as outdir,
    warpSkyfile.path_base,
    CAST(NULL AS CHAR(255)) as chip_path_base,
    warpRun.state,
    warpSkyfile.data_state,
    warpSkyfile.quality,
    distRun.no_magic,
    warpSkyfile.magicked,
    IFNULL(Label.priority, 10000) AS priority
FROM distRun
JOIN distTarget USING(target_id, stage, clean)
JOIN warpRun ON stage_id = warp_id
JOIN warpSkyfile using(warp_id)
JOIN fakeRun USING(fake_id)
JOIN camRun USING(cam_id)
JOIN chipRun ON camRun.chip_id = chipRun.chip_id
JOIN rawExp using(exp_id)
LEFT JOIN distComponent 
    ON distRun.dist_id = distComponent.dist_id 
    AND warpSkyfile.skycell_id = distComponent.component
LEFT JOIN Label ON distRun.label = Label.label
WHERE
    distRun.state = 'new'
    AND distRun.stage = 'warp'
    AND distComponent.dist_id IS NULL
    AND ((warpRun.magicked > 0) OR distRun.no_magic)
    AND (warpRun.state = 'full' OR (distRun.clean AND warpRun.state = 'cleaned'))
    AND (Label.active OR Label.active IS NULL)

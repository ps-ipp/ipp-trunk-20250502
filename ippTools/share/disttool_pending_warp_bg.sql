SELECT
    distRun.dist_id,
    distRun.label,
    distTarget.dist_group,
    stage,
    stage_id,
    warpBackgroundSkyfile.skycell_id AS component,
    exp_type,
    clean,
    rawExp.camera,
    CONCAT_WS('.', outroot, CONVERT(distRun.dist_id, CHAR)) as outdir,
    warpBackgroundSkyfile.path_base,
    CAST(NULL AS CHAR(255)) as chip_path_base,
    warpBackgroundRun.state,
    warpBackgroundRun.state as data_state,
    0 as quality,
    distRun.no_magic,
    warpBackgroundSkyfile.magicked,
    IFNULL(Label.priority, 10000) AS priority
FROM distRun
JOIN distTarget USING(target_id, stage, clean)
JOIN warpBackgroundRun ON stage_id = warp_bg_id
JOIN warpBackgroundSkyfile using(warp_bg_id)
JOIN warpRun using(warp_id)
JOIN fakeRun USING(fake_id)
JOIN camRun USING(cam_id)
JOIN chipRun ON camRun.chip_id = chipRun.chip_id
JOIN rawExp using(exp_id)
LEFT JOIN distComponent 
    ON distRun.dist_id = distComponent.dist_id 
    AND warpBackgroundSkyfile.skycell_id = distComponent.component
LEFT JOIN Label ON distRun.label = Label.label
WHERE
    distRun.state = 'new'
    AND distRun.stage = 'warp_bg'
    AND distComponent.dist_id IS NULL
    AND ((warpBackgroundRun.magicked > 0) OR distRun.no_magic)
    AND (warpBackgroundRun.state = 'full')
    AND (Label.active OR Label.active IS NULL)

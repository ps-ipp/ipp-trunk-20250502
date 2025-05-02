SELECT DISTINCT
    distRun.dist_id,
    distRun.label,
    distTarget.dist_group,
    stage,
    stage_id,
    stackRun.skycell_id AS component,
    exp_type,
    clean,
    rawExp.camera,
    CONCAT_WS('.', outroot, CONVERT(distRun.dist_id, CHAR)) as outdir,
    staticskyResult.path_base,
    CAST(NULL AS CHAR(255)) as chip_path_base,
    staticskyRun.state,
    staticskyRun.state AS data_state,
    staticskyResult.quality,
    1 AS no_magic,
    0 AS magicked,
    IFNULL(Label.priority, 10000) AS priority
FROM distRun
JOIN distTarget USING(target_id, stage, clean)
JOIN staticskyRun
    ON stage_id = sky_id
JOIN staticskyResult
    USING(sky_id)
JOIN staticskyInput
    USING(sky_id)
JOIN stackRun USING(stack_id)
JOIN stackInputSkyfile USING(stack_id)
JOIN warpSkyfile
    ON  stackInputSkyfile.warp_id = warpSkyfile.warp_id
    AND stackRun.skycell_id       = warpSkyfile.skycell_id
    AND stackRun.tess_id          = warpSkyfile.tess_id
JOIN warpRun
    ON warpRun.warp_id = warpSkyfile.warp_id
JOIN fakeRun
    USING(fake_id)
JOIN camRun
    USING(cam_id)
JOIN chipRun
    ON camRun.chip_id = chipRun.chip_id
JOIN rawExp 
     USING (exp_id)
LEFT JOIN distComponent 
    ON distRun.dist_id = distComponent.dist_id 
    AND stackRun.skycell_id = distComponent.component
LEFT JOIN Label ON distRun.label = Label.label
WHERE
    distRun.state = 'new'
    AND distRun.stage = 'sky'
    AND distComponent.dist_id IS NULL
    AND (staticskyRun.state = 'full' OR (distRun.clean AND staticskyRun.state = 'cleaned'))
    AND (staticskyResult.fault = 0 AND staticskyResult.quality = 0)
    AND (Label.active OR Label.active IS NULL)

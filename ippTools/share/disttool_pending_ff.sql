-- look for fullForceResult components and a summary components
SELECT * FROM
(
SELECT
    distRun.dist_id,
    distRun.label,
    distTarget.dist_group,
    stage,
    stage_id,
    CONCAT_WS('.', warp_id, stackRun.skycell_id) AS component,
    exp_type,
    clean,
    rawExp.camera,
    CONCAT_WS('.', outroot, CONVERT(distRun.dist_id, CHAR)) as outdir,
    fullForceResult.path_base,
    CAST(NULL AS CHAR(255)) as chip_path_base,
    fullForceRun.state,
    'full' AS data_state,
    fullForceResult.quality,
    distRun.no_magic,
    warpRun.magicked,
    IFNULL(Label.priority, 10000) AS priority
FROM distRun
JOIN distTarget USING(target_id, stage, clean)
JOIN fullForceRun ON stage_id = ff_id
JOIN skycalRun USING(skycal_id)
JOIN stackRun USING(stack_id)
JOIN fullForceResult USING(ff_id)
JOIN warpRun USING(warp_id)
JOIN fakeRun USING(fake_id)
JOIN camRun USING(cam_id)
JOIN chipRun ON camRun.chip_id = chipRun.chip_id
JOIN rawExp using(exp_id)
LEFT JOIN distComponent 
    ON distRun.dist_id = distComponent.dist_id 
    AND CONCAT_WS('.', warp_id, stackRun.skycell_id) = distComponent.component
LEFT JOIN Label ON distRun.label = Label.label
WHERE
    distRun.state = 'new'
    AND distRun.stage = 'ff'
    AND distComponent.dist_id IS NULL
    AND (Label.active OR Label.active IS NULL)
UNION
SELECT DISTINCT
    distRun.dist_id,
    distRun.label,
    distTarget.dist_group,
    stage,
    stage_id,
    'summary' AS component,
    exp_type,
    clean,
    rawExp.camera,
    CONCAT_WS('.', outroot, CONVERT(distRun.dist_id, CHAR)) as outdir,
    fullForceSummary.path_base,
    CAST(NULL AS CHAR(255)) as chip_path_base,
    fullForceRun.state,
    'full' AS data_state,
    fullForceSummary.quality,
    distRun.no_magic,
    0 AS magicked,
    IFNULL(Label.priority, 10000) AS priority
FROM distRun
JOIN distTarget USING(target_id, stage, clean)
JOIN fullForceRun ON stage_id = ff_id
JOIN fullForceSummary USING(ff_id)
JOIN skycalRun USING(skycal_id)
JOIN stackRun USING(stack_id)
JOIN stackInputSkyfile USING(stack_id)
JOIN warpRun USING(warp_id)
JOIN fakeRun USING(fake_id)
JOIN camRun USING(cam_id)
JOIN chipRun ON camRun.chip_id = chipRun.chip_id
JOIN rawExp using(exp_id)
LEFT JOIN distComponent 
    ON distRun.dist_id = distComponent.dist_id 
    AND 'summary' = distComponent.component
LEFT JOIN Label ON distRun.label = Label.label
WHERE
    distRun.state = 'new'
    AND distRun.stage = 'ff'
    AND distComponent.dist_id IS NULL
    AND (Label.active OR Label.active IS NULL)
) as distRun
-- where arguments are appended here
WHERE 1

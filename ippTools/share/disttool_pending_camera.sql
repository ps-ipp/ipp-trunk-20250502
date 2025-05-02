SELECT
    distRun.dist_id,
    distRun.label,
    distTarget.dist_group,
    stage,
    stage_id,
    'exposure' AS component,
    exp_type,
    clean,
    rawExp.camera,
    CONCAT_WS('.', outroot, CONVERT(distRun.dist_id, CHAR)) as outdir,
    camProcessedExp.path_base,
    CAST(NULL AS CHAR(255)) as chip_path_base,
    camRun.state,
    camRun.state AS data_state,
    camProcessedExp.quality,
    distRun.no_magic,
    chipRun.magicked,
    IFNULL(Label.priority, 10000) AS priority
FROM distRun
JOIN distTarget USING(target_id, stage, clean)
JOIN camRun ON camRun.cam_id = distRun.stage_id
JOIN camProcessedExp USING(cam_id)
JOIN chipRun USING(chip_id)
JOIN rawExp using(exp_id)
LEFT JOIN distComponent 
    ON distRun.dist_id = distComponent.dist_id 
LEFT JOIN Label
    ON distRun.label = Label.label
WHERE
    distRun.state = 'new'
    AND distRun.stage = 'camera'
    AND distComponent.dist_id IS NULL
    AND (((clean OR (chipRun.magicked != 0)) AND (camRun.magicked > 0)) OR distRun.no_magic)
    AND (camRun.state = 'full' OR (distRun.clean AND camRun.state = 'cleaned'))
    AND (Label.active OR Label.active IS NULL)

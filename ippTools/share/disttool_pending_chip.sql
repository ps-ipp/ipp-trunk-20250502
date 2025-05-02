SELECT
    distRun.dist_id,
    distRun.label,
    distTarget.dist_group,
    stage,
    stage_id,
    chipProcessedImfile.class_id AS component,
    exp_type,
    distRun.clean,
    rawExp.camera,
    CONCAT_WS('.', outroot, CONVERT(distRun.dist_id, CHAR)) as outdir,
    chipProcessedImfile.path_base,
    chipProcessedImfile.path_base as chip_path_base,
    chipRun.state,
    chipProcessedImfile.data_state,
    chipProcessedImfile.quality,
    distRun.no_magic,
    chipProcessedImfile.magicked,
    IFNULL(Label.priority, 10000) AS priority
FROM distRun
JOIN distTarget USING(target_id, stage, clean)
JOIN chipRun ON chipRun.chip_id = distRun.stage_id
JOIN rawExp using(exp_id)
JOIN chipProcessedImfile ON chipProcessedImfile.chip_id = stage_id
LEFT JOIN distComponent 
    ON distRun.dist_id = distComponent.dist_id 
    AND chipProcessedImfile.class_id = distComponent.component
LEFT JOIN Label
    ON distRun.label = Label.label
WHERE
    distRun.state = 'new'
    AND distRun.stage = 'chip'
    AND distComponent.dist_id IS NULL
    AND ((chipRun.magicked > 0) OR distRun.no_magic)
    AND (chipRun.state = 'full' OR (distRun.clean AND chipRun.state = 'cleaned'))
    AND (Label.active OR Label.active IS NULL)

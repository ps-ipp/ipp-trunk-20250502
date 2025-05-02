SELECT
    distRun.dist_id,
    distRun.label,
    distTarget.dist_group,
    stage,
    stage_id,
    chipBackgroundImfile.class_id AS component,
    exp_type,
    distRun.clean,
    rawExp.camera,
    CONCAT_WS('.', outroot, CONVERT(distRun.dist_id, CHAR)) as outdir,
    chipBackgroundImfile.path_base,
    chipBackgroundImfile.path_base as chip_path_base,
    chipBackgroundRun.state,
    chipBackgroundRun.state AS data_state,
    chipBackgroundImfile.quality AS quality,
    distRun.no_magic,
    chipBackgroundImfile.magicked,
    IFNULL(Label.priority, 10000) AS priority
FROM distRun
JOIN distTarget USING(target_id, stage, clean)
JOIN chipBackgroundRun ON chipBackgroundRun.chip_bg_id = distRun.stage_id
JOIN chipRun using(chip_id)
JOIN rawExp using(exp_id)
JOIN chipBackgroundImfile ON chipBackgroundImfile.chip_bg_id = stage_id
LEFT JOIN distComponent 
    ON distRun.dist_id = distComponent.dist_id 
    AND chipBackgroundImfile.class_id = distComponent.component
LEFT JOIN Label ON distRun.label = Label.label
WHERE
    distRun.state = 'new'
    AND distRun.stage = 'chip_bg'
    AND distComponent.dist_id IS NULL
    AND ((chipBackgroundRun.magicked > 0) OR distRun.no_magic)
    AND (chipBackgroundRun.state = 'full')
    AND (Label.active OR Label.active IS NULL)

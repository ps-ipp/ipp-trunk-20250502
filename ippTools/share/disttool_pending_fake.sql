SELECT
    distRun.dist_id,
    distRun.label,
    distTarget.dist_group,
    stage,
    stage_id,
    fakeProcessedImfile.class_id AS component,
    exp_type,
    clean,
    rawExp.camera,
    CONCAT_WS('.', outroot, CONVERT(distRun.dist_id, CHAR)) as outdir,
    fakeProcessedImfile.path_base,
    CAST(NULL AS CHAR(255)) AS chip_path_base,
    fakeRun.state,
    fakeRun.state AS data_state,
    0 as quality,
    distRun.no_magic,
    0 as magicked,
    IFNULL(Label.priority, 10000) AS priority
FROM distRun
JOIN distTarget USING(target_id, stage, clean)
JOIN fakeRun ON fakeRun.fake_id = distRun.stage_id
JOIN fakeProcessedImfile USING(fake_id)
JOIN camRun USING(cam_id)
JOIN chipRun USING(chip_id, exp_id)
JOIN rawExp using(exp_id)
LEFT JOIN distComponent 
    ON distRun.dist_id = distComponent.dist_id 
    AND fakeProcessedImfile.class_id = distComponent.component
LEFT JOIN Label ON distRun.label = Label.label
WHERE
    distRun.state = 'new'
    AND distRun.stage = 'fake'
    AND distComponent.dist_id IS NULL
    AND (Label.active OR Label.active IS NULL)

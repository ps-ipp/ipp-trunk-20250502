SELECT
    chipRun.*,
    chipImfile.chip_imfile_id,
    rawImfile.class_id,
    rawImfile.uri,
    rawImfile.magicked as raw_magicked,
    (rawImfile.user_1 is not NULL and rawImfile.user_1 > 0.5) as deburned,
    rawImfile.burntool_state as burntool_state,
    rawImfile.video_cells as video_cells,
    rawExp.exp_tag,
    rawExp.exp_name,
    rawExp.camera,
    rawExp.telescope,
    rawExp.filelevel,
    IFNULL(Label.priority, 10000) AS priority,
    chipProcessedImfile.path_base
FROM chipRun
JOIN rawExp
    USING(exp_id)
JOIN rawImfile
    ON rawExp.exp_id = rawImfile.exp_id
    AND rawImfile.ignored = 0
JOIN chipImfile
    USING(chip_id, class_id)
LEFT JOIN chipProcessedImfile
    ON chipRun.chip_id = chipProcessedImfile.chip_id
    AND rawImfile.exp_id = chipProcessedImfile.exp_id
    AND rawImfile.class_id = chipProcessedImfile.class_id
LEFT JOIN chipMask
    ON chipRun.label = chipMask.label
LEFT JOIN Label ON chipRun.label = Label.label
WHERE
    ((chipRun.state = 'new'
    AND chipProcessedImfile.chip_id IS NULL
    AND chipProcessedImfile.exp_id IS NULL
    AND chipProcessedImfile.class_id IS NULL
    AND chipMask.label IS NULL)
    OR
    (chipRun.state = 'update'
    AND chipProcessedImfile.data_state = 'update'
    AND chipProcessedImfile.fault = 0))
    AND (Label.active OR Label.active IS NULL)

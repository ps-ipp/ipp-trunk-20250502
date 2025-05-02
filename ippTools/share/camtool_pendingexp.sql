SELECT
    camRun.*,
    rawExp.exp_tag,
    rawExp.exp_id,
    rawExp.exp_name,
    rawExp.camera,
    rawExp.telescope,
    rawExp.filelevel,
    chipRun.magicked AS chip_magicked,
    camProcessedExp.path_base,
    IFNULL(Label.priority, 10000) AS priority
FROM camRun
JOIN chipRun
    USING(chip_id)
JOIN rawExp
    USING(exp_id)
LEFT JOIN camProcessedExp
    USING(cam_id)
LEFT JOIN camMask
    ON camRun.label = camMask.label
LEFT JOIN Label
    ON camRun.label = Label.label
WHERE
    chipRun.state = 'full'
    AND ((camRun.state = 'new' AND camProcessedExp.cam_id IS NULL) OR
         (camRun.state = 'update' and camProcessedExp.fault = 0 and camProcessedExp.quality = 0))
    AND camMask.label IS NULL
    AND (Label.active OR Label.active IS NULL)

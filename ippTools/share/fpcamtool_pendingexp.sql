SELECT
    fpcamRun.*,
    rawExp.exp_tag,
    rawExp.exp_id,
    rawExp.exp_name,
    rawExp.camera,
    rawExp.telescope,
    rawExp.filelevel,
    fpcamProcessedExp.path_base,
    IFNULL(Label.priority, 10000) AS priority
FROM fpcamRun
JOIN camRun
    USING(cam_id)
JOIN chipRun
    ON(fpcamRun.chip_id = chipRun.chip_id)
JOIN rawExp
    USING(exp_id)
LEFT JOIN fpcamProcessedExp
    USING(fpcam_id)
LEFT JOIN Label
    ON fpcamRun.label = Label.label
WHERE
    chipRun.state = 'full'
    AND camRun.state = 'full'
    AND ((fpcamRun.state = 'new'    AND fpcamProcessedExp.fpcam_id IS NULL) OR
         (fpcamRun.state = 'update' AND fpcamProcessedExp.fault = 0 AND fpcamProcessedExp.quality = 0))
    AND (Label.active OR Label.active IS NULL)

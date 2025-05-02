SELECT
    camRun.cam_id,
    chipProcessedImfile.*,
    (IF(rawExp.exp_time IS NOT NULL AND deteff_magref IS NOT NULL,
       (2.5 * LOG10(rawExp.exp_time) + chipProcessedImfile.deteff_magref),
        NULL))
        AS deteff_inst,
    rawExp.exp_name,
    rawExp.camera,
    rawExp.telescope,
    rawExp.filelevel
FROM camRun
JOIN chipRun
    USING (chip_id)
JOIN chipProcessedImfile
    USING (chip_id, exp_id)
JOIN rawExp
    USING(exp_id)
JOIN rawImfile
    USING(exp_id, class_id)
LEFT JOIN camProcessedExp
    USING(cam_id)
LEFT JOIN camMask
    ON camRun.label = camMask.label
WHERE
    camMask.label IS NULL
    AND rawImfile.ignored = 0
